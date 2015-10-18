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

#include "albumsmanager.h"
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QStandardItem>
#include <QIcon>
#include <QTimer>
#include <QtDebug>
#include <util/svcauth/vkauthmanager.h>
#include <util/sll/queuemanager.h>
#include <util/sll/parsejson.h>
#include <util/sll/urloperator.h>
#include <util/sll/qtutil.h>
#include <interfaces/media/iradiostationprovider.h>
#include <interfaces/media/audiostructs.h>
#include <interfaces/core/iiconthememanager.h>
#include "util.h"

namespace LeechCraft
{
namespace TouchStreams
{
	namespace
	{
		enum Role
		{
			AlbumID = Media::RadioItemRole::MaxRadioRole + 1
		};
	}

	AlbumsManager::AlbumInfo::AlbumInfo (qlonglong id, const QString& name, QStandardItem *item)
	: ID_ { id }
	, Name_ { name }
	, Item_ { item }
	{
	}

	AlbumsManager::AlbumsManager (Util::SvcAuth::VkAuthManager *authMgr,
			ICoreProxy_ptr proxy, QObject *parent)
	: AlbumsManager (-1, authMgr, proxy, parent)
	{
	}

	AlbumsManager::AlbumsManager (qlonglong id, Util::SvcAuth::VkAuthManager *authMgr,
			ICoreProxy_ptr proxy, QObject *parent)
	: QObject (parent)
	, Proxy_ (proxy)
	, UserID_ (id)
	, AuthMgr_ (authMgr)
	, RequestQueueGuard_ (AuthMgr_->ManageQueue (&RequestQueue_))
	, AlbumsRootItem_ (new QStandardItem (tr ("VKontakte: your audio")))
	{
		InitRootItem ();

		QTimer::singleShot (1000,
				this,
				SLOT (refetchAlbums ()));

		connect (AuthMgr_,
				SIGNAL (justAuthenticated ()),
				this,
				SLOT (refetchAlbums ()));
	}

	AlbumsManager::AlbumsManager (qlonglong id, const QVariant& albums, const QVariant& tracks,
			Util::SvcAuth::VkAuthManager *authMgr, ICoreProxy_ptr proxy, QObject *parent)
	: QObject (parent)
	, Proxy_ (proxy)
	, UserID_ (id)
	, AuthMgr_ (authMgr)
	, RequestQueueGuard_ (AuthMgr_->ManageQueue (&RequestQueue_))
	, AlbumsRootItem_ (new QStandardItem (tr ("VKontakte: your audio")))
	{
		InitRootItem ();

		HandleAlbums (albums);
		HandleTracks (tracks);
	}

	QStandardItem* AlbumsManager::GetRootItem () const
	{
		return AlbumsRootItem_;
	}

	qlonglong AlbumsManager::GetUserID () const
	{
		return UserID_;
	}

	quint32 AlbumsManager::GetTracksCount () const
	{
		return TracksCount_;
	}

	void AlbumsManager::RefreshItems (QList<QStandardItem*>& items)
	{
		for (const auto item : items)
		{
			auto parent = item;
			while (parent)
			{
				if (AlbumsRootItem_ != parent)
				{
					parent = parent->parent ();
					continue;
				}

				if (auto rc = AlbumsRootItem_->rowCount ())
					AlbumsRootItem_->removeRows (0, rc);
				Albums_.clear ();
				refetchAlbums ();

				items.removeOne (item);
				return;
			}
		}
	}

	void AlbumsManager::InitRootItem ()
	{
		static QIcon vkIcon { ":/touchstreams/resources/images/vk.svg" };
		AlbumsRootItem_->setIcon (vkIcon);
		AlbumsRootItem_->setEditable (false);
		AlbumsRootItem_->setData (Media::RadioType::TracksRoot, Media::RadioItemRole::ItemType);
	}

	bool AlbumsManager::HandleAlbums (const QVariant& albumsListVar)
	{
		auto albumsList = albumsListVar.toList ();

		if (albumsList.isEmpty ())
		{
			emit finished (this);
			return false;
		}

		albumsList.removeFirst ();

		if (auto rc = AlbumsRootItem_->rowCount ())
			AlbumsRootItem_->removeRows (0, rc);
		Albums_.clear ();

		const auto& icon = Proxy_->GetIconThemeManager ()->GetIcon ("media-optical");

		auto allItem = new QStandardItem (tr ("Uncategorized"));
		allItem->setEditable (false);
		allItem->setData (-1, Role::AlbumID);
		allItem->setData (Media::RadioType::TracksRoot, Media::RadioItemRole::ItemType);
		allItem->setIcon (icon);
		AlbumsRootItem_->appendRow (allItem);
		Albums_ [-1] = AlbumInfo { -1, allItem->text (), allItem };

		for (const auto& albumMap : albumsList)
		{
			const auto& map = albumMap.toMap ();

			const auto id = map ["album_id"].toLongLong ();
			const auto& name = map ["title"].toString ();

			auto item = new QStandardItem (name);
			item->setEditable (false);
			item->setIcon (icon);
			item->setData (Media::RadioType::TracksRoot, Media::RadioItemRole::ItemType);
			item->setData (id, Role::AlbumID);
			Albums_ [id] = AlbumInfo { id, name, item };

			AlbumsRootItem_->appendRow (item);
		}

		return true;
	}

