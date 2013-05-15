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

#include "laughty.h"
#include "serverobject.h"
#include "serveradaptor.h"
#include <QIcon>
#include <QDBusConnection>

namespace LeechCraft
{
namespace Laughty
{
	void Plugin::Init (ICoreProxy_ptr proxy)
	{
		if (!QDBusConnection::sessionBus ().registerService ("org.freedesktop.Notifications"))
		{
			qWarning () << Q_FUNC_INFO
					<< "unable to register Notifications service."
					<< "Is another notification daemon active?";
			return;
		}

		auto server = new ServerObject (proxy);
		new ServerAdaptor (server);
		QDBusConnection::sessionBus ().registerObject ("/org/freedesktop/Notifications", server);
	}

	void Plugin::SecondInit ()
	{
	}

	QByteArray Plugin::GetUniqueID () const
	{
		return "org.LeechCraft.Laughty";
	}

	void Plugin::Release ()
	{
	}

	QString Plugin::GetName () const
	{
		return "Laughty";
	}

	QString Plugin::GetInfo () const
	{
		return tr ("Desktop Notifications server.");
	}

	QIcon Plugin::GetIcon () const
	{
		return QIcon ();
	}
}
}

LC_EXPORT_PLUGIN (leechcraft_laughty, LeechCraft::Laughty::Plugin);
