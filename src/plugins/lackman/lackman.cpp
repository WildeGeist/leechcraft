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

#include "lackman.h"
#include <QSortFilterProxyModel>
#include <QIcon>
#include <util/util.h>
#include <util/shortcuts/shortcutmanager.h>
#include <util/sll/slotclosure.h>
#include <interfaces/entitytesthandleresult.h>
#include "core.h"
#include "xmlsettingsmanager.h"
#include "lackmantab.h"

namespace LeechCraft
{
namespace LackMan
{
	void Plugin::Init (ICoreProxy_ptr proxy)
	{
		Util::InstallTranslator ("lackman");

		TabClass_.TabClass_ = "Lackman";
		TabClass_.VisibleName_ = "LackMan";
		TabClass_.Description_ = GetInfo ();
		TabClass_.Icon_ = GetIcon ();
		TabClass_.Priority_ = 0;
		TabClass_.Features_ = TFSingle | TFByDefault | TFOpenableByRequest;

		ShortcutMgr_ = new Util::ShortcutManager (proxy, this);

		SettingsDialog_.reset (new Util::XmlSettingsDialog ());
		SettingsDialog_->RegisterObject (XmlSettingsManager::Instance (),
				"lackmansettings.xml");

		Core::Instance ().SetProxy (proxy);
		Core::Instance ().FinishInitialization ();

		SettingsDialog_->SetDataSource ("RepositoryList",
				Core::Instance ().GetRepositoryModel ());

		connect (&Core::Instance (),
				SIGNAL (gotEntity (LeechCraft::Entity)),
				this,
				SIGNAL (gotEntity (LeechCraft::Entity)));
	}

	void Plugin::SecondInit ()
	{
		Core::Instance ().SecondInit ();

		new Util::SlotClosure<Util::NoDeletePolicy>
		{
			[this] { TabOpenRequested (TabClass_.TabClass_); },
			&Core::Instance (),
			SIGNAL (openLackmanRequested ()),
			this
		};
	}

	void Plugin::Release ()
	{
		Core::Instance ().Release ();
	}

	QByteArray Plugin::GetUniqueID () const
	{
		return "org.LeechCraft.LackMan";
	}

	QString Plugin::GetName () const
	{
		return "LackMan";
	}

	QString Plugin::GetInfo () const
	{
		return tr ("LeechCraft Package Manager.");
	}

	QIcon Plugin::GetIcon () const
	{
		static QIcon icon ("lcicons:/resources/images/lackman.svg");
		return icon;
	}

	TabClasses_t Plugin::GetTabClasses () const
	{
		TabClasses_t result;
		result << TabClass_;
		return result;
	}

	void Plugin::TabOpenRequested (const QByteArray& tabClass)
	{
		TabOpenRequested (tabClass, {});
	}

	void Plugin::TabOpenRequested (const QByteArray& tabClass, const QList<QPair<QByteArray, QVariant>>& props)
	{
		if (tabClass == "Lackman")
		{
			if (LackManTab_)
			{
				raiseTab (LackManTab_);
				return;
			}

			LackManTab_ = new LackManTab (ShortcutMgr_, TabClass_, this);
			connect (LackManTab_,
					SIGNAL (removeTab (QWidget*)),
					this,
					SIGNAL (removeTab (QWidget*)));

			for (const auto& pair : props)
				LackManTab_->setProperty (pair.first, pair.second);

			new Util::SlotClosure<Util::DeleteLaterPolicy>
			{
				[this] { LackManTab_ = nullptr; },
				LackManTab_,
				SIGNAL (removeTab (QWidget*)),
				LackManTab_
			};

			emit addNewTab (GetName (), LackManTab_);
			emit raiseTab (LackManTab_);
		}
		else
			qWarning () << Q_FUNC_INFO
					<< "unknown tab class"
					<< tabClass;
	}

	Util::XmlSettingsDialog_ptr Plugin::GetSettingsDialog () const
	{
		return SettingsDialog_;
	}

	EntityTestHandleResult Plugin::CouldHandle (const Entity& entity) const
	{
		if (entity.Mime_ != "x-leechcraft/package-manager-action")
			return {};

		return EntityTestHandleResult (EntityTestHandleResult::PIdeal);
	}

	void Plugin::Handle (Entity entity)
	{
		const auto& action = entity.Entity_.toString ();
		if (action == "ListPackages")
		{
			TabOpenRequested ("Lackman");

			const auto& tags = entity.Additional_ ["Tags"].toStringList ();
			if (!tags.isEmpty ())
				LackManTab_->SetFilterTags (tags);
			else
				LackManTab_->SetFilterString (entity.Additional_ ["FilterString"].toString ());
		}
	}

	void Plugin::SetShortcut (const QString& id, const QKeySequences_t& seqs)
	{
		ShortcutMgr_->SetShortcut (id, seqs);
	}

	QMap<QString, ActionInfo> Plugin::GetActionInfo () const
	{
		return ShortcutMgr_->GetActionInfo ();
	}

	void Plugin::RecoverTabs (const QList<TabRecoverInfo>& infos)
	{
		for (const auto& recInfo : infos)
			if (recInfo.Data_ == "lackmantab")
				TabOpenRequested (TabClass_.TabClass_, recInfo.DynProperties_);
			else
				qWarning () << Q_FUNC_INFO
						<< "unknown context"
						<< recInfo.Data_;
	}

	bool Plugin::HasSimilarTab (const QByteArray&, const QList<QByteArray>&) const
	{
		return true;
	}
}
}

LC_EXPORT_PLUGIN (leechcraft_lackman, LeechCraft::LackMan::Plugin);
