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

#pragma once

#include <memory>
#include <QWidget>
#include <interfaces/ihavetabs.h>
#include <interfaces/media/audiostructs.h>
#include <interfaces/ihaverecoverabletabs.h>
#include "lmpsystemtrayicon.h"
#include "player.h"
#include "ui_playertab.h"

class QStandardItemModel;
class QListWidget;
class QTabBar;

namespace Media
{
	struct LyricsResults;
}

namespace LeechCraft
{
struct Entity;

namespace LMP
{
	struct MediaInfo;
	class Player;
	class NowPlayingPixmapHandler;
	class PreviewHandler;

	class PlayerTab : public QWidget
					, public ITabWidget
					, public IRecoverableTab
	{
		Q_OBJECT
		Q_INTERFACES (ITabWidget IRecoverableTab)

		Ui::PlayerTab Ui_;

		QObject *Plugin_;
		const TabClassInfo TC_;

		Player *Player_;
		PreviewHandler *PreviewHandler_;

		QToolBar *TabToolbar_;

		QHash<QString, Media::SimilarityInfos_t> Similars_;
		QString LastArtist_;

		LMPSystemTrayIcon *TrayIcon_;
		QAction *PlayPause_;
		QMenu *TrayMenu_;

		QListWidget *NavButtons_;
		QTabBar *NavBar_;

		NowPlayingPixmapHandler *NPPixmapHandler_;

		QMenu * const EffectsMenu_;
	public:
		PlayerTab (const TabClassInfo&, Player*, const ICoreProxy_ptr&, QObject*, QWidget* = 0);
		~PlayerTab ();

		TabClassInfo GetTabClassInfo () const;
		QObject* ParentMultiTabs ();
		void Remove ();
		QToolBar* GetToolBar () const;

		Player* GetPlayer () const;

		QByteArray GetTabRecoverData () const;
		QIcon GetTabRecoverIcon () const;
		QString GetTabRecoverName () const;

		void AddNPTab (const QString&, QWidget*);

		void InitWithOtherPlugins ();
	private:
		void SetupNavButtons ();
		void SetupToolbar ();
		void Scrobble (const MediaInfo&);
		void FillSimilar (const Media::SimilarityInfos_t&);
		void RequestLyrics (const MediaInfo&);
	public slots:
		void updateEffectsList (const QStringList&);
	private slots:
		void handleSongChanged (const MediaInfo&);
		void handleLoveTrack ();
		void handleBanTrack ();

		void handleSimilarError ();
		void handleSimilarReady ();

		void handlePlayerAvailable (bool);

		void closeLMP ();
		void handleStateChanged ();
		void handleShowTrayIcon ();
		void handleUseNavTabBar ();
		void handleChangedVolume (qreal delta);
		void handleTrayIconActivated (QSystemTrayIcon::ActivationReason reason);
	signals:
		void changeTabName (QWidget*, const QString&);
		void removeTab (QWidget*);
		void raiseTab (QWidget*);

		void fullRaiseRequested ();

		void gotEntity (const LeechCraft::Entity&);

		void tabRecoverDataChanged ();

		void effectsConfigRequested (int);

		// Internal signal.
		void notifyCurrentTrackRequested ();
	};
}
}
