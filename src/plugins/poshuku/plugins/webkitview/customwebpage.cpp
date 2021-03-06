/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2014  Georg Rudoy
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE or copy at https://www.boost.org/LICENSE_1_0.txt)
 **********************************************************************/

#include "customwebpage.h"
#include <algorithm>
#include <QtDebug>
#include <QFile>
#include <QBuffer>
#include <qwebframe.h>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QDesktopServices>
#include <qwebelement.h>
#include <qwebhistory.h>
#include <util/xpc/util.h>
#include <util/xpc/defaulthookproxy.h>
#include <util/sll/slotclosure.h>
#include <util/sys/sysinfo.h>
#include <interfaces/core/icoreproxy.h>
#include <interfaces/core/ientitymanager.h>
#include <interfaces/core/iiconthememanager.h>
#include <interfaces/poshuku/iproxyobject.h>
#include <util/sll/util.h>
#include "xmlsettingsmanager.h"
#include "customwebview.h"
#include "pluginmanager.h"
#include "jsproxy.h"
#include "externalproxy.h"
#include "webpluginfactory.h"

Q_DECLARE_METATYPE (QVariantList*);
Q_DECLARE_METATYPE (QNetworkReply*);

namespace LC
{
namespace Poshuku
{
namespace WebKitView
{
	CustomWebPage::CustomWebPage (const ICoreProxy_ptr& coreProxy,
			IProxyObject *poshukuProxy, QObject *parent)
	: QWebPage (parent)
	, Proxy_ (coreProxy)
	, PoshukuProxy_ (poshukuProxy)
	, JSProxy_ (new JSProxy (this))
	, ExternalProxy_ (new ExternalProxy (coreProxy->GetEntityManager (), this))
	, LinkOpenModifier_ (poshukuProxy->GetLinkOpenModifier ())
	{
		{
			auto proxy = std::make_shared<Util::DefaultHookProxy> ();
			emit hookWebPageConstructionBegin (proxy, this);
			if (proxy->IsCancelled ())
				return;
		}

		setForwardUnsupportedContent (true);

		setNetworkAccessManager (coreProxy->GetNetworkAccessManager ());

		connect (this,
				SIGNAL (delayedFillForms (QWebFrame*)),
				this,
				SLOT (fillForms (QWebFrame*)),
				Qt::QueuedConnection);

		connect (mainFrame (),
				SIGNAL (javaScriptWindowObjectCleared ()),
				this,
				SLOT (handleJavaScriptWindowObjectCleared ()));
		connect (mainFrame (),
				SIGNAL (urlChanged (const QUrl&)),
				this,
				SIGNAL (loadingURL (const QUrl&)));
		connect (this,
				SIGNAL (contentsChanged ()),
				this,
				SLOT (handleContentsChanged ()));
		connect (this,
				SIGNAL (databaseQuotaExceeded (QWebFrame*, QString)),
				this,
				SLOT (handleDatabaseQuotaExceeded (QWebFrame*, QString)));
		connect (this,
				SIGNAL (downloadRequested (const QNetworkRequest&)),
				this,
				SLOT (handleDownloadRequested (const QNetworkRequest&)));
		connect (this,
				SIGNAL (frameCreated (QWebFrame*)),
				this,
				SLOT (handleFrameCreated (QWebFrame*)));
		connect (this,
				SIGNAL (geometryChangeRequested (const QRect&)),
				this,
				SLOT (handleGeometryChangeRequested (const QRect&)));
		connect (this,
				SIGNAL (linkClicked (const QUrl&)),
				this,
				SLOT (handleLinkClicked (const QUrl&)));
		connect (this,
				SIGNAL (linkHovered (const QString&,
						const QString&, const QString&)),
				this,
				SLOT (handleLinkHovered (const QString&,
						const QString&, const QString&)));
		connect (this,
				SIGNAL (loadFinished (bool)),
				this,
				SLOT (handleLoadFinished (bool)));
		connect (this,
				SIGNAL (unsupportedContent (QNetworkReply*)),
				this,
				SLOT (handleUnsupportedContent (QNetworkReply*)));
		connect (this,
				SIGNAL (windowCloseRequested ()),
				this,
				SLOT (handleWindowCloseRequested ()));

		FillErrorSuggestions ();

		{
			auto proxy = std::make_shared<Util::DefaultHookProxy> ();
			emit hookWebPageConstructionEnd (proxy, this);
			if (proxy->IsCancelled ())
				return;
		}
	}

