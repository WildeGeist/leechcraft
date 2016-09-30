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

#pragma once

#include <memory>
#include <qwebview.h>
#include <interfaces/structures.h>
#include <interfaces/iwkfontssettable.h>
#include <interfaces/core/icoreproxyfwd.h>
#include <interfaces/core/ihookproxy.h>
#include "interfaces/poshuku/poshukutypes.h"
#include "interfaces/poshuku/iwebview.h"

class QTimer;
class QWebInspector;

class IEntityManager;

namespace LeechCraft
{
namespace Util
{
	class FindNotificationWk;
}

namespace Poshuku
{
class IProxyObject;

namespace WebKitView
{
	class WebViewSslWatcherHandler;

	class CustomWebView : public QWebView
						, public IWebView
						, public IWkFontsSettable
	{
		Q_OBJECT
		Q_INTERFACES (IWkFontsSettable LeechCraft::Poshuku::IWebView)

		const ICoreProxy_ptr Proxy_;

		mutable QString PreviousEncoding_;

		std::shared_ptr<QWebInspector> WebInspector_;

		const WebViewSslWatcherHandler *SslWatcherHandler_;

		Util::FindNotificationWk *FindDialog_ = nullptr;
	public:
		CustomWebView (const ICoreProxy_ptr&, IProxyObject*, QWidget* = nullptr);

		void SurroundingsInitialized () override;

		QWidget* GetQWidget () override;
		QList<QAction*> GetActions (ActionArea) const override;

		QAction* GetPageAction (PageAction) const override;

		QString GetTitle () const override;
		QUrl GetUrl () const override;
		QString GetHumanReadableUrl () const override;
		QIcon GetIcon () const override;

		void Load (const QUrl&, const QString&) override;

		void SetContent (const QByteArray&, const QByteArray&, const QUrl& = {}) override;
		QString ToHtml () const override;
		void EvaluateJS (const QString&,
				const std::function<void (QVariant)>&,
				Util::BitFlags<EvaluateJSFlag>) override;
		void AddJavaScriptObject (const QString&, QObject*) override;

		void Print (bool preview) override;
		QPixmap MakeFullPageSnapshot () override;

		QPoint GetScrollPosition () const override;
		void SetScrollPosition (const QPoint&) override;
		double GetZoomFactor () const override;
		void SetZoomFactor (double) override;
		double GetTextSizeMultiplier () const override;
		void SetTextSizeMultiplier (double) override;

		QString GetDefaultTextEncoding () const override;
		void SetDefaultTextEncoding (const QString&) override;

		void InitiateFind (const QString&) override;

		QMenu* CreateStandardContextMenu () override;

		IWebViewHistory_ptr GetHistory () override;

		void SetAttribute (Attribute, bool) override;

		void SetFontFamily (FontFamily family, const QFont& font) override;
		void SetFontSize (FontSize type, int size) override;
		void SetFontSizeMultiplier (qreal factor) override;
		QObject* GetQObject () override;

		void Load (const QNetworkRequest&,
				QNetworkAccessManager::Operation = QNetworkAccessManager::GetOperation,
				const QByteArray& = QByteArray ());

		/** This function is equivalent to url.toString() if the url is
		 * all in UTF-8. But if the site is in another encoding,
		 * QUrl::toString() returns a bad, unreadable and, moreover,
		 * unusable string. In this case, this function converts the url
		 * to its percent-encoding representation.
		 *
		 * @param[in] url The possibly non-UTF-8 URL.
		 * @return The \em url converted to Unicode.
		 */
		QString URLToProperString (const QUrl& url) const;
	protected:
		void mousePressEvent (QMouseEvent*) override;
		void contextMenuEvent (QContextMenuEvent*) override;
		void keyReleaseEvent (QKeyEvent*) override;
	private:
		void NavigatePlugins ();
		void NavigateHome ();
		void PrintImpl (bool, QWebFrame*);
	private slots:
		void remakeURL (const QUrl&);
		void handleLoadFinished (bool);
		void handleFrameState (QWebFrame*, QWebHistoryItem*);
		void handlePrintRequested (QWebFrame*);

		void handleFeaturePermissionReq (QWebFrame*, QWebPage::Feature);
	signals:
		void urlChanged (const QString&) override;
		void closeRequested () override;

		void navigateRequested (const QUrl&);

		void zoomChanged () override;

		void contextMenuRequested (const QPoint& globalPos, const ContextMenuInfo&) override;

		void earliestViewLayout () override;
		void linkHovered (const QString& link, const QString& title, const QString& textContent) override;
		void storeFormData (const PageFormsData_t&) override;
		void featurePermissionRequested (const IWebView::IFeatureSecurityOrigin_ptr&,
				IWebView::Feature) override;

		void webViewCreated (IWebView*, bool);
	};
}
}
}
