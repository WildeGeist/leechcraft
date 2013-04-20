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

#pragma once

#include <QMetaType>
#include <QStringList>
#include <QDateTime>
#include <QHash>
#include <QUrl>
#include <QByteArrayMatcher>
#include "regexp.h"

namespace LeechCraft
{
namespace Poshuku
{
namespace CleanWeb
{
	struct FilterOption
	{
		Qt::CaseSensitivity Case_;
		enum MatchType
		{
			MTWildcard,
			MTRegexp,
			MTPlain,
			MTBegin,
			MTEnd
		} MatchType_;

		enum MatchObject
		{
			All = 0x00,
			Script = 0x01,
			Image = 0x02,
			Object = 0x04,
			CSS = 0x08,
			ObjSubrequest = 0x10,
			Subdocument = 0x20,
			AJAX = 0x40,
			Popup = 0x80
		};
		Q_DECLARE_FLAGS (MatchObjects, MatchObject)
		MatchObjects MatchObjects_;

		QStringList Domains_;
		QStringList NotDomains_;
		QString HideSelector_;
		bool AbortForeign_;

		FilterOption ();
	};

	QDataStream& operator<< (QDataStream&, const FilterOption&);
	QDataStream& operator>> (QDataStream&, FilterOption&);

	bool operator== (const FilterOption&, const FilterOption&);
	bool operator!= (const FilterOption&, const FilterOption&);

	struct SubscriptionData
	{
		/// The URL of the subscription.
		QUrl URL_;

		/** The name of the subscription as provided by the abp:
		 * link.
		 */
		QString Name_;

		/// This is the name of the file inside the
		//~/.leechcraft/cleanweb/.
		QString Filename_;

		/// The date/time of last update.
		QDateTime LastDateTime_;
	};

	struct FilterItem
	{
		QByteArray OrigString_;
		RegExp RegExp_;
		QByteArrayMatcher PlainMatcher_;
		FilterOption Option_;
	};

	QDataStream& operator<< (QDataStream&, const FilterItem&);
	QDataStream& operator>> (QDataStream&, FilterItem&);

	struct Filter
	{
		QList<FilterItem> Filters_;
		QList<FilterItem> Exceptions_;

		SubscriptionData SD_;

		Filter& operator+= (const Filter&);
	};
}
}
}

Q_DECLARE_METATYPE (LeechCraft::Poshuku::CleanWeb::FilterItem);
Q_DECLARE_METATYPE (QList<LeechCraft::Poshuku::CleanWeb::FilterItem>);