	void CustomWebPage::HandleViewReady ()
	{
		LinkOpenModifier_->InstallOn (view ());
	}

	QUrl CustomWebPage::GetLoadingURL () const
	{
		return LoadingURL_;
	}

	bool CustomWebPage::supportsExtension (QWebPage::Extension e) const
	{
		auto proxy = std::make_shared<Util::DefaultHookProxy> ();
		emit hookSupportsExtension (proxy, this, e);
		if (proxy->IsCancelled ())
			return proxy->GetReturnValue ().toBool ();

		switch (e)
		{
		case ErrorPageExtension:
			return true;
		default:
			return QWebPage::supportsExtension (e);
		}
	}

	bool CustomWebPage::HandleExtensionProtocolUnknown (const ErrorPageExtensionOption *error)
	{
		auto e = Util::MakeEntity (error->url,
				{},
				FromUserInitiated);
		e.Additional_ ["IgnorePlugins"] = "org.LeechCraft.Poshuku";
		auto em = Proxy_->GetEntityManager ();
		if (!em->CouldHandle (e))
			return false;

		em->HandleEntity (e);
		const auto closeEmpty = PoshukuProxy_->GetPoshukuConfigValue ("CloseEmptyDelegatedPages").toBool ();
		if (closeEmpty &&
			history ()->currentItem ().url ().isEmpty ())
				emit windowCloseRequested ();

		return true;
	}

	bool CustomWebPage::extension (QWebPage::Extension e,
			const QWebPage::ExtensionOption* eo, QWebPage::ExtensionReturn *er)
	{
		auto proxy = std::make_shared<Util::DefaultHookProxy> ();
		emit hookExtension (proxy, this, e, eo, er);
		if (proxy->IsCancelled ())
			return proxy->GetReturnValue ().toBool ();

		switch (e)
		{
		case ErrorPageExtension:
		{
			auto error = static_cast<const ErrorPageExtensionOption*> (eo);
			auto ret = static_cast<ErrorPageExtensionReturn*> (er);

			qDebug () << Q_FUNC_INFO
					<< "error extension:"
					<< error->domain
					<< error->error
					<< error->errorString
					<< error->url;

			switch (error->error)
			{
			case 102:			// Delegated entity
				return false;
			case QNetworkReply::ProtocolUnknownError:
				return HandleExtensionProtocolUnknown (error);
			default:
			{
				QString data = MakeErrorReplyContents (error->error,
						error->url, error->errorString, error->domain);
				ret->baseUrl = error->url;
				ret->content = data.toUtf8 ();
				if (error->domain == QWebPage::QtNetwork)
					switch (error->error)
					{
					case QNetworkReply::ContentReSendError:
						Proxy_->GetEntityManager ()->HandleEntity (Util::MakeNotification ("Poshuku",
								tr ("Unable to send the request to %1. Please try submitting it again.")
									.arg (error->url.host ()),
								Priority::Critical));
						return false;
					default:
						return true;
					}
				else
					return true;
			}
			}
		}
		default:
			return QWebPage::extension (e, eo, er);
		}
	}

	void CustomWebPage::handleContentsChanged ()
	{
		emit hookContentsChanged (std::make_shared<Util::DefaultHookProxy> (),
				this);
	}

	void CustomWebPage::handleDatabaseQuotaExceeded (QWebFrame *frame, QString string)
	{
		emit hookDatabaseQuotaExceeded (std::make_shared<Util::DefaultHookProxy> (),
				this, frame, string);
	}

	void CustomWebPage::handleDownloadRequested (QNetworkRequest request)
	{
		auto proxy = std::make_shared<Util::DefaultHookProxy> ();
		emit hookDownloadRequested (proxy, this, request);
		if (proxy->IsCancelled ())
			return;

		proxy->FillValue ("request", request);

		auto e = Util::MakeEntity (request.url (),
				{},
				FromUserInitiated);
		e.Additional_ ["AllowedSemantics"] = QStringList { "fetch", "save" };
		e.Additional_ ["IgnorePlugins"] = "org.LeechCraft.Poshuku";
		Proxy_->GetEntityManager ()->HandleEntity (e);
	}

	void CustomWebPage::handleFrameCreated (QWebFrame *frame)
	{
		emit hookFrameCreated (std::make_shared<Util::DefaultHookProxy> (),
				this, frame);
	}

