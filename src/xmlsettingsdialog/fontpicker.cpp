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

#include "fontpicker.h"
#include <QLabel>
#include <QFontInfo>
#include <QPushButton>
#include <QFontDialog>
#include <QHBoxLayout>
#include <QApplication>

namespace LeechCraft
{
	FontPicker::FontPicker (const QString& title, QWidget *parent)
	: QWidget (parent)
	, Title_ (title)
	{
		if (Title_.isEmpty ())
			Title_ = tr ("Choose font");
		Label_ = new QLabel (this);
		ChooseButton_ = new QPushButton (tr ("Choose..."));
		QHBoxLayout *lay = new QHBoxLayout;
		lay->setContentsMargins (0, 0, 0, 0);
		lay->addWidget (Label_);
		lay->addWidget (ChooseButton_);
		setLayout (lay);
		connect (ChooseButton_,
				SIGNAL (released ()),
				this,
				SLOT (chooseFont ()));
		Label_->setMinimumWidth (1.5 * QApplication::fontMetrics ()
				.width (QApplication::font ().toString ()));
	}

	void FontPicker::SetCurrentFont (const QFont& font)
	{
		Font_ = font;
		QString text = Font_.family ();
		text += tr (", %1 pt")
			.arg (QFontInfo (Font_).pointSize ());
		if (Font_.bold ())
			text += tr (", bold");
		if (Font_.italic ())
			text += tr (", italic");
		if (Font_.underline ())
			text += tr (", underlined");
		if (Font_.strikeOut ())
			text += tr (", striken out");
		Label_->setText (text);
	}

	QFont FontPicker::GetCurrentFont () const
	{
		return Font_;
	}

	void FontPicker::chooseFont ()
	{
		bool ok = false;
		QFont font = QFontDialog::getFont (&ok,
				Font_,
				this,
				Title_);

		if (!ok ||
				font == Font_)
			return;

		SetCurrentFont (font);
		emit currentFontChanged (Font_);
	}
};

