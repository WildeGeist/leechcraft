/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2014  Georg Rudoy
 *
 * Boost Software License - Version 1.0 - August 17th, 2003
 *
 * Permission is hereby granted, free of charge, to any person or organization
 * obtaining a copy of the software and accompanying documentation covered by
 * this license (the "Software") to use, reproduce, display, distribute,
 * execute, and transmit the Software, and to prepare derivative works of the
 * Software, and to permit third-parties to whom the Software is furnished to
 * do so, all subject to the following:
 *
 * The copyright notices in the Software and this entire statement, including
 * the above license grant, this restriction and the following disclaimer,
 * must be included in all copies of the Software, in whole or in part, and
 * all derivative works of the Software, unless such copies or derivative
 * works are solely in the form of machine-executable object code generated by
 * a source language processor.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
 * SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
 * FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 **********************************************************************/

#include <stdexcept>
#include <numeric>
#include <algorithm>
#include <QtDebug>
#include <QImage>
#include <QDir>
#include <QDesktopServices>
#include <QUrl>
#include <QTimer>
#include <QTextCodec>
#include <QXmlStreamReader>
#include <QNetworkReply>
#include <interfaces/iwebbrowser.h>
#include <interfaces/core/icoreproxy.h>
#include <interfaces/core/itagsmanager.h>
#include <interfaces/core/ipluginsmanager.h>
#include <interfaces/core/ientitymanager.h>
#include <util/models/mergemodel.h>
#include <util/xpc/util.h>
#include <util/sys/paths.h>
#include <util/xpc/defaulthookproxy.h>
#include <util/sll/prelude.h>
#include <util/sll/qtutil.h>
#include <util/sll/visitor.h>
#include <util/sll/either.h>
#include <util/gui/util.h>
#include "core.h"
#include "xmlsettingsmanager.h"
#include "parserfactory.h"
#include "opmlparser.h"
#include "opmlwriter.h"
#include "jobholderrepresentation.h"
#include "importopml.h"
#include "addfeeddialog.h"
#include "dbupdatethread.h"
#include "dbupdatethreadworker.h"
#include "dumbstorage.h"
#include "storagebackendmanager.h"
#include "parser.h"

namespace LeechCraft
{
namespace Aggregator
{
	Core& Core::Instance ()
	{
		static Core core;
		return core;
	}

	void Core::Release ()
	{
		DBUpThread_.reset ();

		StorageBackend_.reset ();

		XmlSettingsManager::Instance ()->Release ();
	}

	void Core::SetProxy (ICoreProxy_ptr proxy)
	{
		Proxy_ = proxy;
	}

	ICoreProxy_ptr Core::GetProxy () const
	{
		return Proxy_;
	}

	Util::IDPool<IDType_t>& Core::GetPool (PoolType type)
	{
		return Pools_ [type];
	}

	DBUpdateThread& Core::GetDBUpdateThread () const
	{
		return *DBUpThread_;
	}

	bool Core::CouldHandle (const Entity& e)
	{
		if (!e.Entity_.canConvert<QUrl> () ||
				!Initialized_)
			return false;

		const auto& url = e.Entity_.toUrl ();

		if (e.Mime_ == "text/x-opml")
		{
			if (url.scheme () != "file" &&
					url.scheme () != "http" &&
					url.scheme () != "https" &&
					url.scheme () != "itpc")
				return false;
		}
		else if (e.Mime_ == "text/xml")
		{
			if (url.scheme () != "http" &&
					url.scheme () != "https")
				return false;

			const auto& pageData = e.Additional_ ["URLData"].toString ();
			QXmlStreamReader xmlReader (pageData);
			if (!xmlReader.readNextStartElement ())
				return false;
			return xmlReader.name () == "rss" || xmlReader.name () == "atom";
		}
		else
		{
			if (url.scheme () == "feed")
				return true;
			if (url.scheme () == "itpc")
				return true;
			if (url.scheme () != "http" &&
					url.scheme () != "https" &&
					url.scheme () != "itpc")
				return false;

			if (e.Mime_ != "application/atom+xml" &&
					e.Mime_ != "application/rss+xml")
				return false;

			const auto& linkRel = e.Additional_ ["LinkRel"].toString ();
			if (!linkRel.isEmpty () &&
					linkRel != "alternate")
				return false;
		}

		return true;
	}

