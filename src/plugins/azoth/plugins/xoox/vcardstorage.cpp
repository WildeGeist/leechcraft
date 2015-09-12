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

#include "vcardstorage.h"
#include <QXmlStreamWriter>
#include <QtDebug>
#include <util/threads/futures.h>
#include "vcardstorageondisk.h"
#include "vcardstorageondiskwriter.h"

namespace LeechCraft
{
namespace Azoth
{
namespace Xoox
{
	VCardStorage::VCardStorage (QObject *parent)
	: QObject { parent }
	, DB_ { new VCardStorageOnDisk { this } }
	, Writer_
	{
		new VCardStorageOnDiskWriter,
		[] (VCardStorageOnDiskWriter *writer)
		{
			writer->quit ();
			writer->wait (5000);
			delete writer;
		}
	}
	, VCardCache_ { 1024 * 1024 }
	{
		Writer_->start (QThread::IdlePriority);
	}

	void VCardStorage::SetVCard (const QString& jid, const QString& vcard)
	{
		PendingVCards_ [jid] = vcard;

		Util::Sequence (this, Writer_->SetVCard (jid, vcard)) >>
				[this, jid] { PendingVCards_.remove (jid); };
	}

	void VCardStorage::SetVCard (const QString& jid, const QXmppVCardIq& vcard)
	{
		QString serialized;
		QXmlStreamWriter writer { &serialized };
		vcard.toXml (&writer);

		SetVCard (jid, serialized);
	}

	boost::optional<QXmppVCardIq> VCardStorage::GetVCard (const QString& jid) const
	{
		if (const auto vcard = VCardCache_.object (jid))
			return *vcard;

		const auto res = GetVCardString (jid);
		if (!res)
			return {};

		QDomDocument vcardDoc;
		if (!vcardDoc.setContent (*res))
		{
			qWarning () << Q_FUNC_INFO
					<< "unable to parse"
					<< *res;
			return {};
		}

		QXmppVCardIq vcard;
		vcard.parse (vcardDoc.documentElement ());

		VCardCache_.insert (jid, new QXmppVCardIq { vcard }, res->size ());

		return vcard;
	}

	boost::optional<QString> VCardStorage::GetVCardString (const QString& jid) const
	{
		if (PendingVCards_.contains (jid))
			return PendingVCards_.value (jid);

		return DB_->GetVCard (jid);
	}
}
}
}
