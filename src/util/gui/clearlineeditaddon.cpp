/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2014  Georg Rudoy
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE or copy at https://www.boost.org/LICENSE_1_0.txt)
 **********************************************************************/

#include "clearlineeditaddon.h"
#include <QLineEdit>
#include <QToolButton>
#include <QApplication>
#include <QStyle>
#include <QShortcut>
#include "lineeditbuttonmanager.h"
#include "interfaces/core/iiconthememanager.h"

namespace LC
{
namespace Util
{
	ClearLineEditAddon::ClearLineEditAddon (ICoreProxy_ptr proxy, QLineEdit *edit)
	: ClearLineEditAddon { proxy, edit, new LineEditButtonManager { edit } }
	{
	}

	ClearLineEditAddon::ClearLineEditAddon (ICoreProxy_ptr proxy,
			QLineEdit *edit, LineEditButtonManager *mgr)
	: QObject { edit }
	, Button_ { new QToolButton { edit } }
	, Edit_ { edit }
	, EscShortcut_ { new QShortcut { Qt::Key_Escape, edit, SLOT (clear ()), nullptr, Qt::WidgetShortcut } }
	{
		const bool isRtl = QApplication::layoutDirection () == Qt::RightToLeft;
		const auto& icon = proxy->GetIconThemeManager ()->GetIcon (isRtl ?
				"edit-clear-locationbar-ltr" :
				"edit-clear-locationbar-rtl");

		Button_->setIconSize (QSize (16, 16));
		Button_->setIcon (icon);
		Button_->setCursor (Qt::ArrowCursor);
		Button_->setStyleSheet ("QToolButton { border: none; padding: 0px; }");
		Button_->hide ();

		connect (Button_,
				SIGNAL (clicked ()),
				edit,
				SLOT (clear ()));
		connect (edit,
				SIGNAL (textChanged (QString)),
				this,
				SLOT (updateButton (QString)));

		updateButton (edit->text ());

		mgr->Add (Button_);
	}

	void ClearLineEditAddon::updateButton (const QString& text)
	{
		Button_->setVisible (!text.isEmpty ());
	}

	void ClearLineEditAddon::SetEscClearsEdit (bool clears)
	{
		EscShortcut_->setEnabled (clears);
	}
}
}
