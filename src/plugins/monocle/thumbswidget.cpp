/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2014  Georg Rudoy
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE or copy at https://www.boost.org/LICENSE_1_0.txt)
 **********************************************************************/

#include "thumbswidget.h"
#include <QtDebug>
#include "pageslayoutmanager.h"
#include "pagegraphicsitem.h"
#include "smoothscroller.h"
#include "common.h"

namespace LC
{
namespace Monocle
{
	ThumbsWidget::ThumbsWidget (QWidget *parent)
	: QWidget (parent)
	{
		Ui_.setupUi (this);
		Ui_.ThumbsView_->setScene (&Scene_);
		Ui_.ThumbsView_->setBackgroundBrush (palette ().brush (QPalette::Dark));

		auto scroller = new SmoothScroller { Ui_.ThumbsView_, this };

		LayoutMgr_ = new PagesLayoutManager (Ui_.ThumbsView_, scroller, this);
		LayoutMgr_->SetScaleMode (ScaleMode::FitWidth);
		LayoutMgr_->SetMargins (10, 0);

		connect (LayoutMgr_,
				SIGNAL (scheduledRelayoutFinished ()),
				this,
				SLOT (handleRelayouted ()));
	}

	void ThumbsWidget::HandleDoc (IDocument_ptr doc)
	{
		Scene_.clear ();
		CurrentAreaRects_.clear ();
		CurrentDoc_ = doc;

		if (!doc)
			return;

		QList<PageGraphicsItem*> pages;
		for (int i = 0, size = CurrentDoc_->GetNumPages (); i < size; ++i)
		{
			auto item = new PageGraphicsItem (CurrentDoc_, i);
			Scene_.addItem (item);
			item->SetReleaseHandler ([this] (int page, const QPointF&) { emit pageClicked (page); });
			pages << item;
		}

		LayoutMgr_->HandleDoc (CurrentDoc_, pages);
		LayoutMgr_->Relayout ();
	}

	void ThumbsWidget::updatePagesVisibility (const QMap<int, QRect>& page2rect)
	{
		LastVisibleAreas_ = page2rect;

		if (page2rect.size () != CurrentAreaRects_.size ())
		{
			for (auto rect : CurrentAreaRects_)
			{
				Scene_.removeItem (rect);
				delete rect;
			}
			CurrentAreaRects_.clear ();

			const auto& brush = palette ().brush (QPalette::Dark);
			for (int i = 0; i < page2rect.size (); ++i)
			{
				auto item = Scene_.addRect ({}, { Qt::black }, brush);
				item->setZValue (1);
				item->setOpacity (0.3);
				CurrentAreaRects_ << item;
			}
		}

		const auto& pages = LayoutMgr_->GetPages ();

		int rectIdx = 0;
		for (auto i = page2rect.begin (); i != page2rect.end (); ++i, ++rectIdx)
		{
			const auto pageNum = i.key ();
			if (pageNum >= pages.size ())
				continue;

			auto page = pages.at (pageNum);

			const auto& docRect = *i;
			const auto& sceneRect = page->mapToScene (page->MapFromDoc (docRect)).boundingRect ();
			CurrentAreaRects_ [rectIdx]->setRect (sceneRect);
		}
	}

	void ThumbsWidget::handleCurrentPage (int page)
	{
		LayoutMgr_->SetCurrentPage (page, false);
	}

	void ThumbsWidget::handleRelayouted ()
	{
		updatePagesVisibility (LastVisibleAreas_);
	}
}
}

