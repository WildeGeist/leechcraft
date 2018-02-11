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

#include "offlinedatasource.h"
#include <QXmlStreamWriter>
#include <QDomElement>
#include <util/sll/domchildrenrange.h>
#include <util/sll/prelude.h>
#include <interfaces/azoth/iproxyobject.h>
#include "vcardstorage.h"
#include "util.h"
#include "glooxaccount.h"

namespace LeechCraft
{
namespace Azoth
{
namespace Xoox
{
	void Save (OfflineDataSource_ptr ods, QXmlStreamWriter *w, IProxyObject *proxy)
	{
		w->writeStartElement ("entry");
			w->writeTextElement ("idstr", ods->ID_);
			w->writeTextElement ("name", ods->Name_);
			w->writeTextElement ("authstatus", proxy->AuthStatusToString (ods->AuthStatus_));

			w->writeStartElement ("groups");
				for (const auto& group : ods->Groups_)
					w->writeTextElement ("group", group);
			w->writeEndElement ();
		w->writeEndElement ();
	}

	namespace
	{
		void LoadVCard (const QDomElement& vcardElem, const QString& entryId, GlooxAccount *acc, VCardStorage *storage)
		{
			if (vcardElem.isNull ())
				return;

			storage->SetVCard (XooxUtil::GetBareJID (entryId, acc),
					QByteArray::fromBase64 (vcardElem.text ().toLatin1 ()));
		}

		QString LoadEntryID (const QDomElement& entry)
		{
			const auto& idStrElem = entry.firstChildElement ("idstr");
			if (!idStrElem.isNull ())
				return idStrElem.text ();

			const auto& idElem = entry.firstChildElement ("id");
			return QString::fromUtf8 (QByteArray::fromPercentEncoding (idElem.text ().toLatin1 ()));
		}
	}

	void Load (OfflineDataSource_ptr ods,
			const QDomElement& entry,
			IProxyObject *proxy,
			GlooxAccount *acc)
	{
		const auto& entryID = LoadEntryID (entry);

		auto groups = Util::Map (Util::DomChildren (entry.firstChildElement ("groups"), "group"),
				[] (const QDomElement& group) { return group.text (); });
		groups.removeAll ({});

		ods->Name_ = entry.firstChildElement ("name").text ();
		ods->ID_ = entryID;
		ods->Groups_ = groups;

		const auto& authStatusText = entry.firstChildElement ("authstatus").text ();
		ods->AuthStatus_ = proxy->AuthStatusFromString (authStatusText);

		LoadVCard (entry.firstChildElement ("vcard"), entryID,
				acc, acc->GetParentProtocol ()->GetVCardStorage ());
	}
}
}
}