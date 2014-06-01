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

#include "roomconfigwidget.h"
#include <QVBoxLayout>
#include <QStandardItemModel>
#include <QXmppMucManager.h>
#include "roomclentry.h"
#include "glooxaccount.h"
#include "clientconnection.h"
#include "roomhandler.h"
#include "formbuilder.h"
#include "affiliationselectordialog.h"

namespace LeechCraft
{
namespace Azoth
{
namespace Xoox
{
	RoomConfigWidget::RoomConfigWidget (RoomCLEntry *room, QWidget *widget)
	: QWidget (widget)
	, FormWidget_ (0)
	, FB_ (new FormBuilder)
	, Room_ (room)
	, JID_ (room->GetRoomHandler ()->GetRoomJID ())
	, RoomHandler_ (0)
	, PermsModel_ (new QStandardItemModel (this))
	, Aff2Cat_ (InitModel ())
	{
		Ui_.setupUi (this);
		Ui_.PermsTree_->setModel (PermsModel_);

		GlooxAccount *acc = qobject_cast<GlooxAccount*> (room->GetParentAccount ());
		QXmppMucManager *mgr = acc->GetClientConnection ()->GetMUCManager ();

		RoomHandler_ = mgr->addRoom (JID_);
		connect (RoomHandler_,
				SIGNAL (configurationReceived (const QXmppDataForm&)),
				this,
				SLOT (handleConfigurationReceived (const QXmppDataForm&)));
		connect (RoomHandler_,
				SIGNAL (permissionsReceived (const QList<QXmppMucItem>&)),
				this,
				SLOT (handlePermsReceived (const QList<QXmppMucItem>&)));
		RoomHandler_->requestConfiguration ();
		RoomHandler_->requestPermissions ();
	}

	QMap<QXmppMucItem::Affiliation, QStandardItem*> RoomConfigWidget::InitModel () const
	{
		PermsModel_->clear ();
		PermsModel_->setHorizontalHeaderLabels ({ ("JID"), tr ("Reason") });
		QMap<QXmppMucItem::Affiliation, QStandardItem*> aff2cat;
		aff2cat [QXmppMucItem::OutcastAffiliation] = new QStandardItem (tr ("Banned"));
		aff2cat [QXmppMucItem::MemberAffiliation] = new QStandardItem (tr ("Members"));
		aff2cat [QXmppMucItem::AdminAffiliation] = new QStandardItem (tr ("Admins"));
		aff2cat [QXmppMucItem::OwnerAffiliation] = new QStandardItem (tr ("Owners"));
		for (auto item : aff2cat)
		{
			QList<QStandardItem*> rootItems;
			rootItems << item;
			rootItems << new QStandardItem (tr ("Reason"));
			for (auto t : rootItems)
				t->setEditable (false);
			PermsModel_->appendRow (rootItems);
		}
		return aff2cat;
	}

	void RoomConfigWidget::SendItem (const QXmppMucItem& item)
	{
		QList<QXmppMucItem> items;
		items << item;
		QXmppMucAdminIq iq;
		iq.setTo (JID_);
		iq.setType (QXmppIq::Set);
		iq.setItems (items);

		GlooxAccount *account = qobject_cast<GlooxAccount*> (Room_->GetParentAccount ());
		account->GetClientConnection ()->GetClient ()->sendPacket (iq);
	}

	QStandardItem* RoomConfigWidget::GetCurrentItem () const
	{
		const QModelIndex& index = Ui_.PermsTree_->currentIndex ();
		if (!index.isValid ())
			return 0;

		const QModelIndex& sibling = index.sibling (index.row (), 0);
		return PermsModel_->itemFromIndex (sibling);
	}

	void RoomConfigWidget::accept ()
	{
		auto form = FB_->GetForm ();
		form.setType (QXmppDataForm::Submit);
		RoomHandler_->setConfiguration (form);
	}

	void RoomConfigWidget::on_AddPerm__released ()
	{
		AffiliationSelectorDialog dia (this);
		if (dia.exec () != QDialog::Accepted)
			return;

		const QString& jid = dia.GetJID ();
		if (jid.isEmpty ())
			return;

		QXmppMucItem item;
		item.setJid (jid);
		item.setAffiliation (dia.GetAffiliation ());
		SendItem (item);

		handlePermsReceived ({ item });
	}

	void RoomConfigWidget::on_ModifyPerm__released ()
	{
		QStandardItem *stdItem = GetCurrentItem ();
		if (!stdItem)
			return;

		QStandardItem *parent = stdItem->parent ();
		if (!Aff2Cat_.values ().contains (parent))
		{
			qWarning () << Q_FUNC_INFO
					<< "bad parent"
					<< parent
					<< "for"
					<< stdItem;
			return;
		}

		const QXmppMucItem::Affiliation aff = Aff2Cat_.key (parent);
		const QString& jid = stdItem->text ();

		std::unique_ptr<AffiliationSelectorDialog> dia (new AffiliationSelectorDialog (this));
		dia->SetJID (jid);
		dia->SetAffiliation (aff);
		dia->SetReason (stdItem->data (ItemRoles::Reason).toString ());
		if (dia->exec () != QDialog::Accepted)
			return;

		const QString& newJid = dia->GetJID ();
		if (newJid.isEmpty ())
			return;

		parent->removeRow (stdItem->row ());

		QXmppMucItem item;
		item.setJid (newJid);
		item.setAffiliation (dia->GetAffiliation ());
		item.setReason (dia->GetReason ());
		SendItem (item);

		if (item.affiliation () != QXmppMucItem::NoAffiliation)
			handlePermsReceived ({ item });
	}

	void RoomConfigWidget::on_RemovePerm__released ()
	{
		QStandardItem *stdItem = GetCurrentItem ();
		if (!stdItem)
			return;

		const QString& jid = stdItem->text ();
		if (jid.isEmpty ())
			return;

		QStandardItem *parent = stdItem->parent ();
		if (!parent)
			return;

		parent->removeRow (stdItem->row ());

		QXmppMucItem item;
		item.setJid (jid);
		item.setAffiliation (QXmppMucItem::NoAffiliation);
		SendItem (item);
	}

	void RoomConfigWidget::handleConfigurationReceived (const QXmppDataForm& form)
	{
		if (sender () != RoomHandler_)
			return;

		FB_->Clear ();

		FormWidget_ = FB_->CreateForm (form);
		Ui_.ScrollArea_->setWidget (FormWidget_);
		emit dataReady ();
	}

	void RoomConfigWidget::handlePermsReceived (const QList<QXmppMucItem>& perms)
	{
		if (qobject_cast<QXmppMucRoom*> (sender ()) &&
				sender () != RoomHandler_)
			return;

		for (const auto& perm : perms)
		{
			auto parentItem = Aff2Cat_ [perm.affiliation ()];
			if (!parentItem)
			{
				qWarning () << Q_FUNC_INFO
						<< "no parent item for"
						<< perm.affiliation ();
				continue;
			}

			auto firstItem = new QStandardItem (perm.jid ());
			firstItem->setData (perm.reason (), ItemRoles::Reason);

			QList<QStandardItem*> items;
			items << firstItem;
			items << new QStandardItem (perm.reason ());
			Q_FOREACH (QStandardItem *item, items)
				item->setEditable (false);
			parentItem->appendRow (items);
		}

		for (auto item : Aff2Cat_.values ())
		{
			item->sortChildren (0);
			Ui_.PermsTree_->expand (item->index ());
		}
	}
}
}
}
