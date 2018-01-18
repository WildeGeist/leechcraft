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

#include "aggregatorapp.h"
#include <QObject>
#include <QThread>
#include <QtDebug>
#include <Wt/WText.h>
#include <Wt/WContainerWidget.h>
#include <Wt/WBoxLayout.h>
#include <Wt/WCheckBox.h>
#include <Wt/WTreeView.h>
#include <Wt/WTableView.h>
#include <Wt/WStandardItemModel.h>
#include <Wt/WStandardItem.h>
#include <Wt/WOverlayLoadingIndicator.h>
#include <Wt/WPanel.h>
#include <Wt/WPopupMenu.h>
#include <Wt/WCssTheme.h>
#include <Wt/WScrollArea.h>
#include <interfaces/aggregator/iproxyobject.h>
#include <interfaces/aggregator/channel.h>
#include <interfaces/aggregator/iitemsmodel.h>
#include <util/aggregator/itemsmodeldecorator.h>
#include "readchannelsfilter.h"
#include "util.h"
#include "q2wproxymodel.h"
#include "readitemsfilter.h"
#include "wf.h"

namespace LeechCraft
{
namespace Aggregator
{
namespace WebAccess
{
	namespace
	{
		class WittyThread : public QThread
		{
			Wt::WApplication * const App_;
		public:
			WittyThread (Wt::WApplication *app)
			: App_ { app }
			{
				setObjectName ("Aggregator WebAccess (Wt Thread)");
			}
		protected:
			void run ()
			{
				App_->attachThread (true);
				QThread::run ();
				App_->attachThread (false);
			}
		};
	}

	AggregatorApp::AggregatorApp (IProxyObject *ap, ICoreProxy_ptr cp,
			const Wt::WEnvironment& environment)
	: WApplication { environment }
	, AP_ { ap }
	, CP_ { cp }
	, ObjsThread_ { new WittyThread (this) }
	, ChannelsModel_ { new Q2WProxyModel { AP_->GetChannelsModel (), this } }
	, ChannelsFilter_ { std::make_shared<ReadChannelsFilter> (this) }
	, SourceItemModel_ { AP_->CreateItemsModel () }
	, ItemsModel_ { new Q2WProxyModel { SourceItemModel_, this } }
	, ItemsFilter_ { std::make_shared<ReadItemsFilter> () }
	{
		ChannelsModel_->SetRoleMappings ({
				{ ChannelRole::UnreadCount, Aggregator::ChannelRoles::UnreadCount },
				{ ChannelRole::CID, Aggregator::ChannelRoles::ChannelID }
			});

		ItemsModel_->SetRoleMappings ({
				{ ItemRole::IID, Aggregator::IItemsModel::ItemRole::ItemId },
				{ ItemRole::IsRead, Aggregator::IItemsModel::ItemRole::IsRead }
			});
		ItemsModel_->AddDataMorphism ([] (const QModelIndex& idx, int role) -> Wt::cpp17::any
			{
				if (role != Wt::StyleClassRole)
					return {};

				if (!idx.data (Aggregator::IItemsModel::ItemRole::IsRead).toBool ())
					return Wt::WString ("unreadItem");

				return {};
			});

		auto initThread = [this] (QObject *obj)
		{
			obj->moveToThread (ObjsThread_);
			QObject::connect (ObjsThread_,
					SIGNAL (finished ()),
					obj,
					SLOT (deleteLater ()));
		};
		initThread (ChannelsModel_);
		initThread (SourceItemModel_);
		initThread (ItemsModel_);

		ObjsThread_->start ();

		ChannelsFilter_->setSourceModel (ChannelsModel_);
		ItemsFilter_->setSourceModel (ItemsModel_);

		setTitle ("Aggregator WebAccess");
		setLoadingIndicator (new Wt::WOverlayLoadingIndicator ());

		SetupUI ();

		enableUpdates (true);
	}

	AggregatorApp::~AggregatorApp ()
	{
		ObjsThread_->quit ();
		ObjsThread_->wait (1000);
		if (!ObjsThread_->isFinished ())
		{
			qWarning () << Q_FUNC_INFO
					<< "objects thread hasn't finished yet, terminating...";
			ObjsThread_->terminate ();
		}

		delete ObjsThread_;
	}

	void AggregatorApp::HandleChannelClicked (const Wt::WModelIndex& idx)
	{
		ItemView_->setText ({});

		const auto cid = Wt::cpp17::any_cast<IDType_t> (idx.data (ChannelRole::CID));

		ItemsFilter_->ClearCurrentItem ();
		ItemsModelDecorator { SourceItemModel_ }.Reset (cid);
	}