	void CustomWebPage::handleJavaScriptWindowObjectCleared ()
	{
		QWebFrame *frame = qobject_cast<QWebFrame*> (sender ());
		auto proxy = std::make_shared<Util::DefaultHookProxy> ();
		emit hookJavaScriptWindowObjectCleared (proxy, this, frame);
		if (proxy->IsCancelled ())
			return;

		frame->addToJavaScriptWindowObject ("JSProxy", JSProxy_.get ());
		frame->addToJavaScriptWindowObject ("external", ExternalProxy_.get ());
		frame->evaluateJavaScript (R"delim(
			if (!Function.prototype.bind) {
			Function.prototype.bind = function (oThis) {
				if (typeof this !== "function") {
				// closest thing possible to the ECMAScript 5 internal IsCallable function
				throw new TypeError("Function.prototype.bind - what is trying to be bound is not callable");
				}

				var aArgs = Array.prototype.slice.call(arguments, 1),
					fToBind = this,
					fNOP = function () {},
					fBound = function () {
					return fToBind.apply(this instanceof fNOP && oThis
											? this
											: oThis,
										aArgs.concat(Array.prototype.slice.call(arguments)));
					};

				fNOP.prototype = this.prototype || {};
				fBound.prototype = new fNOP();

				return fBound;
			};
			}
		)delim");
	}

	void CustomWebPage::handleGeometryChangeRequested (const QRect& rect)
	{
		emit hookGeometryChangeRequested (std::make_shared<Util::DefaultHookProxy> (),
				this, rect);
	}

	void CustomWebPage::handleLinkClicked (const QUrl& url)
	{
		emit loadingURL (url);
		emit hookLinkClicked (std::make_shared<Util::DefaultHookProxy> (), this, url);
	}

	void CustomWebPage::handleLinkHovered (const QString& link,
			const QString& title, const QString& context)
	{
		emit hookLinkHovered (std::make_shared<Util::DefaultHookProxy> (),
				this, link, title, context);
	}

	void CustomWebPage::handleLoadFinished (bool ok)
	{
		QWebElement body = mainFrame ()->findFirstElement ("body");

		if (body.findAll ("*").count () == 1 &&
				body.firstChild ().tagName () == "IMG")
			mainFrame ()->evaluateJavaScript ("function centerImg() {"
					"var img = document.querySelector('img');"
					"img.style.left = Math.floor((document.width - img.width) / 2) + 'px';"
					"img.style.top =  Math.floor((document.height - img.height) / 2) + 'px';"
					"img.style.position = 'absolute';"
					"}"
					"window.addEventListener('resize', centerImg, false);"
					"centerImg();");

		auto proxy = std::make_shared<Util::DefaultHookProxy> ();
		emit hookLoadFinished (proxy, this, ok);
		if (proxy->IsCancelled ())
			return;

		emit delayedFillForms (mainFrame ());
	}

	void CustomWebPage::handleUnsupportedContent (QNetworkReply *reply)
	{
		auto proxy = std::make_shared<Util::DefaultHookProxy> ();
		emit hookUnsupportedContent (proxy, this, reply);
		if (proxy->IsCancelled ())
			return;

		const auto replyGuard = Util::MakeScopeGuard ([reply]
				{
					reply->abort ();
					reply->deleteLater ();
				});

		const auto& url = reply->url ();
		const auto& mime = reply->header (QNetworkRequest::ContentTypeHeader).toString ();
		const auto& referer = reply->request ().rawHeader ("Referer");

		auto sendEnt = [reply, mime, referer, this]
		{
			auto e = Util::MakeEntity (reply->url (),
					{},
					FromUserInitiated,
					mime);
			e.Additional_ ["IgnorePlugins"] = "org.LeechCraft.Poshuku";
			e.Additional_ ["Referer"] = QUrl::fromEncoded (referer);
			e.Additional_ ["Operation"] = reply->operation ();
			Proxy_->GetEntityManager ()->HandleEntity (e);

			const auto closeEmpty = PoshukuProxy_->
					GetPoshukuConfigValue ("CloseEmptyDelegatedPages").toBool ();
			if (closeEmpty && history ()->currentItem ().url ().isEmpty ())
				emit windowCloseRequested ();
		};

		switch (reply->error ())
		{
		case QNetworkReply::ProtocolUnknownError:
			if (PoshukuProxy_->GetPoshukuConfigValue ("ExternalSchemes")
					.toString ()
					.split (' ')
					.contains (url.scheme ()))
				QDesktopServices::openUrl (url);
			else
				sendEnt ();
			break;
		case QNetworkReply::NoError:
		{
			auto found = FindFrame (url);
			if (!found)
			{
				const auto paranoidDetection = PoshukuProxy_->
						GetPoshukuConfigValue ("ParanoidDownloadsDetection").toBool ();
				if (paranoidDetection || !mime.isEmpty ())
				{
					sendEnt ();
					break;
				}
				else
					qDebug () << Q_FUNC_INFO
							<< mime;
			}
			else
				qDebug () << Q_FUNC_INFO
						<< "but frame is found";
			break;
		}
		default:
		{
			int statusCode = reply->attribute (QNetworkRequest::HttpStatusCodeAttribute).toInt ();
			if (statusCode / 100 == 2 || statusCode / 100 == 3)
			{
				sendEnt ();
				return;
			}

			qDebug () << Q_FUNC_INFO
					<< "general unsupported content"
					<< url
					<< reply->error ()
					<< reply->errorString ();

			const auto& data = MakeErrorReplyContents (statusCode,
					url, reply->errorString (), QtNetwork);

			if (auto found = FindFrame (url))
				found->setHtml (data, url);
			else if (LoadingURL_ == url)
				mainFrame ()->setHtml (data, url);
			break;
		}
		}
	}

