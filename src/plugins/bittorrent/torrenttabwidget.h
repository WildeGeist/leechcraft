/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2013  Georg Rudoy
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

#include <QTabWidget>
#include <QAction>
#include "ui_torrenttabwidget.h"

class QSortFilterProxyModel;

namespace LeechCraft
{
namespace Util
{
	class TagsCompleter;
}
namespace Plugins
{
namespace BitTorrent
{
	class TorrentTabWidget : public QTabWidget
	{
		Q_OBJECT

		Ui::TorrentTabWidget Ui_;
		Util::TagsCompleter *TagsChangeCompleter_;
		QAction *AddPeer_;
		QAction *BanPeer_;
		QAction *AddWebSeed_;
		QAction *RemoveWebSeed_;
		int Index_;

		QSortFilterProxyModel *PeersSorter_;
	public:
		TorrentTabWidget (QWidget* = 0);

		void SetChangeTrackersAction (QAction*);

		void SetCurrentIndex (int);
		void InvalidateSelection ();
		void SetOverallDownloadRateController (int);
		void SetOverallUploadRateController (int);
	public slots:
		void updateTorrentStats ();
	private:
		void UpdateDashboard ();
		void UpdateOverallStats ();
		void UpdateTorrentControl ();
		void UpdateFilesPage ();
	private slots:
		void on_OverallDownloadRateController__valueChanged (int);
		void on_OverallUploadRateController__valueChanged (int);
		void on_TorrentDownloadRateController__valueChanged (int);
		void on_TorrentUploadRateController__valueChanged (int);
		void on_TorrentManaged__stateChanged (int);
		void on_TorrentSequentialDownload__stateChanged (int);
		void on_TorrentSuperSeeding__stateChanged (int);
		void on_DownloadingTorrents__valueChanged (int);
		void on_UploadingTorrents__valueChanged (int);
		void on_TorrentTags__editingFinished ();
		void setTabWidgetSettings ();
		void currentFileChanged (const QModelIndex&);
		void on_FilePriorityRegulator__valueChanged (int);
		void handleAddPeer ();
		void handleBanPeer ();
		void handleAddWebSeed ();
		void currentPeerChanged (const QModelIndex&);
		void currentWebSeedChanged (const QModelIndex&);
		void handleRemoveWebSeed ();
		void handleFileActivated (const QModelIndex&);
	};
}
}
}
