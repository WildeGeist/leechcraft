/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2010-2012  Oleg Linkin
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

#include "blogique.h"
#include <QIcon>
#include <util/util.h>
#include "accountslistwidget.h"
#include "backupmanager.h"
#include "blogiquewidget.h"
#include "core.h"
#include "xmlsettingsmanager.h"

namespace LeechCraft
{
namespace Blogique
{
	void Plugin::Init (ICoreProxy_ptr proxy)
	{
		Util::InstallTranslator ("blogique");
		XmlSettingsDialog_.reset (new Util::XmlSettingsDialog ());
		XmlSettingsDialog_->RegisterObject (&XmlSettingsManager::Instance (),
				"blogiquesettings.xml");
		XmlSettingsDialog_->SetCustomWidget ("AccountsWidget", new AccountsListWidget);

		Core::Instance ().SetCoreProxy (proxy);

		BlogiqueWidget::SetParentMultiTabs (this);

		TabClassInfo tabClass =
		{
			"Blogique",
			"Blogique",
			GetInfo (),
			GetIcon (),
			50,
			TabFeatures (TFOpenableByRequest | TFSuggestOpening)
		};
		TabClasses_ << tabClass;

		connect (&Core::Instance (),
				SIGNAL (gotEntity (LeechCraft::Entity)),
				this,
				SIGNAL (gotEntity (LeechCraft::Entity)));
		connect (&Core::Instance (),
				SIGNAL (delegateEntity (LeechCraft::Entity, int*, QObject**)),
				this,
				SIGNAL (delegateEntity (LeechCraft::Entity, int*, QObject**)));
		connect (&Core::Instance (),
				SIGNAL (addNewTab (QString,QWidget*)),
				this,
				SIGNAL (addNewTab (QString,QWidget*)));
		connect (&Core::Instance (),
				SIGNAL (removeTab (QWidget*)),
				this,
				SIGNAL (removeTab (QWidget*)));
		connect (&Core::Instance (),
				SIGNAL (changeTabName (QWidget*, QString)),
				this,
				SIGNAL (changeTabName (QWidget*, QString)));

		BackupBlog_ = new QAction (tr ("Backup"), this);
		BackupBlog_->setProperty ("ActionIcon", "document-export");

		connect (BackupBlog_,
				SIGNAL (triggered ()),
				Core::Instance ().GetBackupManager (),
				SLOT (backup ()));

		ToolMenu_ = new QMenu ("Blogique");
		ToolMenu_->setIcon (GetIcon ());
		ToolMenu_->addAction (BackupBlog_);
	}

	void Plugin::SecondInit ()
	{
		Core::Instance ().DelayedProfilesUpdate ();
	}

	QByteArray Plugin::GetUniqueID () const
	{
		return Core::Instance ().GetUniqueID ();
	}

	void Plugin::Release ()
	{
	}

	QString Plugin::GetName () const
	{
		return "Blogique";
	}

	QString Plugin::GetInfo () const
	{
		return tr ("Blogging client");
	}

	QIcon Plugin::GetIcon () const
	{
		return Core::Instance ().GetIcon ();
	}

	TabClasses_t Plugin::GetTabClasses () const
	{
		return TabClasses_;
	}

	void Plugin::TabOpenRequested (const QByteArray& tabClass)
	{
		if (tabClass == "Blogique")
			CreateTab ();
		else
			qWarning () << Q_FUNC_INFO
					<< "unknown tab class"
					<< tabClass;
	}

	Util::XmlSettingsDialog_ptr Plugin::GetSettingsDialog () const
	{
		return XmlSettingsDialog_;
	}

	QSet<QByteArray> Plugin::GetExpectedPluginClasses () const
	{
		return Core::Instance ().GetExpectedPluginClasses ();
	}

	void Plugin::AddPlugin (QObject* plugin)
	{
		Core::Instance ().AddPlugin (plugin);
	}

	QList<QAction*> Plugin::GetActions (ActionsEmbedPlace area) const
	{
		QList<QAction*> result;

		switch (area)
		{
			case ActionsEmbedPlace::ToolsMenu:
				result << ToolMenu_->menuAction ();
				break;
			default:
				break;
		}

		return result;
	}

	void Plugin::RecoverTabs (const QList<TabRecoverInfo>& infos)
	{
		Q_FOREACH (const TabRecoverInfo& recInfo, infos)
		{
			QDataStream str (recInfo.Data_);
			qint8 version;
			str >> version;

			if (version == 1)
			{
				auto tab = Core::Instance ().CreateBlogiqueWidget ();
				Entry e;
				str >> e.Subject_
						>> e.Content_
						>> e.Date_
						>> e.Tags_
						>> e.Target_
						>> e.PostOptions_
						>> e.CustomData_;
				QByteArray accId;
				str >> accId;
				tab->FillWidget (e, accId);
				emit addNewTab ("Blogique", tab);
				emit changeTabIcon (tab, GetIcon ());
				emit raiseTab (tab);
				emit changeTabName (tab, e.Subject_);
			}
			else
				qWarning () << Q_FUNC_INFO
						<< "unknown version"
						<< version;
		}
	}

	void Plugin::CreateTab ()
	{
		BlogiqueWidget *blogPage = Core::Instance ().CreateBlogiqueWidget ();
		emit addNewTab ("Blogique", blogPage);
		emit changeTabIcon (blogPage, GetIcon ());
		emit raiseTab (blogPage);
	}
}
}

LC_EXPORT_PLUGIN (leechcraft_blogique, LeechCraft::Blogique::Plugin);
