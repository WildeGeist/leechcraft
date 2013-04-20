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

#include "itemhandlerlineedit.h"
#include <QLabel>
#include <QLineEdit>
#include <QGridLayout>
#include <QApplication>
#include <QtDebug>

namespace LeechCraft
{
	ItemHandlerLineEdit::ItemHandlerLineEdit ()
	{
	}

	ItemHandlerLineEdit::~ItemHandlerLineEdit ()
	{
	}

	bool ItemHandlerLineEdit::CanHandle (const QDomElement& element) const
	{
		return element.attribute ("type") == "lineedit";
	}

	void ItemHandlerLineEdit::Handle (const QDomElement& item,
			QWidget *pwidget)
	{
		QGridLayout *lay = qobject_cast<QGridLayout*> (pwidget->layout ());
		QLabel *label = new QLabel (XSD_->GetLabel (item));
		label->setWordWrap (false);

		const QVariant& value = XSD_->GetValue (item);

		QLineEdit *edit = new QLineEdit (value.toString ());
		XSD_->SetTooltip (edit, item);
		edit->setObjectName (item.attribute ("property"));
		edit->setMinimumWidth (QApplication::fontMetrics ()
				.width ("thisismaybeadefaultsetting"));
		if (item.hasAttribute ("password"))
			edit->setEchoMode (QLineEdit::Password);
		if (item.hasAttribute ("inputMask"))
			edit->setInputMask (item.attribute ("inputMask"));
		connect (edit,
				SIGNAL (textChanged (const QString&)),
				this,
				SLOT (updatePreferences ()));

		edit->setProperty ("ItemHandler", QVariant::fromValue<QObject*> (this));
		edit->setProperty ("SearchTerms", label->text ());

		int row = lay->rowCount ();
		lay->addWidget (label, row, 0, Qt::AlignRight);
		lay->addWidget (edit, row, 1);
	}

	void ItemHandlerLineEdit::SetValue (QWidget *widget, const QVariant& value) const
	{
		QLineEdit *edit = qobject_cast<QLineEdit*> (widget);
		if (!edit)
		{
			qWarning () << Q_FUNC_INFO
				<< "not a QLineEdit"
				<< widget;
			return;
		}
		edit->setText (value.toString ());
	}

	QVariant ItemHandlerLineEdit::GetObjectValue (QObject *object) const
	{
		QLineEdit *edit = qobject_cast<QLineEdit*> (object);
		if (!edit)
		{
			qWarning () << Q_FUNC_INFO
				<< "not a QLineEdit"
				<< object;
			return QVariant ();
		}
		return edit->text ();
	}
};
