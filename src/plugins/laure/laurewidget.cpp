/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2011 Minh Ngo
 * Copyright (C) 2006-2011  Georg Rudoy
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 **********************************************************************/

#include "laurewidget.h"
#include <QAction>
#include <QFileDialog>
#include <QDebug>
#include <QMessageBox>
#include <QToolBar>
#include <QUrl>
#include <QTime>
#include <QKeyEvent>
#include <QTimer>
#include <interfaces/core/icoreproxy.h>
#include "laure.h"
#include "playpauseaction.h"
#include "chooseurldialog.h"
#include "playlistview.h"
#include "player.h"
#include "playlistwidget.h"
#include "xmlsettingsmanager.h"

namespace LeechCraft
{
namespace Laure
{
	QObject *LaureWidget::S_ParentMultiTabs_ = 0;
	
	LaureWidget::LaureWidget (QWidget *parent, Qt::WindowFlags f)
	: QWidget (parent, f)
	, ToolBar_ (new QToolBar (this))
	, Core_ (new Core (this))
	{	
		Ui_.setupUi (this);
		Ui_.Player_->setCore (Core_);
		Core_->setWindow (Ui_.Player_);
		
		connect (Ui_.Player_,
				SIGNAL (timeout ()),
				this,
				SLOT (updateInterface ()));
		connect (Ui_.PositionSlider_,
				SIGNAL (sliderMoved (int)),
				Ui_.Player_,
				SLOT (setPosition (int)));
		connect (Ui_.VolumeSlider_,
				SIGNAL (sliderMoved (int)),
				Core_,
				SLOT (setVolume (int)));
		connect (Core_,
				SIGNAL (nowPlayed (MediaMeta)),
				this,
				SIGNAL (nowPlayed (MediaMeta)));
		connect (Core_,
				SIGNAL (played ()),
				this,
				SIGNAL (played ()));
		connect (Ui_.PlayListWidget_,
				SIGNAL (itemAddedRequest (QString)),
				Core_,
				SLOT (addRow (QString)));
		connect (Core_,
				SIGNAL (itemAdded (MediaMeta)),
				Ui_.PlayListWidget_,
				SLOT (handleItemAdded (MediaMeta)));
		connect (Ui_.PlayListWidget_,
				SIGNAL (itemRemoved (int)),
				Core_,
				SLOT (removeRow (int)));
		connect (Ui_.PlayListWidget_,
				SIGNAL (playItem (int)),
				Core_,
				SLOT (playItem (int)));
		connect (Core_,
				SIGNAL (itemPlayed (int)),
				Ui_.PlayListWidget_,
				SLOT (handleItemPlayed (int)));
		connect (this,
				SIGNAL (addItem (QString)),
				Core_,
				SLOT (addRow (QString)));
	}
	
	void LaureWidget::Init (ICoreProxy_ptr proxy)
	{
		InitToolBar ();
		InitCommandFrame ();
	}
	
	void LaureWidget::InitToolBar ()
	{
		QAction *actionOpenFile = new QAction (tr ("Open File"), this);
		QAction *actionOpenURL = new QAction (tr ("Open URL"), this);
		QAction *playList = new QAction (tr ("Playlist"), this);
		
		actionOpenFile->setProperty ("ActionIcon", "folder");
		actionOpenURL->setProperty ("ActionIcon", "networkmonitor_plugin");
		playList->setProperty ("ActionIcon", "itemlist");
		
		playList->setCheckable (true);
		
		ToolBar_->addAction (actionOpenFile);
		ToolBar_->addAction (actionOpenURL);
		ToolBar_->addAction (playList);
		
		connect (actionOpenFile,
				SIGNAL (triggered (bool)),
				this,
				SLOT (handleOpenFile ()));
		connect (actionOpenURL,
				SIGNAL (triggered (bool)),
				this,
				SLOT (handleOpenURL ()));
		connect (playList,
				SIGNAL (triggered (bool)),
				this,
				SLOT (handlePlaylist ()));
	}
	
