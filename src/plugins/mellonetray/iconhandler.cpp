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

#include "iconhandler.h"
#include <QWidget>
#include <QLabel>
#include <QPainter>
#include <QStyleOption>
#include <QQuickWindow>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QtDebug>
#include "traymodel.h"

namespace LeechCraft
{
namespace Mellonetray
{
	IconHandler::IconHandler (QQuickItem *item)
	: QQuickItem (item)
	{
		setFlag (QQuickItem::ItemHasContents);
	}

	IconHandler::~IconHandler ()
	{
		Free ();
	}

	ulong IconHandler::GetWID () const
	{
		return WID_;
	}

	void IconHandler::SetWID (const ulong& wid)
	{
		if (wid == WID_)
			return;

		Free ();

		WID_ = wid;
		emit widChanged ();
	}

	void IconHandler::geometryChanged (const QRectF& rect, const QRectF& oldRect)
	{
		QQuickItem::geometryChanged (rect, oldRect);

		if (!window ())
			return;

		if (!Proxy_ && WID_)
		{
			Proxy_.reset (QWindow::fromWinId (WID_));
			Proxy_->setPosition (-1024, -1024);
			Proxy_->show ();
		}

		if (Proxy_ && rect.width () * rect.height () > 0)
		{
			Proxy_->resize (rect.width (), rect.height ());

			const auto& scenePoint = mapToScene ({ 0, 0 }).toPoint ();
			Proxy_->setPosition (window ()->mapToGlobal (scenePoint));
		}
	}

	void IconHandler::Free ()
	{
		if (Proxy_)
			Proxy_.reset ();
	}
}
}