	void CustomWebPage::FillErrorSuggestions ()
	{
		QString checkDown = tr ("<a href=\"http://downforeveryoneorjustme.com/{host}\" "
				"target=\"_blank\">check</a> if the site <strong>{host}</strong> is down for you only;",
				"{host} would be substituded with site's host name.");
		QString tryAgainLater = tr ("try again later");
		QString contactRemoteAdmin = tr ("contact remote server's administrator "
				"(typically at <a href=\"mailto:webmaster@{host}\">webmaster@{host}</a>)");
		QString contactSystemAdmin = tr ("contact your system/network administrator, "
				"especially if you can't load any single page");
		QString checkProxySettings = tr ("check your proxy settings");

		Error2Suggestions_ [QtNetwork] [QNetworkReply::ConnectionRefusedError]
				<< tryAgainLater + ";"
				<< contactRemoteAdmin + ";"
				<< checkDown;
		Error2Suggestions_ [QtNetwork] [QNetworkReply::RemoteHostClosedError]
				<< tryAgainLater + ";"
				<< contactRemoteAdmin + ";"
				<< checkDown;
		Error2Suggestions_ [QtNetwork] [QNetworkReply::HostNotFoundError]
				<< tr ("check if the URL is written correctly;")
				<< tr ("try changing your DNS servers;")
				<< tr ("make sure that LeechCraft is allowed to access the Internet and particularly web sites;")
				<< contactSystemAdmin + ";"
				<< checkDown;
		Error2Suggestions_ [QtNetwork] [QNetworkReply::TimeoutError]
				<< tryAgainLater + ";"
				<< tr ("check whether some downloads consume too much bandwidth: try limiting their speed or reducing number of connections for them;")
				<< contactSystemAdmin + ";"
				<< contactRemoteAdmin + ";"
				<< checkDown;
		Error2Suggestions_ [QtNetwork] [QNetworkReply::OperationCanceledError]
				<< tr ("try again.");
		Error2Suggestions_ [QtNetwork] [QNetworkReply::SslHandshakeFailedError]
				<< tr ("make sure that remote server is really what it claims to be;")
				<< contactSystemAdmin + ".";
		Error2Suggestions_ [QtNetwork] [QNetworkReply::TemporaryNetworkFailureError]
				<< tryAgainLater + ";"
				<< contactSystemAdmin + ".";
		Error2Suggestions_ [QtNetwork] [QNetworkReply::ProxyConnectionRefusedError]
				<< tryAgainLater + ";"
				<< checkProxySettings + ";"
				<< contactSystemAdmin + ".";
		Error2Suggestions_ [QtNetwork] [QNetworkReply::ProxyConnectionClosedError]
				<< tryAgainLater + ";"
				<< contactSystemAdmin + ".";
		Error2Suggestions_ [QtNetwork] [QNetworkReply::ProxyNotFoundError] =
				Error2Suggestions_ [QtNetwork] [QNetworkReply::ProxyConnectionRefusedError];
		Error2Suggestions_ [QtNetwork] [QNetworkReply::ProxyTimeoutError] =
				Error2Suggestions_ [QtNetwork] [QNetworkReply::ProxyConnectionRefusedError];
		Error2Suggestions_ [QtNetwork] [QNetworkReply::ProxyAuthenticationRequiredError] =
				Error2Suggestions_ [QtNetwork] [QNetworkReply::ProxyConnectionRefusedError];
		Error2Suggestions_ [QtNetwork] [QNetworkReply::ContentNotFoundError]
				<< tr ("check if the URL is written correctly;")
				<< tr ("go to web site's <a href=\"{schema}://{host}/\">main page</a> and find the required page from there.");
		Error2Suggestions_ [QtNetwork] [QNetworkReply::AuthenticationRequiredError]
				<< tr ("check the login and password you entered and try again");
		Error2Suggestions_ [QtNetwork] [QNetworkReply::ContentReSendError]
				<< tryAgainLater + ".";
		Error2Suggestions_ [QtNetwork] [QNetworkReply::ProtocolUnknownError]
				<< tr ("check if the URL is written correctly, particularly, the part before the '://';")
				<< tr ("try installing plugins that are known to support this protocol;")
				<< tryAgainLater + ";"
				<< contactSystemAdmin + ".";
		Error2Suggestions_ [QtNetwork] [QNetworkReply::ProtocolInvalidOperationError]
										<< tryAgainLater + ";"
				<< contactRemoteAdmin + ";"
				<< contactSystemAdmin + ".";
		Error2Suggestions_ [QtNetwork] [QNetworkReply::UnknownNetworkError]
										<< tryAgainLater + ";"
				<< contactSystemAdmin + ".";
		Error2Suggestions_ [QtNetwork] [QNetworkReply::UnknownProxyError]
				<< checkProxySettings + ";"
				<< tryAgainLater + ";"
				<< contactSystemAdmin + ".";
		Error2Suggestions_ [QtNetwork] [QNetworkReply::UnknownContentError]
				<< tryAgainLater + ";"
				<< contactSystemAdmin + ".";
		Error2Suggestions_ [QtNetwork] [QNetworkReply::ProtocolFailure]
				<< tryAgainLater + ";"
				<< contactRemoteAdmin + ";"
				<< contactSystemAdmin + ".";

		Error2Suggestions_ [Http] [404] = Error2Suggestions_ [QtNetwork] [QNetworkReply::ContentNotFoundError];
	}

