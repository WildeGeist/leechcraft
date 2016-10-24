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

#include "core.h"
#include <algorithm>
#include <memory>
#include <typeinfo>
#include <stdexcept>
#include <QString>
#include <QUrl>
#include <QWidget>
#include <QTextCodec>
#include <QIcon>
#include <QFile>
#include <QFileInfo>
#include <QNetworkCookieJar>
#include <QDir>
#include <QMenu>
#include <QInputDialog>
#include <QNetworkReply>
#include <QSslSocket>
#include <QAuthenticator>
#include <QNetworkProxy>
#include <QDesktopServices>
#include <QFileDialog>
#include <QMessageBox>
#include <QtDebug>
#include <QMainWindow>
#include <util/xpc/util.h>
#include <util/xpc/defaulthookproxy.h>
#include <util/xpc/introspectable.h>
#include <interfaces/ihaveshortcuts.h>
#include <interfaces/core/icoreproxy.h>
#include <interfaces/core/ipluginsmanager.h>
#include <interfaces/core/irootwindowsmanager.h>
#include "interfaces/poshuku/iwebview.h"
#include "interfaces/poshuku/iwebviewprovider.h"
#include "browserwidget.h"
#include "xmlsettingsmanager.h"
#include "sqlstoragebackend.h"
#include "xbelparser.h"
#include "xbelgenerator.h"
#include "favoriteschecker.h"
#include "importentity.h"

namespace LeechCraft
{
namespace Poshuku
{
	using LeechCraft::Util::TagsCompletionModel;

	Core::Core ()
	: PluginManager_ (new PluginManager (this))
	, URLCompletionModel_ (new URLCompletionModel (this))
	, HistoryModel_ (new HistoryModel (this))
	, FavoritesModel_ (new FavoritesModel (this))
	{
		qRegisterMetaType<BrowserWidgetSettings> ("LeechCraft::Poshuku::BrowserWidgetSettings");
		qRegisterMetaTypeStreamOperators<BrowserWidgetSettings> ("LeechCraft::Poshuku::BrowserWidgetSettings");

		qRegisterMetaType<ElementData> ("LeechCraft::Poshuku::ElementData");
		qRegisterMetaTypeStreamOperators<ElementData> ("LeechCraft::Poshuku::ElementData");
		qRegisterMetaType<ElementsData_t> ("LeechCraft::Poshuku::ElementsData_t");
		qRegisterMetaTypeStreamOperators<ElementsData_t> ("LeechCraft::Poshuku::ElementsData_t");

		Util::Introspectable::Instance ().Register<ElementData> (&ToVariantMap);

		TabClass_.TabClass_ = "Poshuku";
		TabClass_.VisibleName_ = tr ("Poshuku");
		TabClass_.Description_ = tr ("The Poshuku web browser");
		TabClass_.Icon_ = QIcon ("lcicons:/resources/images/poshuku.svg");
		TabClass_.Priority_ = 80;
		TabClass_.Features_ = TFOpenableByRequest | TFSuggestOpening;

		PluginManager_->RegisterHookable (this);
		PluginManager_->RegisterHookable (URLCompletionModel_);
		PluginManager_->RegisterHookable (HistoryModel_);
		PluginManager_->RegisterHookable (FavoritesModel_);
	}

	Core& Core::Instance ()
	{
		static Core core;
		return core;
	}

	void Core::Init ()
	{
		QDir dir = QDir::home ();
		if (!dir.cd (".leechcraft/poshuku") &&
				!dir.mkpath (".leechcraft/poshuku"))
		{
			qCritical () << Q_FUNC_INFO
				<< "could not create necessary directories for Poshuku";
			throw std::runtime_error ("could not create necessary directories for Poshuku");
		}

		try
		{
			StorageBackend_ = StorageBackend::Create ();
		}
		catch (const std::runtime_error& s)
		{
			emit error (QTextCodec::codecForName ("UTF-8")->
					toUnicode (s.what ()));
			throw;
		}
		catch (...)
		{
			emit error (tr ("Poshuku: general storage initialization error."));
			throw;
		}

		connect (StorageBackend_.get (),
				SIGNAL (added (const HistoryItem&)),
				HistoryModel_,
				SLOT (handleItemAdded (const HistoryItem&)));

		connect (StorageBackend_.get (),
				SIGNAL (added (const HistoryItem&)),
				URLCompletionModel_,
				SLOT (handleItemAdded (const HistoryItem&)));

		connect (StorageBackend_.get (),
				SIGNAL (added (const FavoritesModel::FavoritesItem&)),
				FavoritesModel_,
				SLOT (handleItemAdded (const FavoritesModel::FavoritesItem&)));
		connect (StorageBackend_.get (),
				SIGNAL (updated (const FavoritesModel::FavoritesItem&)),
				FavoritesModel_,
				SLOT (handleItemUpdated (const FavoritesModel::FavoritesItem&)));
		connect (StorageBackend_.get (),
				SIGNAL (removed (const FavoritesModel::FavoritesItem&)),
				FavoritesModel_,
				SLOT (handleItemRemoved (const FavoritesModel::FavoritesItem&)));

		Initialized_ = true;
	}

