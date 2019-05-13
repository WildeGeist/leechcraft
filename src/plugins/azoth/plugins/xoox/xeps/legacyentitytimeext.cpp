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

#include "legacyentitytimeext.h"
#include <QDomElement>
#include <QDateTime>
#include <QXmppClient.h>

namespace LeechCraft
{
namespace Azoth
{
namespace Xoox
{
	const QString NsLegacyEntityTime = "jabber:iq:time";

	QStringList LegacyEntityTimeExt::discoveryFeatures () const
	{
		return QStringList (NsLegacyEntityTime);
	}

	bool LegacyEntityTimeExt::handleStanza (const QDomElement& elem)
	{
		if (elem.tagName () != "iq" ||
				elem.attribute ("type") != "get")
			return false;

		if (elem.firstChildElement ("query").namespaceURI () != NsLegacyEntityTime)
			return false;

		const QString& from = elem.attribute ("from");
		if (from.isEmpty ())
			return false;

		const QDateTime& date = QDateTime::currentDateTime ().toUTC ();

		QXmppElement utcElem;
		utcElem.setTagName ("utc");
		utcElem.setValue (date.toString ("yyyyMMddThh:mm:ss"));

		const auto& displayStr = QDateTime::currentDateTime ().toString ();
		QXmppElement displayElem;
		displayElem.setTagName ("display");
		displayElem.setValue (displayStr);

		QXmppElement queryElem;
		queryElem.setTagName ("query");
		queryElem.setAttribute ("xmlns", NsLegacyEntityTime);
		queryElem.appendChild (utcElem);
		queryElem.appendChild (displayElem);

		QXmppIq iq (QXmppIq::Result);
		iq.setTo (from);
		iq.setId (elem.attribute ("id"));
		iq.setExtensions (QXmppElementList () << queryElem);

		client ()->sendPacket (iq);

		return true;
	}
}
}
}