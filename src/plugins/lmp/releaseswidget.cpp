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

#include "releaseswidget.h"

#if QT_VERSION < 0x050000
#include <QDeclarativeView>
#include <QDeclarativeContext>
#include <QDeclarativeEngine>
#include <QGraphicsObject>
#else
#include <QQuickWidget>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickItem>
#endif

#include <QStandardItemModel>
#include <QtDebug>
#include <util/xpc/util.h>
#include <util/qml/colorthemeproxy.h>
#include <util/qml/standardnamfactory.h>
#include <util/qml/themeimageprovider.h>
#include <util/sys/paths.h>
#include <util/models/rolenamesmixin.h>
#include <interfaces/core/icoreproxy.h>
#include <interfaces/core/ipluginsmanager.h>
#include <interfaces/media/irecentreleases.h>
#include <interfaces/media/idiscographyprovider.h>
#include <interfaces/iinfo.h>
#include "core.h"
#include "xmlsettingsmanager.h"
#include "util.h"

namespace LeechCraft
{
namespace LMP
{
	namespace
	{
		class ReleasesModel : public Util::RoleNamesMixin<QStandardItemModel>
		{
		public:
			enum Role
			{
				AlbumName = Qt::UserRole + 1,
				ArtistName,
				AlbumImageThumb,
				AlbumImageFull,
				ReleaseDate,
				ReleaseURL,
				TrackList
			};

			ReleasesModel (QObject *parent = 0)
			: RoleNamesMixin<QStandardItemModel> (parent)
			{
				QHash<int, QByteArray> names;
				names [AlbumName] = "albumName";
				names [ArtistName] = "artistName";
				names [AlbumImageThumb] = "albumThumbImage";
				names [AlbumImageFull] = "albumFullImage";
				names [ReleaseDate] = "releaseDate";
				names [ReleaseURL] = "releaseURL";
				names [TrackList] = "trackList";
				setRoleNames (names);
			}
		};
	}

	ReleasesWidget::ReleasesWidget (QWidget *parent)
	: QWidget (parent)
#if QT_VERSION < 0x050000
	, ReleasesView_ (new QDeclarativeView)
#else
	, ReleasesView_ (new QQuickWidget)
#endif
	, ReleasesModel_ (new ReleasesModel (this))
	{
		Ui_.setupUi (this);
		layout ()->addWidget (ReleasesView_);

#if QT_VERSION < 0x050000
		ReleasesView_->setResizeMode (QDeclarativeView::SizeRootObjectToView);
#else
		ReleasesView_->setResizeMode (QQuickWidget::SizeRootObjectToView);
#endif

		new Util::StandardNAMFactory ("lmp/qml",
				[] { return 50 * 1024 * 1024; },
				ReleasesView_->engine ());

		ReleasesView_->engine ()->addImageProvider ("ThemeIcons",
				new Util::ThemeImageProvider (Core::Instance ().GetProxy ()));
		ReleasesView_->rootContext ()->setContextProperty ("releasesModel", ReleasesModel_);
		ReleasesView_->rootContext ()->setContextProperty ("colorProxy",
				new Util::ColorThemeProxy (Core::Instance ().GetProxy ()->GetColorThemeManager (), this));
		ReleasesView_->setSource (Util::GetSysPathUrl (Util::SysPath::QML, "lmp", "ReleasesView.qml"));

		connect (Ui_.InfoProvider_,
				SIGNAL (activated (int)),
				this,
				SLOT (request ()));
		connect (Ui_.WithRecs_,
				SIGNAL (toggled (bool)),
				this,
				SLOT (request ()));

		connect (ReleasesView_->rootObject (),
				SIGNAL (linkActivated (QString)),
				this,
				SLOT (handleLink (QString)));
		connect (ReleasesView_->rootObject (),
				SIGNAL (albumPreviewRequested (int)),
				this,
				SLOT (previewAlbum (int)));
	}

	void ReleasesWidget::InitializeProviders ()
	{
		auto pm = Core::Instance ().GetProxy ()->GetPluginsManager ();

		const auto& lastProv = ShouldRememberProvs () ?
				XmlSettingsManager::Instance ()
					.Property ("LastUsedReleasesProvider", QString ()).toString () :
				QString ();

		bool lastFound = false;

		const auto& objList = pm->GetAllCastableRoots<Media::IRecentReleases*> ();
		for (auto provObj : objList)
		{
			const auto prov = qobject_cast<Media::IRecentReleases*> (provObj);
			Providers_ << prov;

			Ui_.InfoProvider_->addItem (qobject_cast<IInfo*> (provObj)->GetIcon (),
					prov->GetServiceName ());

			if (prov->GetServiceName () == lastProv)
			{
				const int idx = Providers_.size () - 1;
				Ui_.InfoProvider_->setCurrentIndex (idx);
				request ();
				lastFound = true;
			}
		}

		if (!lastFound)
			Ui_.InfoProvider_->setCurrentIndex (-1);

		DiscoProviders_ = pm->GetAllCastableTo<Media::IDiscographyProvider*> ();
	}