	void Core::Release ()
	{
		while (Widgets_.begin () != Widgets_.end ())
			delete *Widgets_.begin ();

		StorageBackend_.reset ();

		XmlSettingsManager::Instance ()->setProperty ("CleanShutdown", true);
		XmlSettingsManager::Instance ()->Release ();
	}

	void Core::SetProxy (ICoreProxy_ptr proxy)
	{
		Proxy_ = proxy;
		NetworkAccessManager_ = proxy->GetNetworkAccessManager ();
	}

	ICoreProxy_ptr Core::GetProxy () const
	{
		return Proxy_;
	}

	void Core::SetShortcutManager (Util::ShortcutManager *sm)
	{
		ShortcutMgr_ = sm;
	}

	TabClassInfo Core::GetTabClass () const
	{
		return TabClass_;
	}

	bool Core::CouldHandle (const Entity& e) const
	{
		if (!(e.Parameters_ & FromUserInitiated) ||
				e.Parameters_ & Internal)
			return false;

		if (e.Mime_ == "x-leechcraft/browser-import-data")
			return true;
		else if (e.Entity_.canConvert<QUrl> ())
		{
			QUrl url = e.Entity_.toUrl ();
			return (url.isValid () &&
				(url.scheme () == "http" || url.scheme () == "https"));
		}

		return false;
	}

	void Core::Handle (Entity e)
	{
		if (e.Mime_ == "x-leechcraft/browser-import-data")
			ImportEntity (e, HistoryModel_, FavoritesModel_,
					Proxy_->GetRootWindowsManager ());
		else if (e.Entity_.canConvert<QUrl> ())
		{
			QUrl url = e.Entity_.toUrl ();
			NewURL (url, !e.Additional_ ["BackgroundHandle"].toBool ());
		}
	}

	QSet<QByteArray> Core::GetExpectedPluginClasses () const
	{
		QSet<QByteArray> result;
		result << "org.LeechCraft.Poshuku.Plugins/1.0";
		return result;
	}

	void Core::AddPlugin (QObject *plugin)
	{
		PluginManager_->AddPlugin (plugin);

		if (const auto iwvp = qobject_cast<IWebViewProvider*> (plugin))
		{
			WebViewProviders_ << iwvp;
			connect (plugin,
					SIGNAL (webViewCreated (IWebView*, bool)),
					this,
					SLOT (handleWebViewCreated (IWebView*, bool)));
		}
	}

	QUrl Core::MakeURL (QString url)
	{
		if (url.isEmpty ())
			return {};

		url = url.trimmed ();
		if (url == "localhost")
			return { "http://localhost" };

		if (url.startsWith ('!'))
		{
			HandleSearchRequest (url);
			return {};
		}

		QHostAddress haddr;
		if (haddr.setAddress (url) &&
				(haddr.protocol () != QAbstractSocket::IPv4Protocol || url.count ('.') == 3))
		{
			QUrl result;
			result.setHost (url);
			result.setScheme ("http://");
			return result;
		}

		// If the url without percent signs and two following characters is
		// a valid url (it should not be percent-encoded), then treat source
		// url as percent-encoded, otherwise treat as not percent-encoded.
		const auto isPercentEncoded = url.contains (QRegExp { "%??", Qt::CaseInsensitive, QRegExp::Wildcard });
		auto result = isPercentEncoded ?
				QUrl::fromEncoded (url.toUtf8 ()) :
				QUrl { url };

		if (result.scheme ().isEmpty ())
		{
			if (!url.count (' ') && url.count ('.'))
				result = QUrl { "http://" + url };
			else
			{
				url = QString::fromUtf8 (url.toUtf8 ().toPercentEncoding ({ "+ " }));
				url.replace ('+', "%2B");
				url.replace (' ', '+');
				auto urlStr = QString { "http://www.google.com/search?q=%2"
						"&client=leechcraft_poshuku"
						"&ie=utf-8"
						"&rls=org.leechcraft:%1" }
					.arg (QLocale::system ().name ().replace ('_', '-'))
					.arg (url);
				result = QUrl::fromEncoded (urlStr.toUtf8 ());
			}
		}
		else if (result.host ().isEmpty ())
		{
			bool isHostNum = false;
			auto num = result.path ().toInt (&isHostNum);
			if (isHostNum)
			{
				QMap<int, QString> port2scheme;
				port2scheme [443] = "https";

				result.setPort (num);
				result.setHost (result.scheme ());
				result.setScheme (port2scheme.value (num, "http"));
				result.setPath (QString ());
			}
		}

		return result;
	}