	void Core::Handle (Entity e)
	{
		QUrl url = e.Entity_.toUrl ();
		if (e.Mime_ == "text/x-opml")
		{
			if (url.scheme () == "file")
				StartAddingOPML (url.toLocalFile ());
			else
			{
				const auto& name = Util::GetTemporaryName ();

				const auto& dlEntity = Util::MakeEntity (url,
						name,
						Internal |
							DoNotNotifyUser |
							DoNotSaveInHistory |
							NotPersistent |
							DoNotAnnounceEntity);

				const auto& handleResult = Proxy_->GetEntityManager ()->DelegateEntity (dlEntity);
				if (!handleResult)
				{
					ErrorNotification (tr ("Import error"),
							tr ("Could not find plugin to download OPML %1.")
								.arg (url.toString ()));
					return;
				}

				Util::Sequence (this, handleResult.DownloadResult_) >>
						Util::Visitor
						{
							[this, name] (IDownload::Success) { StartAddingOPML (name); },
							[this] (const IDownload::Error&)
							{
								ErrorNotification (tr ("OPML import error"),
										tr ("Unable to download the OPML file."));
							}
						}.Finally ([name] { QFile::remove (name); });
			}

			const auto& s = e.Additional_;
			auto copyVal = [&s] (const QByteArray& name)
			{
				if (s.contains (name))
					XmlSettingsManager::Instance ()->setProperty (name, s.value (name));
			};
			copyVal ("UpdateOnStartup");
			copyVal ("UpdateTimeout");
			copyVal ("MaxArticles");
			copyVal ("MaxAge");
		}
		else
		{
			QString str = url.toString ();
			if (str.startsWith ("feed://"))
				str.replace (0, 4, "http");
			else if (str.startsWith ("feed:"))
				str.remove  (0, 5);
			else if (str.startsWith ("itpc://"))
				str.replace (0, 4, "http");

			AddFeedDialog af { Proxy_->GetTagsManager (), str };
			if (af.exec () == QDialog::Accepted)
				AddFeed (af.GetURL (),
						af.GetTags ());
		}
	}

	void Core::StartAddingOPML (const QString& file)
	{
		ImportOPML importDialog (file);
		if (importDialog.exec () == QDialog::Rejected)
			return;

		const auto& tags = Proxy_->GetTagsManager ()->Split (importDialog.GetTags ());
		const auto& selectedUrls = importDialog.GetSelectedUrls ();

		Util::Visit (ParseOPMLItems (importDialog.GetFilename ()),
				[this] (const QString& error) { ErrorNotification (tr ("OPML import error"), error); },
				[&] (const OPMLParser::items_container_t& items)
				{
					for (const auto& item : items)
					{
						if (!selectedUrls.contains (item.URL_))
							continue;

						int interval = 0;
						if (item.CustomFetchInterval_)
							interval = item.FetchInterval_;
						AddFeed (item.URL_, tags + item.Categories_,
								{ { IDNotFound, interval, item.MaxArticleNumber_, item.MaxArticleAge_, false } });
					}
				});
	}

	bool Core::DoDelayedInit ()
	{
		bool result = true;

		QDir dir = QDir::home ();
		if (!dir.cd (".leechcraft/aggregator") &&
				!dir.mkpath (".leechcraft/aggregator"))
		{
			qCritical () << Q_FUNC_INFO << "could not create necessary "
				"directories for Aggregator";
			result = false;
		}

		if (!ReinitStorage ())
			result = false;

		DBUpThread_ = std::make_shared<DBUpdateThread> (Proxy_);
		DBUpThread_->SetAutoQuit (true);
		DBUpThread_->start (QThread::LowestPriority);

		ParserFactory::Instance ().RegisterDefaultParsers ();

		CustomUpdateTimer_ = new QTimer (this);
		CustomUpdateTimer_->start (60 * 1000);
		connect (CustomUpdateTimer_,
				&QTimer::timeout,
				this,
				&Core::handleCustomUpdates);

		UpdateTimer_ = new QTimer (this);
		UpdateTimer_->setSingleShot (true);
		connect (UpdateTimer_,
				&QTimer::timeout,
				this,
				&Core::updateFeeds);

		auto now = QDateTime::currentDateTime ();
		auto lastUpdated = XmlSettingsManager::Instance ()->Property ("LastUpdateDateTime", now).toDateTime ();
		if (auto interval = XmlSettingsManager::Instance ()->property ("UpdateInterval").toInt ())
		{
			auto updateDiff = lastUpdated.secsTo (now);
			if (XmlSettingsManager::Instance ()->property ("UpdateOnStartup").toBool () ||
					updateDiff > interval * 60)
				QTimer::singleShot (7000,
						this,
						SLOT (updateFeeds ()));
			else
				UpdateTimer_->start (updateDiff * 1000);
		}

		XmlSettingsManager::Instance ()->RegisterObject ("UpdateInterval", this, "updateIntervalChanged");
		Initialized_ = true;

		return result;
	}

