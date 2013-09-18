/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2013  Vladislav Tyulbashev
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

#include "playlistmodel.h"
#include <QModelIndex>
#include <QVariant>
#include <QTime>
#include <QTimer>
#include <QMimeData>
#include <QDebug>
#include <QItemSelectionModel>
#include <QUrl>
#include <vlc/vlc.h>
#include "playlistwidget.h"

namespace LeechCraft
{
namespace vlc
{
	PlaylistModel::PlaylistModel (PlaylistWidget *parent, libvlc_media_list_t *playlist)
	: QStandardItemModel (parent)
	, Parent_ (parent)
	, Playlist_ (playlist)
	{
		setColumnCount (2);
		setHorizontalHeaderLabels ({ tr ("Name"), tr ("Duration") });
		setSupportedDragActions (Qt::MoveAction | Qt::CopyAction);
	}
	
	PlaylistModel::~PlaylistModel ()
	{
		const int size = Items_ [0].size ();
		for (int i = 0; i < size; i++)
			libvlc_media_release (libvlc_media_list_item_at_index (Playlist_, i));
		
		setRowCount (0);
	}
	
	void PlaylistModel::updateTable ()
	{
		setRowCount (libvlc_media_list_count (Playlist_));
		if (libvlc_media_list_count (Playlist_) != Items_ [0].size ())
		{
			int cnt = Items_ [ColumnName].size ();
			Items_ [ColumnName].resize (libvlc_media_list_count (Playlist_));
			Items_ [ColumnDuration].resize (libvlc_media_list_count (Playlist_));
			
			for (int i = cnt; i < Items_ [0].size (); i++)
			{
				Items_ [ColumnName] [i] = new QStandardItem;
				Items_ [ColumnName] [i]->setFlags (Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
				Items_ [ColumnDuration] [i] = new QStandardItem;
				Items_ [ColumnDuration] [i]->setFlags (Qt::ItemIsDropEnabled | Qt::ItemIsEnabled);
			}
		}
		
		for (int i = 0; i < libvlc_media_list_count (Playlist_); i++)
		{
			libvlc_media_t *media = libvlc_media_list_item_at_index (Playlist_, i);
			Items_ [ColumnName] [i]->setText (QString::fromUtf8 (libvlc_media_get_meta (media, libvlc_meta_Title)));
			
			if (!libvlc_media_is_parsed (media))
				libvlc_media_parse (media);
				
			QTime time (0, 0);
			time = time.addMSecs (libvlc_media_get_duration (media));
			Items_ [ColumnDuration] [i]->setText (time.toString ("hh:mm:ss"));
		}
		
		for (int i = 0; i < libvlc_media_list_count (Playlist_); i++)
		{
			setItem (i, ColumnName, Items_ [ColumnName] [i]);
			setItem (i, ColumnDuration, Items_ [ColumnDuration] [i]);
		}
	}
	
	bool PlaylistModel::dropMimeData (const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex& parent)
	{
		const QList <QUrl>& urls = data->urls ();
		if (parent != invisibleRootItem ()->index ())
			row = parent.row () - 1;
		else	
			row -= 2;
		
		
		if (data->colorData ().toString () == "vtyulb")
		{	
			QUrl insertAfter;
			for (int i = row; i > 0; i--)
				if (!urls.contains (QUrl (libvlc_media_get_meta (libvlc_media_list_item_at_index (Playlist_, i), libvlc_meta_URL))))
				{
					insertAfter = QUrl (libvlc_media_get_meta (libvlc_media_list_item_at_index (Playlist_, i), libvlc_meta_URL));
					break;
				}
			
			QList<libvlc_media_t*> mediaList;
			for (int i = 0; i < urls.size (); i++)
				mediaList << FindAndDelete (urls [i]);
			
			int after;
			if (insertAfter.isEmpty ())
				after = -1;
			else
				for (int i = 0; i < libvlc_media_list_count (Playlist_); i++)
					if (QUrl (libvlc_media_get_meta (libvlc_media_list_item_at_index (Playlist_, i), libvlc_meta_URL)) == insertAfter)
					{
						after = i;
						break;
					}
					
			if (!parent.isValid () && (after == -1))  // VLC forever
				for (int i = 0; i < urls.size (); i++)
					libvlc_media_list_add_media (Playlist_, mediaList [i]);
			else
				for (int i = 0; i < urls.size (); i++)
					libvlc_media_list_insert_media (Playlist_, mediaList [i], after + i + 2);
			
			updateTable ();
		}
		else
			for (int i = 0; i < urls.size (); i++)
				AddUrl (urls [i]);
		
		return true;
	}
	
	QStringList PlaylistModel::mimeTypes () const
	{
		return QStringList ("text/uri-list");
	}
	
	QMimeData* PlaylistModel::mimeData (const QModelIndexList& indexes) const
	{
		if (libvlc_media_list_count (Playlist_) == 1)
			return nullptr;
		
		QMimeData *result = new QMimeData;
		QList<QUrl> urls;
		for (int i = 0; i < indexes.size (); i++)
			if (indexes [i].column () == 0)
				urls << QUrl (libvlc_media_get_meta (libvlc_media_list_item_at_index (Playlist_, indexes[i].row ()), libvlc_meta_URL));
		
		result->setUrls (urls);
		result->setColorData (QVariant ("vtyulb"));
		return result;
	}
	
	Qt::DropActions PlaylistModel::supportedDropActions () const
	{
		return Qt::MoveAction | Qt::CopyAction;
	}
		
	libvlc_media_t* PlaylistModel::FindAndDelete (QUrl url)
	{
		libvlc_media_t *res = nullptr;
		for (int i = 0; i < libvlc_media_list_count (Playlist_); i++)
			if (QUrl (libvlc_media_get_meta (libvlc_media_list_item_at_index (Playlist_, i), libvlc_meta_URL)) == url)
			{
				res = libvlc_media_list_item_at_index (Playlist_, i);
				libvlc_media_list_remove_index (Playlist_, i);
				break;
			}
			
		if (!res)
			qWarning () << Q_FUNC_INFO << "fatal";
		
		return res;
	}
	
	void PlaylistModel::AddUrl (const QUrl& url)
	{
		Parent_->AddUrl (url);
	}
}
}
