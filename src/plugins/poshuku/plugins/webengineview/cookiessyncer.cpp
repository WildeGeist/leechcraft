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

#include "cookiessyncer.h"
#include <QWebEngineCookieStore>
#include <QTimer>
#include <QtDebug>
#include <util/network/customcookiejar.h>

namespace LC::Poshuku::WebEngineView
{
	CookiesSyncer::CookiesSyncer (Util::CustomCookieJar *lcJar,
			QWebEngineCookieStore *weStore)
	: LCJar_ { lcJar }
	, WebEngineStore_ { weStore }
	{
		WebEngineStore_->deleteAllCookies ();

		HandleLCCookiesAdded (LCJar_->allCookies ());

		connect (LCJar_,
				&Util::CustomCookieJar::cookiesAdded,
				this,
				&CookiesSyncer::HandleLCCookiesAdded);
		connect (LCJar_,
				&Util::CustomCookieJar::cookiesRemoved,
				this,
				&CookiesSyncer::HandleLCCookiesRemoved);

		connect (WebEngineStore_,
				&QWebEngineCookieStore::cookieAdded,
				this,
				&CookiesSyncer::HandleWebEngineCookieAdded);
		connect (WebEngineStore_,
				&QWebEngineCookieStore::cookieRemoved,
				this,
				&CookiesSyncer::HandleWebEngineCookieRemoved);
	}

	void CookiesSyncer::HandleLCCookiesAdded (const QList<QNetworkCookie>& cookies)
	{
		qDebug () << Q_FUNC_INFO << cookies.size ();
		for (const auto& cookie : cookies)
			WebEngineStore_->setCookie (cookie);
	}

	void CookiesSyncer::HandleLCCookiesRemoved (const QList<QNetworkCookie>& cookies)
	{
		qDebug () << Q_FUNC_INFO << cookies.size ();
		for (const auto& cookie : cookies)
			WebEngineStore_->deleteCookie (cookie);
	}

	void CookiesSyncer::HandleWebEngineCookieAdded (const QNetworkCookie& cookie)
	{
		if (WebEngine2LCQueue_.isEmpty ())
			QTimer::singleShot (1000, Qt::VeryCoarseTimer, this,
					[this]
					{
						for (const auto& cookie : WebEngine2LCQueue_)
							LCJar_->insertCookie (cookie);
						WebEngine2LCQueue_.clear ();
					});

		WebEngine2LCQueue_.prepend (cookie);
	}

	void CookiesSyncer::HandleWebEngineCookieRemoved (const QNetworkCookie& cookie)
	{
		WebEngine2LCQueue_.removeAll (cookie);
		LCJar_->deleteCookie (cookie);
	}
}
