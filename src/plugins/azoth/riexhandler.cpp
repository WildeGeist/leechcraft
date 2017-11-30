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

#include "riexhandler.h"
#include <QtDebug>
#include <util/sll/unreachable.h>
#include "interfaces/azoth/iaccount.h"
#include "interfaces/azoth/iclentry.h"
#include "acceptriexdialog.h"

namespace LeechCraft
{
namespace Azoth
{
namespace RIEX
{
	namespace
	{
		void FilterRIEXItems (QList<RIEXItem>& items, const QHash<QString, ICLEntry*>& clEntries)
		{
			const auto end = std::remove_if (items.begin (), items.end (),
					[&clEntries] (const RIEXItem& item)
					{
						const auto entry = clEntries.value (item.ID_);

						switch (item.Action_)
						{
						case RIEXItem::AAdd:
							return entry != nullptr;
						case RIEXItem::ADelete:
							if (item.Groups_.isEmpty ())
								return false;
							else
							{
								const auto& origGroups = entry->Groups ();
								return std::any_of (item.Groups_.begin (), item.Groups_.end (),
										[&origGroups] (const auto& group) { return origGroups.contains (group); });
							}
						case RIEXItem::AModify:
							return !entry;
						}

						Util::Unreachable ();
					});

			items.erase (end, items.end ());
		}

		void AddRIEX (const RIEXItem& item, const QHash<QString, ICLEntry*> entries, IAccount *acc)
		{
			if (!entries.contains (item.ID_))
			{
				acc->RequestAuth (item.ID_, QString (), item.Nick_, item.Groups_);
				return;
			}

			ICLEntry *entry = entries [item.ID_];

			bool allGroups = true;
			for (const QString& group : item.Groups_)
				if (!entry->Groups ().contains (group))
				{
					allGroups = false;
					break;
				}

			if (!allGroups)
			{
				QStringList newGroups = item.Groups_ + entry->Groups ();
				newGroups.removeDuplicates ();
				entry->SetGroups (newGroups);
			}
			else
			{
				qWarning () << Q_FUNC_INFO
						<< "skipping already-existing"
						<< item.ID_;
				return;
			}
		}

		void ModifyRIEX (const RIEXItem& item, const QHash<QString, ICLEntry*> entries, IAccount*)
		{
			if (!entries.contains (item.ID_))
			{
				qWarning () << Q_FUNC_INFO
						<< "skipping non-existent"
						<< item.ID_;
				return;
			}

			ICLEntry *entry = entries [item.ID_];

			if (!item.Groups_.isEmpty ())
				entry->SetGroups (item.Groups_);

			if (!item.Nick_.isEmpty ())
				entry->SetEntryName (item.Nick_);
		}

		void DeleteRIEX (const RIEXItem& item, const QHash<QString, ICLEntry*> entries, IAccount *acc)
		{
			if (!entries.contains (item.ID_))
			{
				qWarning () << Q_FUNC_INFO
						<< "skipping non-existent"
						<< item.ID_;
				return;
			}

			const auto entry = entries [item.ID_];
			if (item.Groups_.isEmpty ())
				acc->RemoveEntry (entry->GetQObject ());
			else
			{
				auto newGroups = entry->Groups ();
				for (const auto& group : item.Groups_)
					newGroups.removeAll (group);

				entry->SetGroups (newGroups);
			}
		}
	}

	void HandleRIEXItemsSuggested (QList<RIEXItem> items, QObject *from, QString message)
	{
		if (items.isEmpty () || !from)
			return;

		const auto entry = qobject_cast<ICLEntry*> (from);
		if (!entry)
		{
			qWarning () << Q_FUNC_INFO
					<< from
					<< "doesn't implement ICLEntry";
			return;
		}

		const auto acc = entry->GetParentAccount ();
		QHash<QString, ICLEntry*> clEntries;
		for (const auto entryObj : acc->GetCLEntries ())
		{
			const auto entry = qobject_cast<ICLEntry*> (entryObj);
			if (!entry ||
					(entry->GetEntryFeatures () & ICLEntry::FMaskLongetivity) != ICLEntry::FPermanentEntry)
				continue;

			clEntries [entry->GetHumanReadableID ()] = entry;
		}

		FilterRIEXItems (items, clEntries);
		if (items.isEmpty ())
			return;

		AcceptRIEXDialog dia (items, from, message);
		if (dia.exec () != QDialog::Accepted)
			return;

		for (const auto& item : dia.GetSelectedItems ())
		{
			switch (item.Action_)
			{
			case RIEXItem::AAdd:
				AddRIEX (item, clEntries, acc);
				break;
			case RIEXItem::AModify:
				ModifyRIEX (item, clEntries, acc);
				break;
			case RIEXItem::ADelete:
				DeleteRIEX (item, clEntries, acc);
				break;
			default:
				qWarning () << Q_FUNC_INFO
						<< "unknown action"
						<< item.Action_
						<< "for item"
						<< item.ID_
						<< item.Nick_
						<< item.Groups_;
				break;
			}
		}
	}
}
}
}