	QString CustomWebPage::MakeErrorReplyContents (int statusCode,
			const QUrl& url, const QString& errorString, ErrorDomain domain) const
	{
		QFile file (":/resources/html/generalerror.html");
		if (!file.open (QIODevice::ReadOnly))
		{
			qCritical () << Q_FUNC_INFO
					<< "unable to open file"
					<< file.fileName ()
					<< file.errorString ();
			return errorString;
		}

		QString data = file.readAll ();
		data.replace ("{title}",
				tr ("Error loading %1")
					.arg (url.toString ()));
		if (statusCode &&
				domain == Http)
			data.replace ("{subtitle}",
					tr ("%1 (%2)")
						.arg (errorString)
						.arg (statusCode));
		else
			data.replace ("{subtitle}",
					tr ("%1")
						.arg (errorString));
		QString bodyContents = tr ("The page you tried to access cannot be loaded now.");

		QStringList suggestions = Error2Suggestions_ [domain] [statusCode];
		QString additionalContents;
		if (suggestions.size ())
		{
			bodyContents += "<br />";
			bodyContents += tr ("Try doing the following:");

			additionalContents += "<ul class=\"suggestionslist\"><li class=\"suggestionitem\">";
			additionalContents += suggestions.join ("</li><li class=\"suggestionitem\">");
			additionalContents += "</li></ul>";
		}
		data.replace ("{body}", bodyContents);
		data.replace ("{additional}", additionalContents);

		if (data.contains ("{host}"))
			data.replace ("{host}", url.host ());
		if (data.contains ("{schema}"))
			data.replace ("{schema}", url.scheme ());

		QBuffer ib;
		ib.open (QIODevice::ReadWrite);
		const auto& px = Proxy_->GetIconThemeManager ()->
				GetIcon ("dialog-error").pixmap (32, 32);
		px.save (&ib, "PNG");

		data.replace ("{img}",
				QByteArray ("data:image/png;base64,") + ib.buffer ().toBase64 ());
		return data;
	}

