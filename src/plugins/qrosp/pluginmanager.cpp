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

#include "pluginmanager.h"
#include <QDir>
#include <QModelIndex>
#include <QtDebug>
#include <qross/core/manager.h>
#include <qross/core/interpreter.h>
#include <util/sll/qtutil.h>
#include "wrapperobject.h"
#include "typesfactory.h"
#include "utilproxy.h"
#include "wrappers/entitywrapper.h"

namespace LeechCraft
{
namespace Qrosp
{
	PluginManager::PluginManager ()
	{
		Qross::Manager::self ().registerMetaTypeHandler ("LeechCraft::Entity", EntityHandler);
		Qross::Manager::self ().addQObject (new TypesFactory, "TypesFactory");
		Qross::Manager::self ().addQObject (new UtilProxy, "Util");

		const auto& interpreters = Qross::Manager::self ().interpreters ();
		qDebug () << Q_FUNC_INFO
				<< "interpreters:"
				<< interpreters;

		const auto& plugins = FindPlugins ();
		qDebug () << Q_FUNC_INFO
				<< "found"
				<< plugins;

		for (const auto& pair : Util::Stlize (plugins))
		{
			const auto& type = pair.first;
			if (!interpreters.contains (type))
			{
				qWarning () << Q_FUNC_INFO
						<< "no interpreter for type"
						<< type
						<< interpreters;
				continue;
			}
			for (const auto& path : pair.second)
				Wrappers_ << new WrapperObject (type, path);
		}
	}

	PluginManager& PluginManager::Instance ()
	{
		static PluginManager pm;
		return pm;
	}

	void PluginManager::Release ()
	{
		Qross::Manager::self ().finalize ();
	}

	QList<QObject*> PluginManager::GetPlugins ()
	{
		return Wrappers_;
	}

	QMap<QString, QStringList> PluginManager::FindPlugins ()
	{
		QMap<QString, QStringList> knownExtensions;
		knownExtensions ["qtscript"] << "*.es" << "*.js" << "*.qs";
		knownExtensions ["python"] << "*.py";
		knownExtensions ["ruby"] << "*.rb";

		QMap<QString, QStringList> result;

		struct Collector
		{
			const QStringList& Extensions_;

			Collector (const QStringList& exts)
			: Extensions_ (exts)
			{
			}

			QFileInfoList operator() (const QString& path)
			{
				QFileInfoList list;
				QDir dir = QDir::home ();
				if (dir.cd (path))
					list = dir.entryInfoList (Extensions_,
							QDir::Files |
								QDir::NoDotAndDotDot |
								QDir::Readable,
							QDir::Name);
				return list;
			}
		};

		QDir dir = QDir::home ();
		if (!dir.cd (".leechcraft/plugins/scriptable"))
			qWarning () << Q_FUNC_INFO
					<< "unable to cd into ~/.leechcraft/plugins/scriptable";
		else
		{
			const auto& dirEntries = dir.entryInfoList (QDir::Dirs |
					QDir::NoDotAndDotDot |
					QDir::Readable);
			// Iterate over the different types of scripts
			for (const auto& sameType : dirEntries)
			{
				const auto& pluginDirs = QDir { sameType.absoluteFilePath () }
						.entryInfoList (QDir::Dirs |
								QDir::NoDotAndDotDot |
								QDir::Readable);
				// For the same type iterate over subdirs with
				// actual plugins.
				for (const auto& pluginDir : pluginDirs)
				{
					const auto& type = sameType.fileName ();
					const auto& exts = knownExtensions.value (type, { "*.*" });
					const auto& list = Collector (exts) (pluginDir.absoluteFilePath ());
					for (const auto& fileInfo : list)
						if (fileInfo.baseName () == pluginDir.baseName ())
							result [type] += fileInfo.absoluteFilePath ();
				}
			}
		}

		return result;
	}
}
}
