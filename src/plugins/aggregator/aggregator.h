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
#include <boost/function.hpp>
#include <QWidget>
#include <QItemSelection>
#include <interfaces/iinfo.h>
#include <interfaces/ihavetabs.h>
#include <interfaces/ijobholder.h>
#include <interfaces/ihavesettings.h>
#include <interfaces/ihaveshortcuts.h>
#include <interfaces/ientityhandler.h>
#include <interfaces/structures.h>
#include <interfaces/iactionsexporter.h>
#include <interfaces/istartupwizard.h>
#include <interfaces/ipluginready.h>
#include <interfaces/ihaverecoverabletabs.h>

class QSystemTrayIcon;
class QTranslator;
class QToolBar;
class IDownload;

namespace LeechCraft
{
namespace Aggregator
{
	struct Enclosure;

	struct Aggregator_Impl;

	class Aggregator : public QWidget
					 , public IInfo
					 , public IHaveTabs
					 , public ITabWidget
					 , public IHaveSettings
					 , public IJobHolder
					 , public IEntityHandler
					 , public IHaveShortcuts
					 , public IActionsExporter
					 , public IStartupWizard
					 , public IPluginReady
					 , public IHaveRecoverableTabs
					 , public IRecoverableTab
	{
		Q_OBJECT
		Q_INTERFACES (IInfo
				IHaveTabs
				ITabWidget
				IHaveSettings
				IJobHolder
				IEntityHandler
				IHaveShortcuts
				IStartupWizard
				IActionsExporter
				IPluginReady
				IHaveRecoverableTabs
				IRecoverableTab)

		LC_PLUGIN_METADATA ("org.LeechCraft.Aggregator")

		Aggregator_Impl *Impl_;
	public:
		void Init (ICoreProxy_ptr);
		void SecondInit ();
		void Release ();
		QByteArray GetUniqueID () const;
		QString GetName () const;
		QString GetInfo () const;
		QStringList Provides () const;
		QStringList Needs () const;
		QStringList Uses () const;
		QIcon GetIcon () const;

		TabClasses_t GetTabClasses () const;
		QToolBar* GetToolBar () const;
		void TabOpenRequested (const QByteArray&);
		TabClassInfo GetTabClassInfo () const;
		QObject* ParentMultiTabs ();
		void Remove ();

		QAbstractItemModel* GetRepresentation () const;

		Util::XmlSettingsDialog_ptr GetSettingsDialog () const;

		EntityTestHandleResult CouldHandle (const Entity&) const;
		void Handle (Entity);

		void SetShortcut (const QString&, const QKeySequences_t&);
		QMap<QString, ActionInfo> GetActionInfo () const;

		QList<QWizardPage*> GetWizardPages () const;

		QList<QAction*> GetActions (ActionsEmbedPlace) const;

		QSet<QByteArray> GetExpectedPluginClasses () const;
		void AddPlugin (QObject*);

		void RecoverTabs (const QList<TabRecoverInfo>& infos);
		bool HasSimilarTab (const QByteArray&, const QList<QByteArray>&) const;

		QByteArray GetTabRecoverData () const;
		QIcon GetTabRecoverIcon () const;
		QString GetTabRecoverName () const;
	protected:
		virtual void keyPressEvent (QKeyEvent*);
	private:
		bool IsRepr () const;
		QModelIndex GetRelevantIndex () const;
		QList<QModelIndex> GetRelevantIndexes () const;
		void BuildID2ActionTupleMap ();
		void Perform (boost::function<void (const QModelIndex&)>);
	public slots:
		void handleTasksTreeSelectionCurrentRowChanged (const QModelIndex&, const QModelIndex&);
	private slots:
		void on_ActionMarkAllAsRead__triggered ();
		void on_ActionAddFeed__triggered ();
		void on_ActionRemoveFeed__triggered ();
		void on_ActionRenameFeed__triggered ();
		void on_ActionRemoveChannel__triggered ();
		void on_ActionUpdateSelectedFeed__triggered ();
		void on_ActionImportOPML__triggered ();
		void on_ActionExportOPML__triggered ();
		void on_ActionImportBinary__triggered ();
		void on_ActionExportBinary__triggered ();
		void on_ActionExportFB2__triggered ();
		void on_ActionMarkChannelAsRead__triggered ();
		void on_ActionMarkChannelAsUnread__triggered ();
		void on_ActionChannelSettings__triggered ();

		void handleFeedsContextMenuRequested (const QPoint&);

		void on_MergeItems__toggled (bool);

		void currentChannelChanged ();
		void handleItemsMovedToChannel (QModelIndex);

		void handleGroupChannels ();
	signals:
		void gotEntity (const LeechCraft::Entity&);
		void addNewTab (const QString&, QWidget*);
		void removeTab (QWidget*);
		void changeTabName (QWidget*, const QString&);
		void changeTabIcon (QWidget*, const QIcon&);
		void changeTooltip (QWidget*, QWidget*);
		void statusBarChanged (QWidget*, const QString&);
		void raiseTab (QWidget*);

		void gotActions (QList<QAction*>, LeechCraft::ActionsEmbedPlace);

		void tabRecoverDataChanged ();
	};
}
}