	void CustomWebPage::handleWindowCloseRequested ()
	{
		emit hookWindowCloseRequested (std::make_shared<Util::DefaultHookProxy> (), this);
	}

	bool CustomWebPage::acceptNavigationRequest (QWebFrame *frame,
			const QNetworkRequest& other, QWebPage::NavigationType type)
	{
		auto proxy = std::make_shared<Util::DefaultHookProxy> ();
		QNetworkRequest request = other;
		emit hookAcceptNavigationRequest (proxy, this, frame, request, type);
		if (proxy->IsCancelled ())
			return proxy->GetReturnValue ().toBool ();

		proxy->FillValue ("request", request);

		QString scheme = request.url ().scheme ();
		if (scheme == "mailto" ||
				scheme == "ftp")
		{
			const auto& e = Util::MakeEntity (request.url (),
					QString (),
					FromUserInitiated);
			auto em = Proxy_->GetEntityManager ();
			if (em->CouldHandle (e))
				em->HandleEntity (e);
			else
				QDesktopServices::openUrl (request.url ());
			return false;
		}

		if (frame)
			HandleForms (frame, request, type);

		if (type == NavigationTypeLinkClicked)
		{
			const auto suggestion = LinkOpenModifier_->GetOpenBehaviourSuggestion ();

			LinkOpenModifier_->ResetSuggestionState ();

			if (suggestion.NewTab_)
			{
				auto view = std::make_shared<CustomWebView> (Proxy_, PoshukuProxy_);
				emit webViewCreated (view, suggestion.Invert_);

				view->Load (request);

				return false;
			}
		}

		if (frame == mainFrame ())
			LoadingURL_ = request.url ();

		return QWebPage::acceptNavigationRequest (frame, request, type);
	}

	QString CustomWebPage::chooseFile (QWebFrame *frame, const QString& thsuggested)
	{
		auto proxy = std::make_shared<Util::DefaultHookProxy> ();
		QString suggested = thsuggested;
		emit hookChooseFile (proxy, this, frame, suggested);
		if (proxy->IsCancelled ())
			return proxy->GetReturnValue ().toString ();

		proxy->FillValue ("suggested", suggested);

		return QWebPage::chooseFile (frame, suggested);
	}

	QObject* CustomWebPage::createPlugin (const QString& thclsid, const QUrl& thurl,
			const QStringList& thnames, const QStringList& thvalues)
	{
		auto proxy = std::make_shared<Util::DefaultHookProxy> ();
		QString clsid = thclsid;
		QUrl url = thurl;
		QStringList names = thnames;
		QStringList values = thvalues;
		emit hookCreatePlugin (proxy, this,
				clsid, url, names, values);
		if (proxy->IsCancelled ())
			return proxy->GetReturnValue ().value<QObject*> ();

		proxy->FillValue ("clsid", clsid);
		proxy->FillValue ("url", url);
		proxy->FillValue ("names", names);
		proxy->FillValue ("values", values);

		return QWebPage::createPlugin (clsid, url, names, values);
	}

	QWebPage* CustomWebPage::createWindow (QWebPage::WebWindowType type)
	{
		auto proxy = std::make_shared<Util::DefaultHookProxy> ();
		emit hookCreateWindow (proxy, this, type);
		if (proxy->IsCancelled ())
			return qobject_cast<QWebPage*> (proxy->GetReturnValue ().value<QObject*> ());

		switch (type)
		{
		case QWebPage::WebBrowserWindow:
		case QWebPage::WebModalDialog:
		{
			const auto view = std::make_shared<CustomWebView> (Proxy_, PoshukuProxy_);
			emit webViewCreated (view, false);
			return view->page ();
		}
		default:
			qWarning () << Q_FUNC_INFO
					<< "unknown type"
					<< type;
			return nullptr;
		}
	}

	void CustomWebPage::javaScriptAlert (QWebFrame *frame, const QString& thmsg)
	{
		auto proxy = std::make_shared<Util::DefaultHookProxy> ();
		QString msg = thmsg;
		emit hookJavaScriptAlert (proxy,
				this, frame, msg);
		if (proxy->IsCancelled ())
			return;

		proxy->FillValue ("message", msg);

		QWebPage::javaScriptAlert (frame, msg);
	}

