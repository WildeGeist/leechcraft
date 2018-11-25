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

namespace LeechCraft
{
namespace Util
{
	class ShortcutManager;
}

namespace Aggregator
{
	class AggregatorTab;
	class RepresentationManager;
	struct AppWideActions;
	struct ChannelActions;

	class Aggregator : public QObject
					 , public IInfo
					 , public IHaveTabs
					 , public IHaveSettings
					 , public IJobHolder
					 , public IEntityHandler
					 , public IHaveShortcuts
					 , public IActionsExporter
					 , public IStartupWizard
					 , public IPluginReady
					 , public IHaveRecoverableTabs
	{
		Q_OBJECT
		Q_INTERFACES (IInfo
				IHaveTabs
				IHaveSettings
				IJobHolder
				IEntityHandler
				IHaveShortcuts
				IStartupWizard
				IActionsExporter
				IPluginReady
				IHaveRecoverableTabs)

		LC_PLUGIN_METADATA ("org.LeechCraft.Aggregator")

		std::shared_ptr<AppWideActions> AppWideActions_;
		std::shared_ptr<ChannelActions> ChannelActions_;

		QMenu *ToolMenu_;

		TabClassInfo TabInfo_;

		std::shared_ptr<Util::XmlSettingsDialog> XmlSettingsDialog_;
		std::shared_ptr<RepresentationManager> ReprManager_;
		std::shared_ptr<AggregatorTab> AggregatorTab_;

		Util::ShortcutManager *ShortcutMgr_ = nullptr;

		bool InitFailed_ = false;
	public:
		void Init (ICoreProxy_ptr) override;
		void SecondInit () override;
		void Release () override;
		QByteArray GetUniqueID () const override;
		QString GetName () const override;
		QString GetInfo () const override;
		QStringList Provides () const override;
		QStringList Needs () const override;
		QStringList Uses () const override;
		QIcon GetIcon () const override;

		TabClasses_t GetTabClasses () const override;
		void TabOpenRequested (const QByteArray&) override;

		QAbstractItemModel* GetRepresentation () const override;

		Util::XmlSettingsDialog_ptr GetSettingsDialog () const override;

		EntityTestHandleResult CouldHandle (const Entity&) const override;
		void Handle (Entity) override;

		void SetShortcut (const QString&, const QKeySequences_t&) override;
		QMap<QString, ActionInfo> GetActionInfo () const override;

		QList<QWizardPage*> GetWizardPages () const override;

		QList<QAction*> GetActions (ActionsEmbedPlace) const override;

		QSet<QByteArray> GetExpectedPluginClasses () const override;
		void AddPlugin (QObject*) override;

		void RecoverTabs (const QList<TabRecoverInfo>& infos) override;
		bool HasSimilarTab (const QByteArray&, const QList<QByteArray>&) const override;
	private:
		QModelIndex GetRelevantIndex () const;
		QList<QModelIndex> GetRelevantIndexes () const;
		void BuildID2ActionTupleMap ();

		template<typename F>
		void Perform (F&&);
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
	signals:
		void gotEntity (const LeechCraft::Entity&) override;
		void addNewTab (const QString&, QWidget*) override;
		void removeTab (QWidget*) override;
		void changeTabName (QWidget*, const QString&) override;
		void changeTabIcon (QWidget*, const QIcon&) override;
		void statusBarChanged (QWidget*, const QString&) override;
		void raiseTab (QWidget*) override;

		void gotActions (QList<QAction*>, LeechCraft::ActionsEmbedPlace) override;
	};
}
}