	bool Core::ReinitStorage ()
	{
		const auto result = Util::Visit (StorageBackendManager::Instance ().CreatePrimaryStorage (),
				[this] (const StorageBackend_ptr& backend)
				{
					StorageBackend_ = backend;
					return true;
				},
				[this] (const auto& error)
				{
					ErrorNotification (tr ("Storage error"), error.Message_);
					return false;
				});

		if (!result)
			return false;

		Pools_.clear ();
		for (int type = 0; type < PTMAX; ++type)
		{
			Util::IDPool<IDType_t> pool;
			pool.SetID (StorageBackend_->GetHighestID (static_cast<PoolType> (type)) + 1);
			Pools_ [static_cast<PoolType> (type)] = pool;
		}

		return true;
	}

	namespace
	{
		using ParseResult = Util::Either<QString, channels_container_t>;

		ParseResult ParseChannels (const QString& path, const QString& url, IDType_t feedId)
		{
			QFile file { path };
			if (!file.open (QIODevice::ReadOnly))
			{
				qWarning () << Q_FUNC_INFO
						<< "unable to open the local file"
						<< path;
				return ParseResult::Left (Core::tr ("Unable to open the temporary file."));
			}

			QDomDocument doc;
			QString errorMsg;
			int errorLine, errorColumn;
			if (!doc.setContent (&file, true, &errorMsg, &errorLine, &errorColumn))
			{
				const auto& copyPath = Util::GetTemporaryName ("lc_aggregator_failed.XXXXXX");
				file.copy (copyPath);
				qWarning () << Q_FUNC_INFO
						<< "error parsing XML for"
						<< url
						<< errorMsg
						<< errorLine
						<< errorColumn
						<< "; copy at"
						<< file.fileName ();
				return ParseResult::Left (Core::tr ("XML parse error for the feed %1.")
								.arg (url));
			}

			auto parser = ParserFactory::Instance ().Return (doc);
			if (!parser)
			{
				const auto& copyPath = Util::GetTemporaryName ("lc_aggregator_failed.XXXXXX");
				file.copy (copyPath);
				qWarning () << Q_FUNC_INFO
						<< "no parser for"
						<< url
						<< "; copy at"
						<< copyPath;
				return ParseResult::Left (Core::tr ("Could not find parser to parse %1.")
								.arg (url));
			}

			return ParseResult::Right (parser->ParseFeed (doc, feedId));
		}

		QString GetErrorString (const IDownload::Error::Type type)
		{
			switch (type)
			{
			case IDownload::Error::Type::Unknown:
				break;
			case IDownload::Error::Type::NoError:
				return Core::tr ("no error");
			case IDownload::Error::Type::NotFound:
				return Core::tr ("address not found");
			case IDownload::Error::Type::AccessDenied:
				return Core::tr ("access denied");
			case IDownload::Error::Type::LocalError:
				return Core::tr ("local error");
			case IDownload::Error::Type::UserCanceled:
				return Core::tr ("user canceled the download");
			}

			return Core::tr ("unknown error");
		}
	}

