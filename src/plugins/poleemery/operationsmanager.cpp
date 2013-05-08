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

#include "operationsmanager.h"
#include <QStandardItemModel>
#include "storage.h"
#include "entriesmodel.h"
#include "core.h"
#include "currenciesmanager.h"

namespace LeechCraft
{
namespace Poleemery
{
	OperationsManager::OperationsManager (Storage_ptr storage, QObject *parent)
	: QObject (parent)
	, Storage_ (storage)
	, Model_ (new EntriesModel (this))
	{
	}

	void OperationsManager::Load ()
	{
		QList<EntryBase_ptr> entries;
		for (const auto& entry : Storage_->GetReceiptEntries ())
			entries << std::make_shared<ReceiptEntry> (entry);
		for (const auto& entry : Storage_->GetExpenseEntries ())
		{
			entries << std::make_shared<ExpenseEntry> (entry);
			for (const auto& cat : entry.Categories_)
				KnownCategories_ << cat;
		}
		Model_->AddEntries (entries);

		connect (Core::Instance ().GetCurrenciesManager (),
				SIGNAL (currenciesUpdated ()),
				Model_,
				SLOT (recalcSums ()));
	}

	QAbstractItemModel* OperationsManager::GetModel () const
	{
		return Model_;
	}

	QList<EntryBase_ptr> OperationsManager::GetAllEntries () const
	{
		return Model_->GetEntries ();
	}

	QSet<QString> OperationsManager::GetKnownCategories () const
	{
		return KnownCategories_;
	}

	void OperationsManager::AddEntry (EntryBase_ptr entry)
	{
		switch (entry->GetType ())
		{
		case EntryType::Expense:
		{
			auto expense = std::dynamic_pointer_cast<ExpenseEntry> (entry);
			Storage_->AddExpenseEntry (*expense);
			for (const auto& cat : expense->Categories_)
				KnownCategories_ << cat;
			break;
		}
		case EntryType::Receipt:
			Storage_->AddReceiptEntry (*std::dynamic_pointer_cast<ReceiptEntry> (entry));
			break;
		}

		Model_->AddEntry (entry);
	}

	void OperationsManager::UpdateEntry (EntryBase_ptr entry)
	{
		switch (entry->GetType ())
		{
		case EntryType::Expense:
		{
			auto expense = std::dynamic_pointer_cast<ExpenseEntry> (entry);
			Storage_->UpdateExpenseEntry (*expense);
			for (const auto& cat : expense->Categories_)
				KnownCategories_ << cat;
			break;
		}
		case EntryType::Receipt:
			Storage_->UpdateReceiptEntry (*std::dynamic_pointer_cast<ReceiptEntry> (entry));
			break;
		}
	}

	void OperationsManager::RemoveEntry (const QModelIndex& index)
	{
		auto entry = Model_->GetEntry (index);

		switch (entry->GetType ())
		{
		case EntryType::Expense:
			Storage_->DeleteExpenseEntry (*std::dynamic_pointer_cast<ExpenseEntry> (entry));
			break;
		case EntryType::Receipt:
			Storage_->DeleteReceiptEntry (*std::dynamic_pointer_cast<ReceiptEntry> (entry));
			break;
		}

		Model_->RemoveEntry (index);
	}
}
}