	bool CustomWebPage::javaScriptConfirm (QWebFrame *frame, const QString& thmsg)
	{
		auto proxy = std::make_shared<Util::DefaultHookProxy> ();
		QString msg = thmsg;
		emit hookJavaScriptConfirm (proxy,
				this, frame, msg);
		if (proxy->IsCancelled ())
			return proxy->GetReturnValue ().toBool ();

		proxy->FillValue ("message", msg);

		return QWebPage::javaScriptConfirm (frame, msg);
	}

	void CustomWebPage::javaScriptConsoleMessage (const QString& thmsg, int line,
			const QString& thsid)
	{
		auto proxy = std::make_shared<Util::DefaultHookProxy> ();
		QString msg = thmsg;
		QString sid = thsid;
		emit hookJavaScriptConsoleMessage (proxy,
				this, msg, line, sid);
		if (proxy->IsCancelled ())
			return;

		proxy->FillValue ("message", msg);
		proxy->FillValue ("line", line);
		proxy->FillValue ("sourceID", sid);

		QWebPage::javaScriptConsoleMessage (msg, line, sid);
	}

	bool CustomWebPage::javaScriptPrompt (QWebFrame *frame, const QString& thpr,
			const QString& thdef, QString *result)
	{
		auto proxy = std::make_shared<Util::DefaultHookProxy> ();
		QString pr = thpr;
		QString def = thdef;
		emit hookJavaScriptPrompt (proxy,
				this, frame, pr, def, *result);
		proxy->FillValue ("result", *result);
		if (proxy->IsCancelled ())
			return proxy->GetReturnValue ().toBool ();

		proxy->FillValue ("message", pr);
		proxy->FillValue ("default", def);

		return QWebPage::javaScriptPrompt (frame, pr, def, result);
	}

	namespace
	{
		QString GetDefaultUserAgent (const ICoreProxy_ptr& proxy)
		{
#if defined(Q_OS_WIN32)
			const auto platform = "Windows";
#elif defined (Q_OS_MAC)
			const auto platform = "Macintosh";
#else
			const auto platform = "X11";
#endif

			const auto& osInfo = Util::SysInfo::GetOSInfo ();
			auto osVersion = osInfo.Flavour_;
			if (!osInfo.Arch_.isEmpty ())
				osVersion += " " + osInfo.Arch_;

			const auto& lcVersion = proxy->GetVersion ();

			return QString { "Mozilla/5.0 (%1; %2) AppleWebKit/%3 (KHTML, like Gecko) Leechcraft/%4 Safari/%3" }
					.arg (platform)
					.arg (osVersion)
					.arg (qWebKitVersion ())
					.arg (lcVersion.section ('-', 0, 0));
		}

	}

	QString CustomWebPage::userAgentForUrl (const QUrl& url) const
	{
		const auto& overridden = PoshukuProxy_->GetUserAgent (url);
		if (!overridden.isEmpty ())
			return overridden;

		return GetDefaultUserAgent (Proxy_);
	}

	QWebFrame* CustomWebPage::FindFrame (const QUrl& url)
	{
		QList<QWebFrame*> frames { mainFrame () };
		while (!frames.isEmpty ())
		{
			const auto frame = frames.takeFirst ();
			if (frame->url () == url)
				return frame;
			frames << frame->childFrames ();
		}
		return nullptr;
	}

	namespace
	{
		bool CheckData (const PageFormsData_t& data,
				QWebFrame *frame,
				const QNetworkRequest& request = QNetworkRequest ())
		{
			if (data.isEmpty ())
			{
				qWarning () << Q_FUNC_INFO
					<< "no form data for"
					<< frame
					<< request.url ();
				return false;
			}
			return true;
		}

