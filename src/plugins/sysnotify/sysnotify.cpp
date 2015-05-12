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

#include "sysnotify.h"
#include <QIcon>
#include <util/util.h>
#include <interfaces/entitytesthandleresult.h>
#include "notificationmanager.h"

namespace LeechCraft
{
namespace Sysnotify
{
	void Plugin::Init (ICoreProxy_ptr)
	{
		Util::InstallTranslator ("sysnotify");

		Manager_.reset (new NotificationManager (this));
	}

	void Plugin::SecondInit ()
	{
	}

	QByteArray Plugin::GetUniqueID () const
	{
		return "org.LeechCraft.Sysnotify";
	}

	void Plugin::Release ()
	{
		Manager_.reset ();
	}

	QString Plugin::GetName () const
	{
		return "Sysnotify";
	}

	QString Plugin::GetInfo () const
	{
		return tr ("Notifies about LeechCraft events via Desktop Notifications.");
	}

	QIcon Plugin::GetIcon () const
	{
		return QIcon ();
	}

	EntityTestHandleResult Plugin::CouldHandle (const Entity& e) const
	{
		return EntityTestHandleResult { Manager_->get () && Manager_->CouldNotify (e) ?
					EntityTestHandleResult::PHigh :
					EntityTestHandleResult::PNone };
	}

	void Plugin::Handle (Entity e)
	{
		Manager_->HandleNotification (e);
	}
}
}

LC_EXPORT_PLUGIN (leechcraft_sysnotify, LeechCraft::Sysnotify::Plugin);
