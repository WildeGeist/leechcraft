/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2009  Georg Rudoy
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 **********************************************************************/

#include "pluginmanager.h"
#include <stdexcept>
#include <QtDebug>
#include "proxyobject.h"
#include "core.h"
#include "customwebpage.h"

namespace LeechCraft
{
namespace Poshuku
{
	PluginManager::PluginManager (QObject *parent)
	: Util::BaseHookInterconnector (parent)
	, ProxyObject_ (new ProxyObject)
	{
	}

	void PluginManager::AddPlugin (QObject *plugin)
	{
		if (plugin->metaObject ()->indexOfMethod (QMetaObject::normalizedSignature ("initPlugin (QObject*)")) != -1)
			QMetaObject::invokeMethod (plugin,
					"initPlugin",
					Q_ARG (QObject*, ProxyObject_.get ()));

		Util::BaseHookInterconnector::AddPlugin (plugin);
	}
}
}