	void LaureWidget::InitCommandFrame ()
	{
		QToolBar *bar = new QToolBar (Ui_.CommandFrame_);
		bar->setToolButtonStyle (Qt::ToolButtonIconOnly);
		bar->setIconSize (QSize (32, 32));
		
		PlayPauseAction *actionPlay = new PlayPauseAction (tr ("Play"), Ui_.CommandFrame_);
	
		QAction *actionStop = new QAction (tr ("Stop"), Ui_.CommandFrame_);
		QAction *actionNext = new QAction (tr ("Next"), Ui_.CommandFrame_);
		QAction *actionPrev = new QAction (tr ("Previous"), Ui_.CommandFrame_);
		
		actionStop->setProperty ("ActionIcon", "media_stop");
		actionNext->setProperty ("ActionIcon", "media_skip_forward");
		actionPrev->setProperty ("ActionIcon", "media_skip_backward");
		
		bar->addAction (actionPrev);
		bar->addAction (actionPlay);
		bar->addAction (actionStop);
		bar->addAction (actionNext);
		
		connect (Core_,
				SIGNAL (paused ()),
				actionPlay,
				SLOT (handlePause ()));
		connect (Core_,
				SIGNAL (itemPlayed (int)),
				actionPlay,
				SLOT (handlePlay ()));
		connect (actionStop,
				SIGNAL (triggered (bool)),
				actionPlay,
				SLOT (handlePause ()));
		connect (actionStop,
				SIGNAL (triggered (bool)),
				Core_,
				SLOT (stop ()));
		connect (actionNext,
				SIGNAL (triggered (bool)),
				Core_,
				SLOT (next ()));
		connect (actionPrev,
				SIGNAL (triggered (bool)),
				Core_,
				SLOT (prev ()));
		connect (actionPlay,
				SIGNAL (play ()),
				Core_,
				SLOT (play ()));
		connect (actionPlay,
				SIGNAL (pause ()),
				Core_,
				SLOT (pause ()));
		connect (this,
				SIGNAL (playPause ()),
				actionPlay,
				SLOT (handleTriggered ()));
	}
	
	void LaureWidget::updateInterface ()
	{
		Ui_.VolumeSlider_->setValue (Core_->Volume ());
		Ui_.PositionSlider_->setValue (Ui_.Player_->Position ());
		const QTime& currTime = Ui_.Player_->Time ();
		const QTime& length = Ui_.Player_->Length ();
		Ui_.TimeStamp_->setText ("[" + currTime.toString () + "/" + length.toString () + "]");
	}
	
	void LaureWidget::SetParentMultiTabs (QObject* parent)
	{
		S_ParentMultiTabs_ = parent;
	}

	TabClassInfo LaureWidget::GetTabClassInfo () const
	{
		return qobject_cast<Plugin *> (S_ParentMultiTabs_)->GetTabClasses ().first ();
	}
	
	QObject* LaureWidget::ParentMultiTabs ()
	{
		return S_ParentMultiTabs_;
	}
	
	void LaureWidget::Remove ()
	{
		emit needToClose ();
	}
	
	QToolBar* LaureWidget::GetToolBar () const
	{
		return ToolBar_;
	}
	
	void LaureWidget::keyPressEvent (QKeyEvent *event)
	{
		switch (event->key ())
		{
		case Qt::Key_Space:
			emit playPause ();
		}
	}
	
	void LaureWidget::handleOpenURL ()
	{
		ChooseURLDialog *dialog = new ChooseURLDialog (this);
		if (dialog->exec () == QDialog::Accepted)
		{
			if (dialog->IsUrlValid ())
				emit addItem (dialog->GetUrl ());
			else
				QMessageBox::warning (this,
						tr ("The URL's not valid"),
						tr ("The URL's not valid"));
		}
	}
	
	void LaureWidget::handleOpenMediaContent (const QString& val)
	{
		emit addItem (val);
	}
	
	void LaureWidget::handleOpenFile ()
	{
		const QString& fileName = QFileDialog::getOpenFileName (this,
				tr ("Choose file"), QDir::homePath ());
		if (!fileName.isEmpty ())
			emit addItem (fileName);
	}
	
	void LaureWidget::handlePlaylist ()
	{
		Ui_.PlayListWidget_->setVisible (qobject_cast<QAction *> (sender ())->isChecked ());
	}
}
}