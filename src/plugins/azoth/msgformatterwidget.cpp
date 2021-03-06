/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2014  Georg Rudoy
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE or copy at https://www.boost.org/LICENSE_1_0.txt)
 **********************************************************************/

#include "msgformatterwidget.h"
#include <cmath>
#include <QVBoxLayout>
#include <QTextEdit>
#include <QToolBar>
#include <QAction>
#include <QTextBlock>
#include <QColorDialog>
#include <QDomDocument>
#include <QFontDialog>
#include <QActionGroup>
#include <QGridLayout>
#include <QToolButton>
#include <QKeyEvent>
#include <QApplication>
#include <QtDebug>
#include <util/sll/qtutil.h>
#include "interfaces/azoth/iresourceplugin.h"
#include "xmlsettingsmanager.h"
#include "core.h"

namespace LC
{
namespace Azoth
{
	namespace
	{
		class SmilesTooltip : public QWidget
		{
		public:
			SmilesTooltip (QWidget *parent)
			: QWidget (parent, Qt::Tool)
			{
			}

			void keyPressEvent (QKeyEvent *event)
			{
				if (event->key () == Qt::Key_Escape)
				{
					event->accept ();
					hide ();
				}
				else
					QWidget::keyPressEvent (event);
			}
		};
	}

	MsgFormatterWidget::MsgFormatterWidget (QTextEdit *edit, QWidget *parent)
	: QWidget (parent)
	, Edit_ (edit)
	, StockCharFormat_ (Edit_->currentCharFormat ())
	, StockBlockFormat_ (Edit_->document ()->begin ().blockFormat ())
	, StockFrameFormat_ (Edit_->document ()->rootFrame ()->frameFormat ())
	, HasCustomFormatting_ (false)
	, SmilesTooltip_ (new SmilesTooltip (this))
	{
		SmilesTooltip_->setWindowTitle (tr ("Emoticons"));

		setLayout (new QVBoxLayout ());
		layout ()->setContentsMargins (0, 0, 0, 0);
		QToolBar *toolbar = new QToolBar ();
		toolbar->setIconSize (QSize (16, 16));
		layout ()->addWidget (toolbar);

		FormatBold_ = toolbar->addAction (tr ("Bold"),
				this,
				SLOT (handleBold ()));
		FormatBold_->setCheckable (true);
		FormatBold_->setProperty ("ActionIcon", "format-text-bold");

		FormatItalic_ = toolbar->addAction (tr ("Italic"),
				this,
				SLOT (handleItalic ()));
		FormatItalic_->setCheckable (true);
		FormatItalic_->setProperty ("ActionIcon", "format-text-italic");

		FormatUnderline_ = toolbar->addAction (tr ("Underline"),
				this,
				SLOT (handleUnderline ()));
		FormatUnderline_->setCheckable (true);
		FormatUnderline_->setProperty ("ActionIcon", "format-text-underline");

		FormatStrikeThrough_ = toolbar->addAction (tr ("Strike through"),
				this,
				SLOT (handleStrikeThrough ()));
		FormatStrikeThrough_->setCheckable (true);
		FormatStrikeThrough_->setProperty ("ActionIcon", "format-text-strikethrough");

		toolbar->addSeparator ();

		FormatColor_ = toolbar->addAction (tr ("Text color"),
				this,
				SLOT (handleTextColor ()));
		FormatColor_->setProperty ("ActionIcon", "format-text-color");

		FormatFont_ = toolbar->addAction (tr ("Font"),
				this,
				SLOT (handleFont ()));
		FormatFont_->setProperty ("ActionIcon", "preferences-desktop-font");

		toolbar->addSeparator ();

		FormatAlignLeft_ = toolbar->addAction (tr ("Align left"),
				this,
				SLOT (handleParaAlignment ()));
		FormatAlignLeft_->setProperty ("ActionIcon", "format-justify-left");
		FormatAlignLeft_->setProperty ("Alignment", static_cast<int> (Qt::AlignLeft));
		FormatAlignLeft_->setCheckable (true);
		FormatAlignLeft_->setChecked (true);

		FormatAlignCenter_ = toolbar->addAction (tr ("Align center"),
				this,
				SLOT (handleParaAlignment ()));
		FormatAlignCenter_->setProperty ("ActionIcon", "format-justify-center");
		FormatAlignCenter_->setProperty ("Alignment", static_cast<int> (Qt::AlignCenter));
		FormatAlignCenter_->setCheckable (true);

		FormatAlignRight_ = toolbar->addAction (tr ("Align right"),
				this,
				SLOT (handleParaAlignment ()));
		FormatAlignRight_->setProperty ("ActionIcon", "format-justify-right");
		FormatAlignRight_->setProperty ("Alignment", static_cast<int> (Qt::AlignRight));
		FormatAlignRight_->setCheckable (true);

		FormatAlignJustify_ = toolbar->addAction (tr ("Align justify"),
				this,
				SLOT (handleParaAlignment ()));
		FormatAlignJustify_->setProperty ("ActionIcon", "format-justify-fill");
		FormatAlignJustify_->setProperty ("Alignment", static_cast<int> (Qt::AlignJustify));
		FormatAlignJustify_->setCheckable (true);

		QActionGroup *alignGroup = new QActionGroup (this);
		alignGroup->addAction (FormatAlignLeft_);
		alignGroup->addAction (FormatAlignCenter_);
		alignGroup->addAction (FormatAlignRight_);
		alignGroup->addAction (FormatAlignJustify_);

		connect (Edit_,
				SIGNAL (currentCharFormatChanged (const QTextCharFormat&)),
				this,
				SLOT (updateState (const QTextCharFormat&)));
		connect (Edit_,
				SIGNAL (textChanged ()),
				this,
				SLOT (checkCleared ()));

		toolbar->addSeparator ();

		AddEmoticon_ = toolbar->addAction (tr ("Emoticons..."),
				this,
				SLOT (handleAddEmoticon ()));
		AddEmoticon_->setProperty ("ActionIcon", "face-smile");

		for (const auto act : toolbar->actions ())
			if (!act->isSeparator ())
				act->setParent (this);

		XmlSettingsManager::Instance ().RegisterObject ("SmileIcons",
				this, "handleEmoPackChanged");
		handleEmoPackChanged ();
	}