	void Core::AddFeed (QString url, const QStringList& tags, const std::optional<Feed::FeedSettings>& maybeFeedSettings)
	{
		const auto& fixedUrl = QUrl::fromUserInput (url);
		url = fixedUrl.toString ();
		if (StorageBackend_->FindFeed (url))
		{
			ErrorNotification (tr ("Feed addition error"),
					tr ("The feed %1 is already added")
						.arg (url));
			return;
		}

		const auto& name = Util::GetTemporaryName ();
		const auto& e = Util::MakeEntity (fixedUrl,
				name,
				Internal |
					DoNotNotifyUser |
					DoNotSaveInHistory |
					NotPersistent |
					DoNotAnnounceEntity);

		const auto& delegateResult = Proxy_->GetEntityManager ()->DelegateEntity (e);
		if (!delegateResult)
		{
			ErrorNotification (tr ("Plugin error"),
					tr ("Could not find plugin to download feed %1.")
						.arg (url),
					false);
			return;
		}

		const auto& tagIds = Proxy_->GetTagsManager ()->GetIDs (tags);

		Util::Sequence (this, delegateResult.DownloadResult_) >>
				Util::Visitor
				{
					[=] (IDownload::Success)
					{
						Feed feed;
						feed.URL_ = url;
						StorageBackend_->AddFeed (feed);

						if (maybeFeedSettings)
						{
							auto fs = *maybeFeedSettings;
							fs.FeedID_ = feed.FeedID_;
							StorageBackend_->SetFeedSettings (fs);
						}

						Util::Visit (ParseChannels (name, url, feed.FeedID_),
								[&] (const channels_container_t& channels) { HandleFeedAdded (channels, tagIds); },
								[&] (const QString& error) { ErrorNotification (tr ("Feed error"), error); });
					},
					[=] (const IDownload::Error& error)
					{
						ErrorNotification (tr ("Feed error"),
								tr ("Unable to download %1: %2.")
									.arg (Util::FormatName (url))
									.arg (GetErrorString (error.Type_)));
					}
				}.Finally ([name] { QFile::remove (name); });
	}

	void Core::UpdateFavicon (const QModelIndex& index)
	{
		FetchFavicon (index.data (ChannelRoles::ChannelID).value<IDType_t> (),
				index.data (ChannelRoles::ChannelLink).toString ());
	}

	void Core::AddFeeds (const feeds_container_t& feeds, const QString& tagsString)
	{
		auto tags = Proxy_->GetTagsManager ()->Split (tagsString);
		tags.removeDuplicates ();

		for (const auto& feed : feeds)
		{
			for (const auto& channel : feed->Channels_)
			{
				channel->Tags_ += tags;
				channel->Tags_.removeDuplicates ();
			}

			StorageBackend_->AddFeed (*feed);
		}
	}

	void Core::updateFeeds ()
	{
		for (const auto id : StorageBackend_->GetFeedsIDs ())
		{
			// It's handled by custom timer.
			if (StorageBackend_->GetFeedSettings (id).value_or (Feed::FeedSettings {}).UpdateTimeout_)
				continue;

			UpdateFeed (id);
		}
		XmlSettingsManager::Instance ()->setProperty ("LastUpdateDateTime", QDateTime::currentDateTime ());
		if (int interval = XmlSettingsManager::Instance ()->property ("UpdateInterval").toInt ())
			UpdateTimer_->start (interval * 60 * 1000);
	}

	void Core::FetchExternalFile (const QString& url, const std::function<void (QString)>& cont)
	{
		auto where = Util::GetTemporaryName ();

		const auto& e = Util::MakeEntity (QUrl (url),
				where,
				Internal |
					DoNotNotifyUser |
					DoNotSaveInHistory |
					NotPersistent |
					DoNotAnnounceEntity);

		const auto& delegateResult = Proxy_->GetEntityManager ()->DelegateEntity (e);
		if (!delegateResult)
		{
			ErrorNotification (tr ("Feed error"),
					tr ("Could not find plugin to download external file %1.").arg (url));
			return;
		}

		Util::Sequence (this, delegateResult.DownloadResult_) >>
				Util::Visitor
				{
					[=] (IDownload::Success) { cont (where); },
					[] (const IDownload::Error&) {}
				}.Finally ([where] { QFile::remove (where); });
	}

	void Core::updateIntervalChanged ()
	{
		int min = XmlSettingsManager::Instance ()->
			property ("UpdateInterval").toInt ();
		if (min)
		{
			if (UpdateTimer_->isActive ())
				UpdateTimer_->setInterval (min * 60 * 1000);
			else
				UpdateTimer_->start (min * 60 * 1000);
		}
		else
			UpdateTimer_->stop ();
	}

