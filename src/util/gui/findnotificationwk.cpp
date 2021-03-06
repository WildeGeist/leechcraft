/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2014  Georg Rudoy
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE or copy at https://www.boost.org/LICENSE_1_0.txt)
 **********************************************************************/

#include "findnotificationwk.h"
#include <QWebView>
#include <util/sll/slotclosure.h>

namespace LC
{
namespace Util
{
	FindNotificationWk::FindNotificationWk (ICoreProxy_ptr proxy, QWebView *near)
	: FindNotification { proxy, near }
	, WebView_ { near }
	{
		new Util::SlotClosure<Util::NoDeletePolicy>
		{
			[this]
			{
				if (PreviousFindText_.isEmpty ())
					return;

				ClearFindResults ();
				findNext ();
			},
			near,
			SIGNAL (loadFinished (bool)),
			this
		};
	}

	QWebPage::FindFlags FindNotificationWk::ToPageFlags (FindFlags flags)
	{
		QWebPage::FindFlags pageFlags;
		auto check = [&pageFlags, flags] (FindFlag ourFlag, QWebPage::FindFlag pageFlag)
		{
			if (flags & ourFlag)
				pageFlags |= pageFlag;
		};
		check (FindCaseSensitively, QWebPage::FindCaseSensitively);
		check (FindBackwards, QWebPage::FindBackward);
		check (FindWrapsAround, QWebPage::FindWrapsAroundDocument);
		return pageFlags;
	}

	void FindNotificationWk::ClearFindResults ()
	{
		PreviousFindText_ = "";
		WebView_->page ()->findText ("", QWebPage::HighlightAllOccurrences);
	}

	void FindNotificationWk::handleNext (const QString& text, FindNotification::FindFlags findFlags)
	{
		const auto flags = ToPageFlags (findFlags);

		if (PreviousFindText_ != text)
		{
			const auto nflags = flags | QWebPage::HighlightAllOccurrences;
			WebView_->page ()->findText ({}, nflags);
			WebView_->page ()->findText (text, nflags);
			PreviousFindText_ = text;
		}

		const auto found = WebView_->page ()->findText (text, flags);
		SetSuccessful (found);
	}

	void FindNotificationWk::reject ()
	{
		FindNotification::reject ();
		ClearFindResults ();
	}
}
}
