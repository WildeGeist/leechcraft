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

#include "documentbookmarksmanager.h"
#include <QStandardItemModel>
#include "core.h"
#include "bookmarksmanager.h"
#include "bookmark.h"
#include "documenttab.h"

namespace LeechCraft
{
namespace Monocle
{
	namespace
	{
		enum Roles
		{
			RBookmark = Qt::UserRole + 1
		};
	}

	DocumentBookmarksManager::DocumentBookmarksManager (DocumentTab *tab, QObject *parent)
	: QObject { parent }
	, Tab_ { tab }
	, Model_ { new QStandardItemModel { this } }
	{
	}

	QAbstractItemModel* DocumentBookmarksManager::GetModel () const
	{
		return Model_;
	}

	bool DocumentBookmarksManager::HasDoc () const
	{
		return Doc_ != nullptr;
	}

	void DocumentBookmarksManager::HandleDoc (IDocument_ptr doc)
	{
		Doc_ = doc;
		ReloadBookmarks ();
		emit docAvailable (HasDoc ());
	}

	void DocumentBookmarksManager::AddBookmark ()
	{
		if (!Doc_)
			return;

		const auto page = Tab_->GetCurrentPage ();
		const auto& center = Tab_->GetCurrentCenter ();

		Bookmark bm (tr ("Page %1").arg (page + 1), page, center);
		Core::Instance ().GetBookmarksManager ()->AddBookmark (Doc_, bm);
		AddBMToTree (bm);
	}

	void DocumentBookmarksManager::RemoveBookmark (QModelIndex idx)
	{
		if (!idx.isValid ())
			return;

		idx = idx.sibling (idx.row (), 0);
		const auto& bm = idx.data (Roles::RBookmark).value<Bookmark> ();
		Core::Instance ().GetBookmarksManager ()->RemoveBookmark (Doc_, bm);

		ReloadBookmarks ();
	}

	void DocumentBookmarksManager::Navigate (const QModelIndex& idx)
	{
		const auto& bm = idx.sibling (idx.row (), 0).data (Roles::RBookmark).value<Bookmark> ();
		Tab_->CenterOn (bm.GetPosition ());
	}

	void DocumentBookmarksManager::AddBMToTree (const Bookmark& bm)
	{
		auto item = new QStandardItem (bm.GetName ());
		item->setEditable (false);
		item->setData (QVariant::fromValue<Bookmark> (bm), Roles::RBookmark);
		Model_->appendRow (item);
	}

	void DocumentBookmarksManager::ReloadBookmarks ()
	{
		Model_->clear ();
		Model_->setHorizontalHeaderLabels ({ tr ("Name") });

		if (!Doc_)
			return;

		for (const auto& bm : Core::Instance ().GetBookmarksManager ()->GetBookmarks (Doc_))
			AddBMToTree (bm);
	}
}
}