	void Core::handleCustomUpdates ()
	{
		using Util::operator*;

		QDateTime current = QDateTime::currentDateTime ();
		for (const auto id : StorageBackend_->GetFeedsIDs ())
		{
			const auto ut = (StorageBackend_->GetFeedSettings (id) * &Feed::FeedSettings::UpdateTimeout_).value_or (0);

			// It's handled by normal timer.
			if (!ut)
				continue;

			if (!Updates_.contains (id) ||
					(Updates_ [id].isValid () &&
						Updates_ [id].secsTo (current) / 60 > ut))
			{
				UpdateFeed (id);
				Updates_ [id] = QDateTime::currentDateTime ();
			}
		}
	}

	void Core::rotateUpdatesQueue ()
	{
		if (UpdatesQueue_.isEmpty ())
			return;

		const auto feedId = UpdatesQueue_.takeFirst ();

		if (!UpdatesQueue_.isEmpty ())
			QTimer::singleShot (2000,
					this,
					SLOT (rotateUpdatesQueue ()));

		const auto& url = StorageBackend_->GetFeed (feedId).URL_;

		auto filename = Util::GetTemporaryName ();

		auto e = Util::MakeEntity (QUrl (url),
				filename,
				Internal |
					DoNotNotifyUser |
					DoNotSaveInHistory |
					NotPersistent |
					DoNotAnnounceEntity);

		const auto& delegateResult = Proxy_->GetEntityManager ()->DelegateEntity (e);
		if (!delegateResult)
		{
			ErrorNotification ("Aggregator",
					tr ("Could not find plugin for feed with URL %1")
						.arg (url));
			return;
		}

		Util::Sequence (this, delegateResult.DownloadResult_) >>
				Util::Visitor
				{
					[=] (IDownload::Success)
					{
						Util::Visit (ParseChannels (filename, url, feedId),
								[&] (const channels_container_t& channels)
								{
									DBUpThread_->ScheduleImpl (&DBUpdateThreadWorker::updateFeed, channels, url);
								},
								[this] (const QString& error) { ErrorNotification (tr ("Feed error"), error); });
					},
					[=] (const IDownload::Error& error)
					{
						if (!XmlSettingsManager::Instance ()->property ("BeSilent").toBool ())
							ErrorNotification (tr ("Feed error"),
									tr ("Unable to download %1: %2.")
											.arg (Util::FormatName (url))
											.arg (GetErrorString (error.Type_)));
					}
				}.Finally ([filename] { QFile::remove (filename); });
	}

	void Core::FetchPixmap (const Channel& channel)
	{
		if (QUrl (channel.PixmapURL_).isValid () &&
				!QUrl (channel.PixmapURL_).isRelative ())
		{
			auto cid = channel.ChannelID_;
			FetchExternalFile (channel.PixmapURL_,
					[this, cid] (const QString& path) { StorageBackend_->SetChannelPixmap (cid, QImage { path }); });
		}
	}

	void Core::FetchFavicon (IDType_t cid, const QString& link)
	{
		QUrl oldUrl { link };
		oldUrl.setPath ("/favicon.ico");
		QString iconUrl = oldUrl.toString ();

		FetchExternalFile (iconUrl,
				[this, cid] (const QString& path) { StorageBackend_->SetChannelFavicon (cid, QImage { path }); });
	}

	void Core::HandleFeedAdded (const channels_container_t& channels, const QStringList& tagIds)
	{
		for (const auto& channel : channels)
		{
			for (const auto& item : channel->Items_)
				item->FixDate ();

			channel->Tags_ = tagIds;
			StorageBackend_->AddChannel (*channel);

			FetchPixmap (*channel);
			FetchFavicon (channel->ChannelID_, channel->Link_);
		}
	}

	void Core::UpdateFeed (const IDType_t& id)
	{
		if (UpdatesQueue_.isEmpty ())
			QTimer::singleShot (500,
					this,
					SLOT (rotateUpdatesQueue ()));

		UpdatesQueue_ << id;
	}

	void Core::ErrorNotification (const QString& h, const QString& body, bool wait) const
	{
		auto e = Util::MakeNotification (h, body, Priority::Critical);
		e.Additional_ ["UntilUserSees"] = wait;
		Proxy_->GetEntityManager ()->HandleEntity (e);
	}
}
}