	bool MsgFormatterWidget::HasCustomFormatting () const
	{
		return HasCustomFormatting_;
	}

	void MsgFormatterWidget::Clear ()
	{
		HasCustomFormatting_ = false;
	}

	QString MsgFormatterWidget::GetNormalizedRichText () const
	{
		if (!HasCustomFormatting ())
			return QString ();

		QString result = Edit_->toHtml ();

		QDomDocument doc;
		if (!doc.setContent (result))
			return result;

		const QDomNodeList& styles = doc.elementsByTagName ("style");
		const QDomElement& style = styles.size () ?
				styles.at (0).toElement () :
				QDomElement ();

		QDomElement body = doc.elementsByTagName ("body").at (0).toElement ();
		const QDomElement& elem = body.firstChildElement ();
		if (elem.isNull ())
			return QString ();
		else
			body.insertBefore (style.cloneNode (true), elem);

		body.setTagName ("div");

		QDomDocument finalDoc;
		finalDoc.appendChild (finalDoc.importNode (body, true));

		result = finalDoc.toString ();
		result = result.simplified ();
		result.remove ('\n');
		return result;
	}

	void MsgFormatterWidget::HidePopups ()
	{
		if (SmilesTooltip_)
			SmilesTooltip_->hide ();
	}

	void MsgFormatterWidget::CharFormatActor (std::function<void (QTextCharFormat*)> format)
	{
		QTextCursor cursor = Edit_->textCursor ();
		if (cursor.hasSelection ())
		{
			QTextCharFormat fmt = cursor.charFormat ();
			format (&fmt);
			cursor.setCharFormat (fmt);
		}
		else
		{
			QTextCharFormat fmt = Edit_->currentCharFormat ();
			format (&fmt);
			Edit_->setCurrentCharFormat (fmt);
		}

		HasCustomFormatting_ = true;
	}

	void MsgFormatterWidget::BlockFormatActor (std::function<void (QTextBlockFormat*)> format)
	{
		QTextBlockFormat fmt = Edit_->textCursor ().blockFormat ();
		format (&fmt);
		Edit_->textCursor ().setBlockFormat (fmt);

		HasCustomFormatting_ = true;
	}

	QTextCharFormat MsgFormatterWidget::GetActualCharFormat () const
	{
		const QTextCursor& cursor = Edit_->textCursor ();
		return cursor.hasSelection () ?
				cursor.charFormat () :
				Edit_->currentCharFormat ();
	}

	void MsgFormatterWidget::handleBold ()
	{
		const auto weight = FormatBold_->isChecked () ? QFont::Bold : QFont::Normal;
		CharFormatActor ([weight] (QTextCharFormat *fmt) { fmt->setFontWeight (weight); });
	}

	void MsgFormatterWidget::handleItalic ()
	{
		CharFormatActor ([this] (QTextCharFormat *fmt) { fmt->setFontItalic (FormatItalic_->isChecked ()); });
	}

	void MsgFormatterWidget::handleUnderline ()
	{
		CharFormatActor ([this] (QTextCharFormat *fmt) { fmt->setFontUnderline (FormatUnderline_->isChecked ()); });
	}

