/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2014  Georg Rudoy
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE or copy at https://www.boost.org/LICENSE_1_0.txt)
 **********************************************************************/

#include "textdocumentadapter.h"
#include <cmath>
#include <QTextDocument>
#include <QTextBlock>
#include <QTextEdit>
#include <QtDebug>
#include <util/threads/futures.h>
#include <util/sll/views.h>
#include <util/sll/prelude.h>

namespace LC
{
namespace Monocle
{
	TextDocumentAdapter::TextDocumentAdapter (const std::shared_ptr<QTextDocument>& doc)
	{
		SetDocument (doc);
	}

	bool TextDocumentAdapter::IsValid () const
	{
		return static_cast<bool> (Doc_);
	}

	int TextDocumentAdapter::GetNumPages () const
	{
		return Doc_->pageCount ();
	}

	QSize TextDocumentAdapter::GetPageSize (int) const
	{
		auto size = Doc_->pageSize ();
		size.setWidth (std::ceil (size.width ()));
		return size.toSize ();
	}

	QFuture<QImage> TextDocumentAdapter::RenderPage (int page, double xScale, double yScale)
	{
		const auto& size = Doc_->pageSize ();

		auto imgSize = size.toSize ();
		imgSize.rwidth () *= xScale;
		imgSize.rheight () *= yScale;
		QImage image (imgSize, QImage::Format_ARGB32);
		image.fill (Qt::white);

		QRectF rect (QPointF (0, 0), size);
		rect.moveTop (rect.height () * page);

		QPainter painter;
		painter.begin (&image);
		painter.setRenderHints (Hints_);
		painter.scale (xScale, yScale);
		painter.translate (0, rect.height () * (-page));
		Doc_->drawContents (&painter, rect);
		painter.end ();

		return Util::MakeReadyFuture (image);
	}

	QList<ILink_ptr> TextDocumentAdapter::GetPageLinks (int page)
	{
		return Links_.value (page);
	}

	void TextDocumentAdapter::PaintPage (QPainter *painter, int page, double xScale, double yScale)
	{
		painter->save ();
		const auto oldHints = painter->renderHints ();
		painter->setRenderHints (Hints_);

		painter->scale (xScale, yScale);

		const auto& size = Doc_->pageSize ();

		QRectF rect (QPointF (0, 0), size);
		rect.moveTop (rect.height () * page);
		Doc_->drawContents (painter, rect);

		painter->setRenderHints (oldHints);
		painter->restore ();
	}

	void TextDocumentAdapter::SetRenderHint (QPainter::RenderHint hint, bool enable)
	{
		if (enable)
			Hints_ |= hint;
		else
			Hints_ &= ~hint;
	}

	namespace
	{
		auto GetCursorsPositions (QTextDocument *doc, const QList<QPair<QTextCursor, QTextCursor>>& cursors)
		{
			const auto& pageSize = doc->pageSize ();
			const auto pageHeight = pageSize.height ();

			QTextEdit hackyEdit;
			hackyEdit.setHorizontalScrollBarPolicy (Qt::ScrollBarAlwaysOff);
			hackyEdit.setVerticalScrollBarPolicy (Qt::ScrollBarAlwaysOff);
			hackyEdit.setFixedSize (doc->pageSize ().toSize ());
			hackyEdit.setDocument (doc);
			doc->setPageSize (pageSize);

			const QMatrix scale = QMatrix {}.scale (1 / pageSize.width (), 1 / pageSize.height ());

			QList<QPair<int, QRectF>> result;
			for (const auto& pair : cursors)
			{
				auto rect = hackyEdit.cursorRect (pair.first);
				auto endRect = hackyEdit.cursorRect (pair.second);

				const int pageNum = rect.y () / pageHeight;
				rect.moveTop (rect.y () - pageHeight * pageNum);
				endRect.moveTop (endRect.y () - pageHeight * pageNum);

				if (rect.y () != endRect.y ())
				{
					rect.setWidth (pageSize.width () - rect.x ());
					endRect.setX (0);
				}
				QRectF bounding { rect | endRect };

				bounding = scale.mapRect (bounding);

				result << QPair { pageNum, bounding };
			}
			return result;
		}

		QMap<int, QList<QRectF>> ListToMap (const QList<QPair<int, QRectF>>& list)
		{
			QMap<int, QList<QRectF>> result;
			for (const auto& [page, rect] : list)
				result [page] << rect;
			return result;
		}
	}

	QMap<int, QList<QRectF>> TextDocumentAdapter::GetTextPositions (const QString& text, Qt::CaseSensitivity cs)
	{
		QList<QPair<QTextCursor, QTextCursor>> cursors;

		const auto tdFlags = cs == Qt::CaseSensitive ?
				QTextDocument::FindCaseSensitively :
				QTextDocument::FindFlags ();
		auto cursor = Doc_->find (text, 0, tdFlags);
		while (!cursor.isNull ())
		{
			auto startCursor = cursor;
			startCursor.setPosition (cursor.selectionStart ());
			cursors << QPair { startCursor, cursor };
			cursor = Doc_->find (text, cursor, tdFlags);
		}

		return ListToMap (GetCursorsPositions (Doc_.get (), cursors));
	}

	namespace
	{
		class Link : public ILink
				   , public IPageLink
		{
			const QRectF LinkArea_;

			const int TargetPage_;
			const QRectF TargetArea_;

			IDocument * const Doc_;
		public:
			Link (const QRectF& area, int targetPage, const QRectF& targetArea, IDocument *doc)
			: LinkArea_ { area }
			, TargetPage_ { targetPage }
			, TargetArea_ { targetArea }
			, Doc_ { doc }
			{
			}

			LinkType GetLinkType () const override
			{
				return LinkType::PageLink;
			}

			QRectF GetArea () const override
			{
				return LinkArea_;
			}

			void Execute () override
			{
				Doc_->navigateRequested ({}, { TargetPage_, TargetArea_.topLeft () });
			}

			QString GetDocumentFilename () const override
			{
				return {};
			}

			int GetPageNumber () const override
			{
				return TargetPage_;
			}

			double NewX () const override
			{
				return TargetArea_.left ();
			}

			double NewY () const override
			{
				return TargetArea_.top ();
			}

			double NewZoom () const override
			{
				return 0;
			}
		};
	}

	void TextDocumentAdapter::SetDocument (const std::shared_ptr<QTextDocument>& doc, const QList<InternalLink>& links)
	{
		Doc_ = doc;
		Links_.clear ();
		if (!Doc_ || links.isEmpty ())
			return;

		const auto& srcCursors = Util::Map (links,
				[this] (const InternalLink& link)
				{
					QTextCursor start { Doc_.get () };
					start.setPosition (link.FromSpan_.first);
					QTextCursor end { Doc_.get () };
					end.setPosition (link.FromSpan_.second);
					return QPair { start, end };
				});
		const auto& dstCursors = Util::Map (links,
				[this] (const InternalLink& link)
				{
					QTextCursor start { Doc_.get () };
					start.setPosition (link.ToSpan_.first);
					QTextCursor end { Doc_.get () };
					end.setPosition (link.ToSpan_.second);
					return QPair { start, end };
				});
		const auto& srcPositions = GetCursorsPositions (Doc_.get (), srcCursors);
		const auto& dstPositions = GetCursorsPositions (Doc_.get (), dstCursors);

		for (const auto& [srcPos, dstPos] : Util::Views::Zip (srcPositions, dstPositions))
			Links_ [srcPos.first] << std::make_shared<Link> (srcPos.second, dstPos.first, dstPos.second, this);
	}
}
}
