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

#include <functional>
#include <QAbstractItemModel>
#include <QIcon>
#include <util/x11/winflags.h>
#include <util/models/rolenamesmixin.h>

#if QT_VERSION < 0x050000
class QDeclarativeImageProvider;
#else
class QQuickImageProvider;
#endif

namespace LeechCraft
{
namespace Util
{
class XWrapper;
}

namespace Krigstask
{
	class TaskbarImageProvider;

	class WindowsModel : public Util::RoleNamesMixin<QAbstractItemModel>
	{
		Q_OBJECT

		struct WinInfo
		{
			ulong WID_;

			QString Title_;
			QIcon Icon_;
			int IconGenID_;
			bool IsActive_;

			int DesktopNum_;

			Util::WinStateFlags State_;
			Util::AllowedActionFlags Actions_;
		};
		QList<WinInfo> Windows_;

		int CurrentDesktop_;

		enum Role
		{
			WindowName = Qt::UserRole + 1,
			WindowID,
			IconGenID,
			IsCurrentDesktop,
			IsActiveWindow,
			IsMinimizedWindow
		};

		TaskbarImageProvider *ImageProvider_;
	public:
		WindowsModel (QObject* = 0);

#if QT_VERSION < 0x050000
		QDeclarativeImageProvider* GetImageProvider () const;
#else
		QQuickImageProvider* GetImageProvider () const;
#endif

		int columnCount (const QModelIndex& parent = QModelIndex()) const;
		int rowCount (const QModelIndex& parent = QModelIndex()) const;
		QModelIndex index (int row, int column, const QModelIndex& parent = QModelIndex()) const;
		QModelIndex parent (const QModelIndex& child) const;
		QVariant data (const QModelIndex& index, int role = Qt::DisplayRole) const;
	private:
		void AddWindow (ulong, Util::XWrapper&);

		QList<WinInfo>::iterator FindWinInfo (ulong);
		void UpdateWinInfo (ulong, std::function<void (WinInfo&)>);
	private slots:
		void updateWinList ();
		void updateActiveWindow ();

		void updateWindowName (ulong);
		void updateWindowIcon (ulong);
		void updateWindowState (ulong);
		void updateWindowActions (ulong);
		void updateWindowDesktop (ulong);
		void updateCurrentDesktop ();
	};
}
}