	bool AlbumsManager::HandleTracks (const QVariant& tracksListVar)
	{
		for (const auto& trackVar : tracksListVar.toMap () ["items"].toList ())
		{
			const auto& map = trackVar.toMap ();

			const auto& url = QUrl::fromEncoded (map ["url"].toString ().toUtf8 ());
			if (!url.isValid ())
				continue;

			const auto albumId = map.value ("album_id", "-1").toLongLong ();
			auto albumItem = Albums_ [albumId].Item_;
			if (!albumItem)
			{
				qWarning () << Q_FUNC_INFO
						<< "no album item for album"
						<< albumId;
				continue;
			}

			Media::AudioInfo info {};
			info.Title_ = map ["title"].toString ();
			info.Artist_ = map ["artist"].toString ();
			info.Length_ = map ["duration"].toInt ();
			info.Other_ ["URL"] = url;

			QUrl radioID { "vk://track" };
			Util::UrlOperator { radioID }
					("audio_id", map.value ("id").toString ())
					("owner_id", map.value ("owner_id").toString ());

			auto trackItem = new QStandardItem (QString::fromUtf8 ("%1 — %2")
						.arg (info.Artist_)
						.arg (info.Title_));
			trackItem->setEditable (false);
			trackItem->setData (Media::RadioType::SingleTrack, Media::RadioItemRole::ItemType);
#if QT_VERSION >= 0x050000
			trackItem->setData (radioID.toString (QUrl::FullyEncoded), Media::RadioItemRole::RadioID);
#else
			trackItem->setData (radioID.toString (), Media::RadioItemRole::RadioID);
#endif
			trackItem->setData ("org.LeechCraft.TouchStreams", Media::RadioItemRole::PluginID);
			trackItem->setData (QVariant::fromValue<QList<Media::AudioInfo>> ({ info }),
					Media::RadioItemRole::TracksInfos);
			albumItem->appendRow (trackItem);

			++TracksCount_;
		}

		return true;
	}

	void AlbumsManager::refetchAlbums ()
	{
		if (!CheckAuthentication (AlbumsRootItem_, AuthMgr_, Proxy_))
			return;

		RequestQueue_.append ({
				[this] (const QString& key) -> void
				{
					QUrl url ("https://api.vk.com/method/audio.getAlbums");
					Util::UrlOperator { url }
							("access_token", key)
							("count", "100");
					if (UserID_ >= 0)
						Util::UrlOperator { url }
								("uid", QString::number (UserID_));

					auto nam = Proxy_->GetNetworkAccessManager ();
					connect (nam->get (QNetworkRequest (url)),
							SIGNAL (finished ()),
							this,
							SLOT (handleAlbumsFetched ()));
				},
				Util::QueuePriority::Normal
			});
		AuthMgr_->GetAuthKey ();
	}

	void AlbumsManager::handleAlbumsFetched ()
	{
		auto reply = qobject_cast<QNetworkReply*> (sender ());
		reply->deleteLater ();

		const auto& data = Util::ParseJson (reply, Q_FUNC_INFO).toMap ();
		HandleAlbums (data ["response"]);

		RequestQueue_.prepend ({
				[this] (const QString& key) -> void
				{
					QUrl url ("https://api.vk.com/method/audio.get");
					Util::UrlOperator { url }
							("v", "5.37")
							("access_token", key)
							("count", "6000");
					if (UserID_ >= 0)
						Util::UrlOperator { url } ("owner_id", QString::number (UserID_));

					auto nam = Proxy_->GetNetworkAccessManager ();
					connect (nam->get (QNetworkRequest (url)),
							SIGNAL (finished ()),
							this,
							SLOT (handleTracksFetched ()));
				},
				Util::QueuePriority::High
			});
		AuthMgr_->GetAuthKey ();
	}

	void AlbumsManager::handleTracksFetched ()
	{
		auto reply = qobject_cast<QNetworkReply*> (sender ());
		reply->deleteLater ();

		const auto& data = Util::ParseJson (reply, Q_FUNC_INFO).toMap ();
		HandleTracks (data ["response"]);

		emit finished (this);
	}
}
}
