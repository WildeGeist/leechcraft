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

#include "quarkproxy.h"
#include <QGraphicsObject>
#include <QToolTip>
#include <QApplication>
#include <QToolBar>
#include <QMainWindow>
#include <QtDebug>
#include <interfaces/iquarkcomponentprovider.h>
#include <util/gui/util.h>
#include <util/gui/autoresizemixin.h>
#include "viewmanager.h"
#include "sbview.h"
#include "quarkunhidelistview.h"
#include "quarkorderview.h"
#include "declarativewindow.h"

namespace LeechCraft
{
namespace SB2
{
	QuarkProxy::QuarkProxy (ViewManager *mgr, ICoreProxy_ptr proxy, QObject *parent)
	: QObject (parent)
	, Manager_ (mgr)
	, Proxy_ (proxy)
	{
	}

	const QString& QuarkProxy::GetExtHoveredQuarkClass () const
	{
		return ExtHoveredQuarkClass_;
	}

	QRect QuarkProxy::GetFreeCoords () const
	{
		return Manager_->GetFreeCoords ();
	}

	QPoint QuarkProxy::mapToGlobal (double x, double y)
	{
		return Manager_->GetView ()->mapToGlobal (QPoint (x, y));
	}

	void QuarkProxy::showTextTooltip (int x, int y, const QString& str)
	{
		QToolTip::showText ({ x, y }, str);
	}

	void QuarkProxy::showSettings (const QUrl& url)
	{
		Manager_->ShowSettings (url);
	}

	void QuarkProxy::removeQuark (const QUrl& url)
	{
		Manager_->RemoveQuark (url);
	}

	QVariant QuarkProxy::openWindow (const QUrl& url, const QString& str, const QVariant& var)
	{
		const auto& newUrl = url.resolved (str);

		auto varMap = var.toMap ();

		const auto& existing = varMap.take ("existing").toString ();

		if ((existing == "toggle" || existing.isEmpty ()) &&
				URL2LastOpened_.value (newUrl))
		{
			URL2LastOpened_.take (newUrl)->deleteLater ();
			return QVariant ();
		}

		int x = varMap.take ("x").toInt ();
		int y = varMap.take ("y").toInt ();

		auto window = new DeclarativeWindow (newUrl, varMap, { x, y }, Manager_, Proxy_);
		window->show ();

		URL2LastOpened_ [newUrl] = window;

		return QVariant::fromValue<QObject*> (window->rootObject ());
	}

	QRect QuarkProxy::getWinRect ()
	{
		return GetFreeCoords ();
	}

	void QuarkProxy::quarkAddRequested (int x, int y)
	{
		auto toAdd = Manager_->FindAllQuarks ();
		for (const auto& existing : Manager_->GetAddedQuarks ())
		{
			const auto pos = std::find_if (toAdd.begin (), toAdd.end (),
					[&existing] (decltype (toAdd.at (0)) item) { return item->Url_ == existing; });
			if (pos == toAdd.end ())
				continue;

			toAdd.erase (pos);
		}

		if (toAdd.isEmpty ())
			return;

		auto unhide = new QuarkUnhideListView (toAdd, Manager_, Proxy_, Manager_->GetView ());
		new Util::AutoResizeMixin ({ x, y }, [this] () { return Manager_->GetFreeCoords (); }, unhide);
		unhide->show ();
	}

	void QuarkProxy::quarkOrderRequested (int x, int y)
	{
		if (QuarkOrderView_)
		{
			QuarkOrderView_->deleteLater ();
			return;
		}

		QuarkOrderView_ = new QuarkOrderView (Manager_, Proxy_);
		QuarkOrderView_->move (Util::FitRect ({ x, y }, QuarkOrderView_->size (), GetFreeCoords (),
				Util::FitFlag::NoOverlap));
		QuarkOrderView_->show ();

		connect (QuarkOrderView_,
				SIGNAL (quarkClassHovered (QString)),
				this,
				SLOT (handleExtHoveredQuarkClass (QString)));
	}

	void QuarkProxy::handleExtHoveredQuarkClass (const QString& qClass)
	{
		if (ExtHoveredQuarkClass_ == qClass)
			return;

		ExtHoveredQuarkClass_ = qClass;
		emit extHoveredQuarkClassChanged ();
	}
}
}