	BrowserWidget* Core::CreateBrowserWidget (IWebView *view, const QUrl& url,
			bool raise, const QList<QPair<QByteArray, QVariant>>& props)
	{
		const auto widget = new BrowserWidget { view, ShortcutMgr_ };
		emit browserWidgetCreated (widget);
		widget->FinalizeInit ();
		Widgets_.push_back (widget);

		for (const auto& pair : props)
			widget->setProperty (pair.first, pair.second);

		QString tabTitle = "Poshuku";
		if (url.host ().size ())
			tabTitle = url.host ();
		emit addNewTab (tabTitle, widget);

		ConnectSignals (widget);

		if (!url.isEmpty ())
			widget->SetURL (url);

		if (raise)
			emit raiseTab (widget);

		emit hookTabAdded (std::make_shared<Util::DefaultHookProxy> (),
				widget,
				url);

		return widget;
	}

	BrowserWidget* Core::NewURL (const QUrl& url, bool raise,
			const QList<QPair<QByteArray, QVariant>>& props)
	{
		if (!Initialized_)
			return nullptr;

		return CreateBrowserWidget (CreateWebView (), url, raise, props);
	}

	BrowserWidget* Core::NewURL (const QString& str, bool raise)
	{
		return NewURL (MakeURL (str), raise);
	}

	IWebWidget* Core::GetWidget ()
	{
		if (!Initialized_)
			return nullptr;

		const auto widget = new BrowserWidget { CreateWebView (), ShortcutMgr_ };
		emit browserWidgetCreated (widget);
		widget->Deown ();
		widget->FinalizeInit ();
		SetupConnections (widget);
		return widget;
	}

	namespace
	{
		bool ShouldRaise (bool invert)
		{
			return invert == XmlSettingsManager::Instance ()->property ("BackgroundNewTabs").toBool ();
		}
	}

	IWebView* Core::MakeWebView (bool invert)
	{
		if (!Initialized_)
			return nullptr;

		return NewURL (QUrl {}, ShouldRaise (invert))->GetWebView ();
	}

	void Core::ConnectSignals (BrowserWidget *widget)
	{
		SetupConnections (widget);
		connect (widget,
				SIGNAL (titleChanged (const QString&)),
				this,
				SLOT (handleTitleChanged (const QString&)));
		connect (widget,
				SIGNAL (iconChanged (const QIcon&)),
				this,
				SLOT (handleIconChanged (const QIcon&)));
		connect (widget,
				SIGNAL (needToClose ()),
				this,
				SLOT (handleNeedToClose ()));
		connect (widget,
				SIGNAL (statusBarChanged (const QString&)),
				this,
				SLOT (handleStatusBarChanged (const QString&)));
		connect (widget,
				SIGNAL (tooltipChanged (QWidget*)),
				this,
				SLOT (handleTooltipChanged (QWidget*)));
		connect (widget,
				SIGNAL (raiseTab (QWidget*)),
				this,
				SIGNAL (raiseTab (QWidget*)));
	}

	void Core::CheckFavorites ()
	{
		const auto checker = new FavoritesChecker (this);
		checker->Check ();
	}

	void Core::ReloadAll ()
	{
		for (const auto widget : Widgets_)
			if (const auto act = widget->GetWebView ()->GetPageAction (IWebView::PageAction::Reload))
				act->trigger ();
	}

	FavoritesModel* Core::GetFavoritesModel () const
	{
		return FavoritesModel_;
	}