	void AggregatorApp::HandleItemClicked (const Wt::WModelIndex& idx, const Wt::WMouseEvent& event)
	{
		if (!idx.isValid ())
			return;

		const auto& src = ItemsModel_->MapToSource (ItemsFilter_->mapToSource (idx));
		const auto itemId = Wt::cpp17::any_cast<IDType_t> (idx.data (ItemRole::IID));
		const auto& item = AP_->GetItem (itemId);
		if (!item)
			return;

		ItemsFilter_->SetCurrentItem (itemId);

		switch (event.button ())
		{
		case Wt::WMouseEvent::LeftButton:
			ShowItem (src, item);
			break;
		case Wt::WMouseEvent::RightButton:
			ShowItemMenu (src, item, event);
			break;
		default:
			break;
		}
	}

	void AggregatorApp::ShowItem (const QModelIndex& src, const Item_ptr& item)
	{
		ItemsModelDecorator { SourceItemModel_ }.Selected (src);

		auto text = Wt::WString ("<div><a href='{1}' target='_blank'>{2}</a><br />{3}<br /><hr/>{4}</div>")
				.arg (ToW (item->Link_))
				.arg (ToW (item->Title_))
				.arg (ToW (item->PubDate_.toString ()))
				.arg (ToW (item->Description_));
		ItemView_->setText (text);
	}

	void AggregatorApp::ShowItemMenu (const QModelIndex&,
			const Item_ptr& item, const Wt::WMouseEvent& event)
	{
		Wt::WPopupMenu menu;
		if (item->Unread_)
			menu.addItem (ToW (tr ("Mark as read")))->
					triggered ().connect (WF ([this, &item] { AP_->SetItemRead (item->ItemID_, true); }));
		else
			menu.addItem (ToW (tr ("Mark as unread")))->
					triggered ().connect (WF ([this, &item] { AP_->SetItemRead (item->ItemID_, false); }));
		menu.exec (event);
	}

	void AggregatorApp::SetupUI ()
	{
		setTheme (new Wt::WCssTheme ("polished"));
		setLocale ({ QLocale {}.name ().toUtf8 ().constData () });

		styleSheet ().addRule (".unreadItem", "font-weight: bold;");

		auto rootLay = new Wt::WBoxLayout (Wt::WBoxLayout::LeftToRight);
		root ()->setLayout (rootLay);

		auto leftPaneLay = new Wt::WBoxLayout (Wt::WBoxLayout::TopToBottom);
		rootLay->addLayout (leftPaneLay, 2);

		auto showReadChannels = new Wt::WCheckBox (ToW (tr ("Include read channels")));
		showReadChannels->setToolTip (ToW (tr ("Also display channels that have no unread items.")));
		showReadChannels->setChecked (false);
		showReadChannels->checked ().connect (WF ([this] { ChannelsFilter_->SetHideRead (false); }));
		showReadChannels->unChecked ().connect (WF ([this] { ChannelsFilter_->SetHideRead (true); }));
		leftPaneLay->addWidget (showReadChannels);

		auto channelsTree = new Wt::WTreeView ();
		channelsTree->setModel (ChannelsFilter_);
		channelsTree->setSelectionMode (Wt::SingleSelection);
		channelsTree->clicked ().connect (this, &AggregatorApp::HandleChannelClicked);
		channelsTree->setAlternatingRowColors (true);
		leftPaneLay->addWidget (channelsTree, 1, Wt::AlignTop);

		auto rightPaneLay = new Wt::WBoxLayout (Wt::WBoxLayout::TopToBottom);
		rootLay->addLayout (rightPaneLay, 7);

		auto showReadItems = new Wt::WCheckBox (ToW (tr ("Show read items")));
		showReadItems->setChecked (false);
		showReadItems->checked ().connect (WF ([this] { ItemsFilter_->SetHideRead (false); }));
		showReadItems->unChecked ().connect (WF ([this] { ItemsFilter_->SetHideRead (true); }));
		rightPaneLay->addWidget (showReadItems);

		ItemsTable_ = new Wt::WTableView ();
		ItemsTable_->setModel (ItemsFilter_);
		ItemsTable_->mouseWentUp ().connect (this, &AggregatorApp::HandleItemClicked);
		ItemsTable_->setAlternatingRowColors (true);
		ItemsTable_->setColumnWidth (0, { 550, Wt::WLength::Pixel });
		ItemsTable_->setSelectionMode (Wt::SingleSelection);
		ItemsTable_->setAttributeValue ("oncontextmenu",
				"event.cancelBubble = true; event.returnValue = false; return false;");
		rightPaneLay->addWidget (ItemsTable_, 2, Wt::AlignJustify);

		ItemView_ = new Wt::WText ();
		ItemView_->setTextFormat (Wt::XHTMLUnsafeText);

		auto scrollArea = new Wt::WScrollArea;
		scrollArea->setHorizontalScrollBarPolicy (Wt::WScrollArea::ScrollBarAlwaysOff);
		scrollArea->setVerticalScrollBarPolicy (Wt::WScrollArea::ScrollBarAsNeeded);
		scrollArea->setWidget (ItemView_);

		auto itemPanel = new Wt::WPanel ();
		itemPanel->setCentralWidget (scrollArea);

		rightPaneLay->addWidget (itemPanel, 5);
	}
}
}
}
