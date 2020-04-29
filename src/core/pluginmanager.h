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
#include <QAbstractItemModel>
#include <QMap>
#include <QMultiMap>
#include <QStringList>
#include <QDir>
#include <QIcon>
#include "loaders/ipluginloader.h"
#include "interfaces/iinfo.h"
#include "interfaces/core/ipluginsmanager.h"

namespace LC
{
	class MainWindow;
	class PluginTreeBuilder;

	class PluginManager : public QAbstractItemModel
						, public IPluginsManager
	{
		Q_OBJECT
		Q_INTERFACES (IPluginsManager)

		const bool DBusMode_;

		typedef QList<Loaders::IPluginLoader_ptr> PluginsContainer_t;

		// Only currently loaded plugins
		mutable PluginsContainer_t PluginContainers_;
		typedef QList<QObject*> Plugins_t;
		mutable Plugins_t Plugins_;

		QMap<QObject*, Loaders::IPluginLoader_ptr> Obj2Loader_;

		// All plugins ever seen
		PluginsContainer_t AvailablePlugins_;

		QStringList Headers_;
		QIcon DefaultPluginIcon_;
		QStringList PluginLoadErrors_;
		mutable QMap<QByteArray, QObject*> PluginID2PluginCache_;

		std::shared_ptr<PluginTreeBuilder> PluginTreeBuilder_;

		mutable bool CacheValid_;
		mutable QObjectList SortedCache_;

		class PluginLoadProcess;
	public:
		enum Roles
		{
			PluginObject = Qt::UserRole + 100,
			PluginID,
			PluginFilename
		};

		enum class InitStage
		{
			BeforeFirst,
			BeforeSecond,
			PostSecond,
			Complete
		};
	private:
		InitStage InitStage_ = InitStage::BeforeFirst;
	public:
		typedef PluginsContainer_t::size_type Size_t;
		PluginManager (const QStringList& pluginPaths, QObject *parent = 0);

		int columnCount (const QModelIndex& = QModelIndex ()) const;
		QVariant data (const QModelIndex&, int = Qt::DisplayRole) const;
		Qt::ItemFlags flags (const QModelIndex&) const;
		QVariant headerData (int, Qt::Orientation, int = Qt::DisplayRole) const;
		QModelIndex index (int, int, const QModelIndex& = QModelIndex()) const;
		QModelIndex parent (const QModelIndex&) const;
		int rowCount (const QModelIndex& = QModelIndex ()) const;
		bool setData (const QModelIndex&, const QVariant&, int);

		Size_t GetSize () const;
		void Init (bool safeMode);
		void Release ();
		QString Name (const Size_t& pos) const;
		QString Info (const Size_t& pos) const;

		QList<Loaders::IPluginLoader_ptr> GetAllAvailable () const;
		QObjectList GetAllPlugins () const;
		QString GetPluginLibraryPath (const QObject*) const;

		QObject* GetPluginByID (const QByteArray&) const;

		QObjectList GetFirstLevels (const QByteArray& pclass) const;
		QObjectList GetFirstLevels (const QSet<QByteArray>& pclasses) const;

		void InjectPlugin (QObject *object);
		void ReleasePlugin (QObject *object);

		void SetAllPlugins (Qt::CheckState);

		QObject* GetQObject ();

		void OpenSettings (QObject*);

		ILoadProgressReporter_ptr CreateLoadProgressReporter (QObject*);


		const QStringList& GetPluginLoadErrors () const;

		InitStage GetInitStage () const;
	private:
		void SetInitStage (InitStage);

		QStringList FindPluginsPaths () const;
		void FindPlugins ();
		void ScanPlugins (const QStringList&);

		/** Tries to load all the plugins and filters out those who fail
		 * various sanity checks.
		 */
		void CheckPlugins ();

		/** Fills the Plugins_ list with all instances, both from "real"
		 * plugins and from adaptors.
		 */
		void FillInstances ();

		/** Tries to perform IInfo::Init() on all plugins. Returns the
		 * list of plugins that failed.
		 */
		QList<QObject*> FirstInitAll (PluginLoadProcess*);

		/** Tries to perform IInfo::Init() on plugins and returns the
		 * first plugin that has failed to initialize. This function
		 * stops initializing plugins upon first failure. If all plugins
		 * were initialized successfully, this function returns NULL.
		 */
		QObject* TryFirstInit (QObjectList, PluginLoadProcess*);

		/** Plainly tries to find a corresponding QPluginLoader and
		 * unload the corresponding library.
		 */
		void TryUnload (QObjectList);

		Loaders::IPluginLoader_ptr MakeLoader (const QString&);

		QList<Plugins_t::iterator> FindProviders (const QString&);
		QList<Plugins_t::iterator> FindProviders (const QSet<QByteArray>&);
	signals:
		void pluginInjected (QObject*);

		void initStageChanged (PluginManager::InitStage);
	};
}