	HistoryModel* Core::GetHistoryModel () const
	{
		return HistoryModel_;
	}

	URLCompletionModel* Core::GetURLCompletionModel () const
	{
		return URLCompletionModel_;
	}

	QNetworkAccessManager* Core::GetNetworkAccessManager () const
	{
		return NetworkAccessManager_;
	}

	StorageBackend* Core::GetStorageBackend () const
	{
		return StorageBackend_.get ();
	}

	PluginManager* Core::GetPluginManager () const
	{
		return PluginManager_;
	}

	QIcon Core::GetIcon (const QUrl& url) const
	{
		Util::DefaultHookProxy_ptr proxy (new Util::DefaultHookProxy ());
		emit hookIconRequested (proxy, url);
		if (proxy->IsCancelled ())
			return proxy->GetReturnValue ().value<QIcon> ();

		for (const auto prov : WebViewProviders_)
		{
			const auto& icon = prov->GetIconForUrl (url);
			if (!icon.isNull ())
				return icon;
		}

		for (const auto prov : WebViewProviders_)
		{
			const auto& icon = prov->GetDefaultUrlIcon ();
			if (!icon.isNull ())
				return icon;
		}

		return {};
	}

	QString Core::GetUserAgent (const QUrl& url) const
	{
		Util::DefaultHookProxy_ptr proxy (new Util::DefaultHookProxy ());
		emit hookUserAgentForUrlRequested (proxy, url);
		if (proxy->IsCancelled ())
			return proxy->GetReturnValue ().toString ();

		return QString ();
	}

	bool Core::IsUrlInFavourites (const QString& url)
	{
		return FavoritesModel_->IsUrlExists (url);
	}

	void Core::RemoveFromFavorites (const QString& url)
	{
		emit bookmarkRemoved (url);
	}

	void Core::Unregister (BrowserWidget *widget)
	{
		auto pos = std::find (Widgets_.begin (), Widgets_.end (), widget);
		if (pos == Widgets_.end ())
		{
			qWarning () << Q_FUNC_INFO << widget << "not found in the collection";
			return;
		}

		Widgets_.erase (pos);
	}

	void Core::HandleHistory (IWebView *view)
	{
		const auto& url = view->GetHumanReadableUrl ();

		if (!view->GetTitle ().isEmpty () &&
				!url.isEmpty () && url != "about:blank")
			HistoryModel_->addItem (view->GetTitle (),
					url,
					QDateTime::currentDateTime ());
	}

	void Core::SetupConnections (BrowserWidget *widget)
	{
		connect (widget,
				SIGNAL (addToFavorites (const QString&, const QString&)),
				this,
				SLOT (handleAddToFavorites (const QString&, const QString&)));
		connect (widget,
				SIGNAL (gotEntity (const LeechCraft::Entity&)),
				this,
				SIGNAL (gotEntity (const LeechCraft::Entity&)));
		connect (widget,
				SIGNAL (delegateEntity (const LeechCraft::Entity&, int*, QObject**)),
				this,
				SIGNAL (delegateEntity (const LeechCraft::Entity&, int*, QObject**)));
		connect (widget,
				SIGNAL (couldHandle (const LeechCraft::Entity&, bool*)),
				this,
				SIGNAL (couldHandle (const LeechCraft::Entity&, bool*)));
		connect (widget,
				SIGNAL (urlChanged (QUrl)),
				this,
				SLOT (handleURLChanged ()));
	}

	void Core::HandleSearchRequest (const QString& url)
	{
		const int pos = url.indexOf (' ');
		const QString& category = url.mid (1, pos - 1);
		const QString& query = url.mid (pos + 1);

		Entity e;
		if (XmlSettingsManager::Instance ()->property ("UseSummaryForSearches").toBool ())
		{
			e = Util::MakeEntity (query,
					QString (),
					FromUserInitiated,
					"x-leechcraft/category-search-request");
			e.Additional_ ["Categories"] = QStringList (category);
		}
		else
		{
			e = Util::MakeEntity (query,
					QString (),
					FromUserInitiated | OnlyHandle,
					"x-leechcraft/data-filter-request");
			e.Additional_ ["DataFilter"] = category.toUtf8 ();
		}
		emit gotEntity (e);
	}

	IWebView* Core::CreateWebView ()
	{
		return WebViewProviders_.value (0)->CreateWebView ();
	}