	void MsgFormatterWidget::handleStrikeThrough ()
	{
		CharFormatActor ([this] (QTextCharFormat *fmt) { fmt->setFontStrikeOut (FormatStrikeThrough_->isChecked ()); });
	}

	void MsgFormatterWidget::handleTextColor ()
	{
		QBrush brush = GetActualCharFormat ().foreground ();
		const QColor& color = QColorDialog::getColor (brush.color (), Edit_);
		if (!color.isValid ())
			return;

		CharFormatActor ([color] (QTextFormat *fmt) { fmt->setForeground (QBrush (color)); });
	}

	void MsgFormatterWidget::handleFont ()
	{
		QFont font = GetActualCharFormat ().font ();
		bool ok = false;
		font = QFontDialog::getFont (&ok, font, Edit_);
		if (!ok)
			return;

		CharFormatActor ([font] (QTextCharFormat *fmt) { fmt->setFont (font); });
	}

	void MsgFormatterWidget::handleParaAlignment ()
	{
		const auto alignment = static_cast<Qt::Alignment> (sender ()->property ("Alignment").toInt ());
		BlockFormatActor ([alignment] (QTextBlockFormat* fmt) { fmt->setAlignment (alignment); });
	}

	void MsgFormatterWidget::handleAddEmoticon ()
	{
		if (!SmilesTooltip_)
			return;

		SmilesTooltip_->move (QCursor::pos ());
		SmilesTooltip_->show ();
		SmilesTooltip_->activateWindow ();

		const QSize& size = SmilesTooltip_->size ();
		const QPoint& newPos = QCursor::pos () - QPoint (0, size.height ());
		SmilesTooltip_->move (newPos);
	}

	void MsgFormatterWidget::handleEmoPackChanged ()
	{
		const auto& emoPack = XmlSettingsManager::Instance ()
				.property ("SmileIcons").toString ();

		const auto src = Core::Instance ().GetCurrentEmoSource ();

		AddEmoticon_->setEnabled (src);

		if (!src)
			return;

		const auto& images = src->GetReprImages (emoPack);

		if (const auto lay = SmilesTooltip_->layout ())
		{
			while (const auto item = lay->takeAt (0))
				delete item;
			delete lay;
		}

		qDeleteAll (SmilesTooltip_->children ());

		const auto layout = new QGridLayout (SmilesTooltip_);
		layout->setSpacing (0);
		layout->setContentsMargins (1, 1, 1, 1);
		const int numRows = std::sqrt (static_cast<double> (images.size ())) + 1;
		int pos = 0;
		QSize maxSize;
		QList<QToolButton*> buttons;
		for (const auto& pair : Util::Stlize (images))
		{
			const auto& size = pair.first.size ();
			maxSize.setWidth (std::max (size.width (), maxSize.width ()));
			maxSize.setHeight (std::max (size.height (), maxSize.height ()));

			const QIcon icon (QPixmap::fromImage (pair.first));
			QAction *action = new QAction (icon, pair.second, this);
			action->setToolTip (pair.second);
			action->setProperty ("Text", pair.second);

			connect (action,
					SIGNAL (triggered ()),
					this,
					SLOT (insertEmoticon ()));

			const auto button = new QToolButton { SmilesTooltip_ };
			button->setDefaultAction (action);
			buttons << button;

			layout->addWidget (button, pos / numRows, pos % numRows);
			++pos;
		}

		for (const auto button : buttons)
			button->setIconSize (maxSize);

		SmilesTooltip_->setLayout (layout);
		SmilesTooltip_->adjustSize ();
		SmilesTooltip_->setMaximumSize (SmilesTooltip_->sizeHint ());
	}

	void MsgFormatterWidget::insertEmoticon ()
	{
		const QString& text = sender ()->property ("Text").toString ();
		Edit_->textCursor ().insertText (text + " ");

		if (SmilesTooltip_ &&
				!(QApplication::keyboardModifiers () & Qt::ControlModifier))
			SmilesTooltip_->hide ();
	}

	void MsgFormatterWidget::checkCleared ()
	{
		if (Edit_->toPlainText ().simplified ().isEmpty ())
			updateState (Edit_->currentCharFormat ());
	}

	void MsgFormatterWidget::updateState (const QTextCharFormat& fmt)
	{
		FormatBold_->setChecked (fmt.fontWeight () != QFont::Normal);
		FormatItalic_->setChecked (fmt.fontItalic ());
		FormatUnderline_->setChecked (fmt.fontUnderline ());
		FormatStrikeThrough_->setChecked (fmt.fontStrikeOut ());
	}
}
}
