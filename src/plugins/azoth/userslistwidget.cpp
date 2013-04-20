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

#include "userslistwidget.h"
#include <QSortFilterProxyModel>
#include <QStandardItemModel>
#include <util/gui/clearlineeditaddon.h>
#include "core.h"
#include "keyboardrosterfixer.h"

namespace LeechCraft
{
namespace Azoth
{
	namespace
	{
		enum PartsListRoles
		{
			PLRObject = Qt::UserRole + 1
		};
	}

	UsersListWidget::UsersListWidget (const QList<QObject*>& parts,
			std::function<QString (ICLEntry*)> nameGetter, QWidget *parent)
	: QDialog (parent,
			static_cast<Qt::WindowFlags> (Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint))
	, Filter_ (new QSortFilterProxyModel (this))
	, PartsModel_ (new QStandardItemModel (this))
	{
		Ui_.setupUi (this);

		for (auto part : parts)
		{
			auto entry = qobject_cast<ICLEntry*> (part);

			auto item = new QStandardItem (nameGetter (entry));
			item->setData (QVariant::fromValue (part), PLRObject);
			item->setEditable (false);

			PartsModel_->appendRow (item);
		}

		Filter_->setSourceModel (PartsModel_);
		Filter_->setSortCaseSensitivity (Qt::CaseInsensitive);
		Filter_->setSortRole (Qt::DisplayRole);
		connect (Ui_.FilterLine_,
				SIGNAL (textChanged (QString)),
				Filter_,
				SLOT (setFilterFixedString (QString)));
		Ui_.ListView_->setModel (Filter_);
		Ui_.ListView_->sortByColumn (0, Qt::AscendingOrder);

		new Util::ClearLineEditAddon (Core::Instance ().GetProxy (), Ui_.FilterLine_);

		connect (Ui_.ListView_,
				SIGNAL (activated (QModelIndex)),
				this,
				SLOT (accept ()));

		Ui_.ListView_->setFocusProxy (Ui_.FilterLine_);
		Ui_.ListView_->setFocus ();
		Ui_.FilterLine_->installEventFilter (new KeyboardRosterFixer (Ui_.ListView_, this));
	}

	QObject* UsersListWidget::GetActivatedParticipant () const
	{
		const auto& current = Ui_.ListView_->currentIndex ();
		if (!current.isValid ())
			return 0;

		return current.data (PLRObject).value<QObject*> ();
	}
}
}