	void ReleasesWidget::handleRecentReleases (const QList<Media::AlbumRelease>& releases)
	{
		TrackLists_.resize (releases.size ());

		auto discoProv = DiscoProviders_.value (0);
		Q_FOREACH (const auto& release, releases)
		{
			auto item = new QStandardItem ();
			item->setData (release.Title_, ReleasesModel::Role::AlbumName);
			item->setData (release.Artist_, ReleasesModel::Role::ArtistName);
			item->setData (release.ThumbImage_, ReleasesModel::Role::AlbumImageThumb);
			item->setData (release.FullImage_, ReleasesModel::Role::AlbumImageFull);
			item->setData (release.Date_.date ().toString (Qt::DefaultLocaleLongDate),
						ReleasesModel::Role::ReleaseDate);
			item->setData (release.ReleaseURL_, ReleasesModel::Role::ReleaseURL);
			item->setData (QString (), ReleasesModel::Role::TrackList);
			ReleasesModel_->appendRow (item);

			if (discoProv)
			{
				auto pending = discoProv->GetReleaseInfo (release.Artist_, release.Title_);
				connect (pending->GetQObject (),
						SIGNAL (ready ()),
						this,
						SLOT (handleReleaseInfo ()));
				Pending2Release_ [pending->GetQObject ()] = { release.Artist_, release.Title_ };
			}
		}
	}

	void ReleasesWidget::handleReleaseInfo ()
	{
		const auto& pendingRelease = Pending2Release_.take (sender ());

		auto pending = qobject_cast<Media::IPendingDisco*> (sender ());
		const auto& infos = pending->GetReleases ();
		if (infos.isEmpty ())
			return;

		const auto& info = infos.at (0);

		QStandardItem *item = 0;
		for (int i = 0; i < ReleasesModel_->rowCount (); ++i)
		{
			auto candidate = ReleasesModel_->item (i);
			if (candidate->data (ReleasesModel::Role::ArtistName) != pendingRelease.Artist_ ||
				candidate->data (ReleasesModel::Role::AlbumName) != pendingRelease.Title_)
				continue;

			item = candidate;
			QList<Media::ReleaseTrackInfo> trackList;
			for (const auto& list : info.TrackInfos_)
				trackList += list;
			TrackLists_ [i] = trackList;
			break;
		}

		if (!item)
		{
			qWarning () << Q_FUNC_INFO
					<< "item not found for"
					<< pendingRelease.Artist_
					<< pendingRelease.Title_;
			return;
		}

		item->setData (MakeTrackListTooltip (info.TrackInfos_), ReleasesModel::Role::TrackList);
	}

	void ReleasesWidget::request ()
	{
		Pending2Release_.clear ();
		TrackLists_.clear ();
		ReleasesModel_->clear ();

		const auto idx = Ui_.InfoProvider_->currentIndex ();
		if (idx < 0)
			return;

		Q_FOREACH (auto prov, Providers_)
			disconnect (dynamic_cast<QObject*> (prov),
					0,
					this,
					0);

		const bool withRecs = Ui_.WithRecs_->checkState () == Qt::Checked;
		auto prov = Providers_.at (idx);
		connect (dynamic_cast<QObject*> (prov),
				SIGNAL (gotRecentReleases (QList<Media::AlbumRelease>)),
				this,
				SLOT (handleRecentReleases (const QList<Media::AlbumRelease>&)));
		prov->RequestRecentReleases (15, withRecs);

		XmlSettingsManager::Instance ()
				.setProperty ("LastUsedReleasesProvider", prov->GetServiceName ());
	}

	void ReleasesWidget::previewAlbum (int index)
	{
		auto item = ReleasesModel_->item (index);
		const auto& artist = item->data (ReleasesModel::Role::ArtistName).toString ();
		for (const auto& track : TrackLists_.value (index))
			emit previewRequested (track.Name_, artist, track.Length_);
	}

	void ReleasesWidget::handleLink (const QString& link)
	{
		Core::Instance ().SendEntity (Util::MakeEntity (QUrl (link),
					QString (),
					FromUserInitiated | OnlyHandle));
	}
}
}