		QPair<PageFormsData_t, QMap<ElementData, QWebElement>> HarvestForms (QWebFrame *frame, const QUrl& reqUrl = QUrl ())
		{
			PageFormsData_t formsData;
			QMap<ElementData, QWebElement> ed2element;

			QUrl pageUrl = frame->url ();
			for (const auto& form : frame->findAllElements ("form"))
			{
				QUrl relUrl = QUrl::fromEncoded (form.attribute ("action").toUtf8 ());
				QUrl actionUrl = pageUrl.resolved (relUrl);
				if (reqUrl.isValid () && actionUrl != reqUrl)
					continue;

				QString url = actionUrl.toEncoded ();
				QString formId = QString ("%1<>%2<>%3")
						.arg (url)
						.arg (form.attribute ("id"))
						.arg (form.attribute ("name"));

				for (auto child : form.findAll ("input"))
				{
					QString elemType = child.attribute ("type");
					if (elemType == "hidden" ||
							elemType == "submit")
						continue;

					QString name = child.attribute ("name");
					// Ugly workaround for https://bugs.webkit.org/show_bug.cgi?id=32865
					QString value = child.evaluateJavaScript ("this.value").toString ();

					if (name.isEmpty () ||
							(reqUrl.isValid () && value.isEmpty ()))
						continue;

					ElementData ed
					{
						pageUrl,
						formId,
						name,
						elemType,
						value
					};

					formsData [name] << ed;
					ed2element [ed] = child;
				}
			}

			return { formsData, ed2element };
		}
	};

	void CustomWebPage::HandleForms (QWebFrame *frame,
			const QNetworkRequest& request, QWebPage::NavigationType type)
	{
		if (type != NavigationTypeFormSubmitted)
			return;

		const auto& formsData = HarvestForms (frame ? frame : mainFrame (), request.url ()).first;

		if (!CheckData (formsData, frame, request))
			return;

		if (FilledState_ == formsData)
			return;

		emit storeFormData (formsData);
	}

	namespace
	{
		ElementsData_t::const_iterator FindElement (const ElementData& filled,
				const ElementsData_t& list, bool strict)
		{
			auto typeChecker = [filled] (const ElementData& ed)
				{ return ed.Type_ == filled.Type_; };
			auto urlChecker = [filled] (const ElementData& ed)
				{ return ed.PageURL_ == filled.PageURL_; };
			auto formIdChecker = [filled] (const ElementData& ed)
				{ return ed.FormID_ == filled.FormID_; };

			auto pos = std::find_if (list.begin (), list.end (),
					[typeChecker, urlChecker, formIdChecker] (const ElementData& ed)
						{ return typeChecker (ed) && urlChecker (ed) && formIdChecker (ed); });
			if (!strict)
			{
				if (pos == list.end ())
					pos = std::find_if (list.begin (), list.end (),
							[typeChecker, formIdChecker] (const ElementData& ed)
								{ return typeChecker (ed) && formIdChecker (ed); });
				if (pos == list.end ())
					pos = std::find_if (list.begin (), list.end (),
							[typeChecker, urlChecker] (const ElementData& ed)
								{ return typeChecker (ed) && urlChecker (ed); });
			}

			return pos;
		}
	}

	void CustomWebPage::fillForms (QWebFrame *frame)
	{
		PageFormsData_t formsData;

		auto pair = HarvestForms (frame);

		if (pair.first.isEmpty ())
		{
			FilledState_.clear ();
			return;
		}

		QVariantList values;
		QList<QByteArray> keys;
		const auto& pairFirstKeys = pair.first.keys ();
		for (const auto& name : pairFirstKeys)
		{
			const auto& key = "org.LeechCraft.Poshuku.Forms.InputByName/" + name.toUtf8 ();
			keys << key;
			values.append (Util::GetPersistentData (key, Proxy_));
		}

		const int size = keys.size ();
		if (values.size () != size)
			return;

		for (int i = 0; i < size; ++i)
		{
			QString inputName = QString::fromUtf8 (keys.at (i));
			if (inputName.isEmpty ())
			{
				qWarning () << Q_FUNC_INFO
						<< "empty input.name for"
						<< keys.at (i);
				continue;
			}

			const auto& vars = values.at (i).toList ();
			if (!vars.size ())
				continue;

			const auto list = pair.first [pairFirstKeys.at (i)];
			auto source = list.end ();
			QString value;
			for (const auto& var : vars)
			{
				const ElementData ed = var.value<ElementData> ();
				source = FindElement (ed, list, true);
				if (source != list.end ())
				{
					value = ed.Value_;
					break;
				}
			}
			if (source == list.end ())
				for (const auto& var : vars)
				{
					const auto& ed = var.value<ElementData> ();
					source = FindElement (ed, list, false);
					if (source != list.end ())
					{
						value = ed.Value_;
						break;
					}
				}

			if (source < list.end ())
				pair.second [*source].setAttribute ("value", value);
		}

		for (auto childFrame : frame->childFrames ())
			fillForms (childFrame);

		FilledState_ = HarvestForms (frame ? frame : mainFrame ()).first;
	}
}
}
}
