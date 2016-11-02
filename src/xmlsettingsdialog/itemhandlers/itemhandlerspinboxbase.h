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

#include "itemhandlerstringgetvalue.h"
#include <functional>
#include <QGridLayout>
#include <QLabel>
#include <QtDebug>
#include "../xmlsettingsdialog.h"

namespace LeechCraft
{
	template<typename WidgetType, typename ValueType>
	class ItemHandlerSpinboxBase : public ItemHandlerStringGetValue
	{
	public:
		using Converter_t = std::function<ValueType (QString)>;
	private:
		Converter_t Converter_;
		QString ElementType_;
		const char *ChangedSignal_;
	public:
		ItemHandlerSpinboxBase (Converter_t cvt, const QString& etype, const char *cs, Util::XmlSettingsDialog *xsd)
		: ItemHandlerStringGetValue { xsd }
		, Converter_ { cvt }
		, ElementType_ { etype }
		, ChangedSignal_ { cs }
		{
		}

		bool CanHandle (const QDomElement& element) const
		{
			return element.attribute ("type") == ElementType_;
		}

		void Handle (const QDomElement& item, QWidget *pwidget)
		{
			QGridLayout *lay = qobject_cast<QGridLayout*> (pwidget->layout ());
			QLabel *label = new QLabel (XSD_->GetLabel (item));
			label->setWordWrap (false);

			WidgetType *box = new WidgetType (XSD_->GetWidget ());
			XSD_->SetTooltip (box, item);
			box->setObjectName (item.attribute ("property"));

			if (item.hasAttribute ("minimum"))
				box->setMinimum (Converter_ (item.attribute ("minimum")));
			if (item.hasAttribute ("maximum"))
				box->setMaximum (Converter_ (item.attribute ("maximum")));
			if (item.hasAttribute ("step"))
				box->setSingleStep (Converter_ (item.attribute ("step")));
			if (item.hasAttribute ("suffix"))
				box->setSuffix (item.attribute ("suffix"));

			const auto& langs = XSD_->GetLangElements (item);
			if (langs.Valid_)
			{
				if (langs.Label_.first)
					label->setText (langs.Label_.second);
				if (langs.Suffix_.first)
					box->setSuffix (langs.Suffix_.second);
				if (langs.SpecialValue_.first)
					box->setSpecialValueText (langs.SpecialValue_.second);
			}

			QVariant value = XSD_->GetValue (item);

			box->setValue (value.value<ValueType> ());
			connect (box,
					ChangedSignal_,
					this,
					SLOT (updatePreferences ()));

			box->setProperty ("ItemHandler", QVariant::fromValue<QObject*> (this));
			box->setProperty ("SearchTerms", label->text ());

			int row = lay->rowCount ();
			lay->setColumnMinimumWidth (0, 10);
			lay->setColumnStretch (0, 1);
			lay->setColumnStretch (1, 5);
			lay->addWidget (label, row, 0, Qt::AlignRight);
			lay->addWidget (box, row, 1);
		}

		void SetValue (QWidget *widget,
					const QVariant& value) const
		{
			WidgetType *spinbox = qobject_cast<WidgetType*> (widget);
			if (!spinbox)
			{
				qWarning () << Q_FUNC_INFO
					<< "not an expected class"
					<< widget;
				return;
			}
			spinbox->setValue (value.value<ValueType> ());
		}
	protected:
		QVariant GetObjectValue (QObject *object) const
		{
			WidgetType *spinbox = qobject_cast<WidgetType*> (object);
			if (!spinbox)
			{
				qWarning () << Q_FUNC_INFO
					<< "not an expected class"
					<< object;
				return QVariant ();
			}
			return spinbox->value ();
		}
	};
}
