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

#include "annmanager.h"
#include <QGraphicsScene>
#include <QStandardItemModel>
#include <QtDebug>
#include "interfaces/monocle/isupportannotations.h"
#include "interfaces/monocle/iannotation.h"
#include "pagegraphicsitem.h"
#include "annitem.h"
#include "pagesview.h"

namespace LeechCraft
{
namespace Monocle
{
	AnnManager::AnnManager (PagesView *view, QObject *parent)
	: QObject { parent }
	, View_ { view }
	, Scene_ { view->scene () }
	, AnnModel_ { new QStandardItemModel { this } }
	{
	}

	void AnnManager::HandleDoc (IDocument_ptr doc, const QList<PageGraphicsItem*>& pages)
	{
		if (const auto rc = AnnModel_->rowCount ())
			AnnModel_->removeRows (0, rc);

		Ann2Item_.clear ();
		Ann2GraphicsItem_.clear ();
		Annotations_.clear ();
		CurrentAnn_ = -1;

		const auto isa = qobject_cast<ISupportAnnotations*> (doc->GetQObject ());
		if (!isa)
			return;

		for (auto page : pages)
		{
			QStandardItem *pageItem = nullptr;
			auto createItem = [&pageItem, page, this] () -> void
			{
				if (pageItem)
					return;

				pageItem = new QStandardItem (tr ("Page %1")
							.arg (page->GetPageNum () + 1));
				pageItem->setData (ItemTypes::PageItem, Role::ItemType);
				pageItem->setEditable (false);
				AnnModel_->appendRow (pageItem);
			};

			for (const auto& ann : isa->GetAnnotations (page->GetPageNum ()))
			{
				const auto item = MakeItem (ann, page);
				if (!item)
				{
					qWarning () << Q_FUNC_INFO
							<< "unhandled annotation type"
							<< static_cast<int> (ann->GetAnnotationType ());
					continue;
				}

				Annotations_ << ann;

				Ann2GraphicsItem_ [ann] = item;

				item->SetHandler ([this] (const IAnnotation_ptr& ann) -> void
						{
							EmitSelected (ann);
							SelectAnnotation (ann);
						});

				const auto& docRect = page->MapToDoc (page->boundingRect ());
				const auto& rect = ann->GetBoundary ();

				QRectF targetRect (rect.x () * docRect.width (),
						rect.y () * docRect.height (),
						rect.width () * docRect.width (),
						rect.height () * docRect.height ());

				page->RegisterChildRect (item->GetItem (), targetRect,
						[item] (const QRectF& rect) { item->UpdateRect (rect); });

				auto annItem = new QStandardItem (ann->GetText ());
				annItem->setToolTip (ann->GetText ());
				annItem->setEditable (false);
				annItem->setData (QVariant::fromValue (ann), Role::Annotation);
				annItem->setData (ItemTypes::AnnHeaderItem, Role::ItemType);

				auto subItem = new QStandardItem (ann->GetText ());
				subItem->setToolTip (ann->GetText ());
				subItem->setEditable (false);
				subItem->setData (QVariant::fromValue (ann), Role::Annotation);
				subItem->setData (ItemTypes::AnnItem, Role::ItemType);

				Ann2Item_ [ann] = subItem;

				annItem->appendRow (subItem);
				createItem ();
				pageItem->appendRow (annItem);
			}
		}
	}

	QAbstractItemModel* AnnManager::GetModel () const
	{
		return AnnModel_;
	}

	void AnnManager::EmitSelected (const IAnnotation_ptr& ann)
	{
		if (const auto item = Ann2Item_ [ann])
			emit annotationSelected (item->index ());
	}

	void AnnManager::CenterOn (const IAnnotation_ptr& ann)
	{
		const auto item = Ann2GraphicsItem_.value (ann);
		if (!item)
			return;

		const auto graphicsItem = item->GetItem ();
		const auto& mapped = graphicsItem->scenePos ();
		View_->SmoothCenterOn (mapped.x (), mapped.y ());
	}

	void AnnManager::SelectAnnotation (const IAnnotation_ptr& ann)
	{
		const auto modelItem = Ann2Item_ [ann];
		if (!modelItem)
			return;

		const auto graphicsItem = Ann2GraphicsItem_ [ann];
		if (graphicsItem->IsSelected ())
			return;

		for (const auto& item : Ann2GraphicsItem_)
			if (item->IsSelected ())
			{
				item->SetSelected (false);
				break;
			}

		graphicsItem->SetSelected (true);
		CurrentAnn_ = Annotations_.indexOf (ann);
	}

	void AnnManager::selectPrev ()
	{
		if (CurrentAnn_ == -1 || Annotations_.size () < 2)
			return;

		if (--CurrentAnn_ < 0)
			CurrentAnn_ = Annotations_.size () - 1;

		const auto& ann = Annotations_.at (CurrentAnn_);

		EmitSelected (ann);
		CenterOn (ann);
		SelectAnnotation (ann);
	}

	void AnnManager::selectNext ()
	{
		if (CurrentAnn_ == -1 || Annotations_.size () < 2)
			return;

		if (++CurrentAnn_ >= Annotations_.size ())
			CurrentAnn_ = 0;

		const auto& ann = Annotations_.at (CurrentAnn_);

		EmitSelected (ann);
		CenterOn (ann);
		SelectAnnotation (ann);
	}

	void AnnManager::selectAnnotation (const QModelIndex& idx)
	{
		if (idx.data (Role::ItemType).toInt () != ItemTypes::AnnItem)
			return;

		const auto& ann = idx.data (Role::Annotation).value<IAnnotation_ptr> ();
		if (!ann)
			return;

		CenterOn (ann);

		if (Annotations_.indexOf (ann) == CurrentAnn_)
			return;

		SelectAnnotation (ann);
	}
}
}
