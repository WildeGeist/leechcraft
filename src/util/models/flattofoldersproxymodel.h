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

#ifndef UTIL_FLATTOFOLDERSPROXYMODEL_H
#define UTIL_FLATTOFOLDERSPROXYMODEL_H
#include <memory>
#include <QAbstractItemModel>
#include <QStringList>
#include <QMultiHash>
#include <util/utilconfig.h>

class ITagsManager;

namespace LeechCraft
{
	struct FlatTreeItem;
	typedef std::shared_ptr<FlatTreeItem> FlatTreeItem_ptr;

	namespace Util
	{
		class UTIL_API FlatToFoldersProxyModel : public QAbstractItemModel
		{
			Q_OBJECT

			QAbstractItemModel *SourceModel_;

			ITagsManager *TM_;

			FlatTreeItem_ptr Root_;
			QMultiHash<QPersistentModelIndex, FlatTreeItem_ptr> Items_;
		public:
			FlatToFoldersProxyModel (QObject* = 0);

			void SetTagsManager (ITagsManager*);

			int columnCount (const QModelIndex& = QModelIndex ()) const;
			QVariant data (const QModelIndex&, int = Qt::DisplayRole) const;
			QVariant headerData (int, Qt::Orientation, int) const;
			Qt::ItemFlags flags (const QModelIndex&) const;
			QModelIndex index (int, int, const QModelIndex& = QModelIndex ()) const;
			QModelIndex parent (const QModelIndex&) const;
			int rowCount (const QModelIndex& = QModelIndex ()) const;

			Qt::DropActions supportedDropActions () const;
			QStringList mimeTypes () const;
			QMimeData* mimeData (const QModelIndexList& indexes) const;
			bool dropMimeData (const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex& parent);

			void SetSourceModel (QAbstractItemModel*);
			QAbstractItemModel* GetSourceModel () const;
			QModelIndex MapToSource (const QModelIndex&) const;
			QList<QModelIndex> MapFromSource (const QModelIndex&) const;
		private:
			void HandleRowInserted (int);
			void HandleRowRemoved (int);
			void AddForTag (const QString&, const QPersistentModelIndex&);
			void RemoveFromTag (const QString&, const QPersistentModelIndex&);
			void HandleChanged (const QModelIndex&);
			FlatTreeItem_ptr GetFolder (const QString&);
		private slots:
			void handleDataChanged (const QModelIndex&, const QModelIndex&);
			void handleModelReset ();
			void handleRowsInserted (const QModelIndex&, int, int);
			void handleRowsAboutToBeRemoved (const QModelIndex&, int, int);
		};
	};
};

#endif