	void Core::importXbel ()
	{
		QString suggestion = XmlSettingsManager::Instance ()->
				Property ("LastXBELOpen", QDir::homePath ()).toString ();
		QString filename = QFileDialog::getOpenFileName (0,
				tr ("Select XBEL file"),
				suggestion,
				tr ("XBEL files (*.xbel);;"
					"All files (*.*)"));

		if (filename.isEmpty ())
			return;

		XmlSettingsManager::Instance ()->setProperty ("LastXBELOpen",
				QFileInfo (filename).absolutePath ());

		QFile file (filename);
		if (!file.open (QIODevice::ReadOnly))
		{
			auto rootWM = Core::Instance ().GetProxy ()->GetRootWindowsManager ();
			QMessageBox::critical (rootWM->GetPreferredWindow (),
					"LeechCraft",
					tr ("Could not open file %1 for reading.")
						.arg (filename));
			return;
		}

		QByteArray data = file.readAll ();

		try
		{
			XbelParser p (data);
		}
		catch (const std::exception& e)
		{
			auto rootWM = Core::Instance ().GetProxy ()->GetRootWindowsManager ();
			QMessageBox::critical (rootWM->GetPreferredWindow (),
					"LeechCraft",
					e.what ());
		}
	}

	void Core::exportXbel ()
	{
		QString suggestion = XmlSettingsManager::Instance ()->
				Property ("LastXBELSave", QDir::homePath ()).toString ();
		QString filename = QFileDialog::getSaveFileName (0,
				tr ("Save XBEL file"),
				suggestion,
				tr ("XBEL files (*.xbel);;"
					"All files (*.*)"));

		if (filename.isEmpty ())
			return;

		if (!filename.endsWith (".xbel"))
			filename.append (".xbel");

		XmlSettingsManager::Instance ()->setProperty ("LastXBELSave",
				QFileInfo (filename).absolutePath ());

		QFile file (filename);
		if (!file.open (QIODevice::WriteOnly | QIODevice::Truncate))
		{
			auto rootWM = Core::Instance ().GetProxy ()->GetRootWindowsManager ();
			QMessageBox::critical (rootWM->GetPreferredWindow (),
					"LeechCraft",
					tr ("Could not open file %1 for writing.")
						.arg (filename));
			return;
		}

		QByteArray data;
		XbelGenerator g (data);
		file.write (data);
	}

	void Core::handleTitleChanged (const QString& newTitle)
	{
		emit changeTabName (dynamic_cast<QWidget*> (sender ()), newTitle);
	}

	void Core::handleURLChanged ()
	{
		HandleHistory (dynamic_cast<BrowserWidget*> (sender ())->GetWebView ());
	}

	void Core::handleIconChanged (const QIcon& newIcon)
	{
		emit changeTabIcon (dynamic_cast<QWidget*> (sender ()), newIcon);
	}

	void Core::handleNeedToClose ()
	{
		BrowserWidget *w = dynamic_cast<BrowserWidget*> (sender ());
		emit removeTab (w);

		w->deleteLater ();
	}

	void Core::handleAddToFavorites (QString title, QString url)
	{
		Util::DefaultHookProxy_ptr proxy = Util::DefaultHookProxy_ptr (new Util::DefaultHookProxy ());
		emit hookAddToFavoritesRequested (proxy, title, url);
		if (proxy->IsCancelled ())
			return;

		proxy->FillValue ("title", title);
		proxy->FillValue ("url", url);

		bool oneClick = XmlSettingsManager::Instance ()->property ("BookmarkInOneClick").toBool ();

		const auto& index = FavoritesModel_->addItem (title, url, QStringList ());

		if (!oneClick)
			FavoritesModel_->EditBookmark (index);

		emit bookmarkAdded (url);
	}

	void Core::handleStatusBarChanged (const QString& msg)
	{
		emit statusBarChanged (static_cast<QWidget*> (sender ()), msg);
	}

	void Core::handleTooltipChanged (QWidget *tip)
	{
		emit changeTooltip (static_cast<QWidget*> (sender ()), tip);
	}

	void Core::handleWebViewCreated (IWebView *view, bool invert)
	{
		CreateBrowserWidget (view, {}, ShouldRaise (invert), {});
	}

	void Core::favoriteTagsUpdated (const QStringList& tags)
	{
		XmlSettingsManager::Instance ()->setProperty ("FavoriteTags", tags);
	}
}
}
