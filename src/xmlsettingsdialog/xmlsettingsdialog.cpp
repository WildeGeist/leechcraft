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

#include "xmlsettingsdialog.h"
#include <stdexcept>
#include <QFile>
#include <QtDebug>
#include <QGroupBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QListWidget>
#include <QStackedWidget>
#include <QLabel>
#include <QGridLayout>
#include <QApplication>
#include <QUrl>
#include <QScrollArea>
#include <QComboBox>
#include <QTextDocument>
#include <QDomNodeList>
#include <QtScript>
#include <util/util.h>
#include <util/sll/qtutil.h>
#include <util/sll/util.h>
#include "itemhandlerfactory.h"
#include "basesettingsmanager.h"
#include "settingsthreadmanager.h"

namespace LeechCraft
{
namespace Util
{
	XmlSettingsDialog::XmlSettingsDialog ()
	: Pages_ { new QStackedWidget { this } }
	, Document_ { std::make_shared<QDomDocument> () }
	, HandlersManager_ { new ItemHandlerFactory { this } }
	{
		const auto mainLay = new QHBoxLayout (this);
		mainLay->setContentsMargins (0, 0, 0, 0);
		mainLay->addWidget (Pages_);
		setLayout (mainLay);
	}

	XmlSettingsDialog::~XmlSettingsDialog ()
	{
		if (WorkingObject_)
			SettingsThreadManager::Instance ().Flush (WorkingObject_);
	}

	void XmlSettingsDialog::RegisterObject (BaseSettingsManager *obj, const QString& basename)
	{
		Basename_ = QFileInfo (basename).baseName ();
		TrContext_ = basename.endsWith (".xml") ?
				Basename_ :
				QFileInfo (basename).fileName ();
		WorkingObject_ = obj;
		QString filename;
		if (QFile::exists (basename))
			filename = basename;
		else if (QFile::exists (QString (":/") + basename))
			filename = QString (":/") + basename;
	#ifdef Q_OS_WIN32
		else if (QFile::exists (QApplication::applicationDirPath () + "/settings/" + basename))
			filename = QApplication::applicationDirPath () + "/settings/" + basename;
	#elif defined (Q_OS_MAC)
		else if (QFile::exists (QApplication::applicationDirPath () +
				"/../Resources/settings/" + basename))
			filename = QApplication::applicationDirPath () +
					"/../Resources/settings/" + basename;
		else if (QFile::exists (QString ("/usr/local/share/leechcraft/settings/") + basename))
			filename = QString ("/usr/local/share/leechcraft/settings/") + basename;
	#elif defined (INSTALL_PREFIX)
		else if (QFile::exists (QString (INSTALL_PREFIX "/share/leechcraft/settings/") + basename))
			filename = QString (INSTALL_PREFIX "/share/leechcraft/settings/") + basename;
	#else
		else if (QFile::exists (QString ("/usr/local/share/leechcraft/settings/") + basename))
			filename = QString ("/usr/local/share/leechcraft/settings/") + basename;
		else if (QFile::exists (QString ("/usr/share/leechcraft/settings/") + basename))
			filename = QString ("/usr/share/leechcraft/settings/") + basename;
	#endif
		QFile file (filename);
		if (!file.open (QIODevice::ReadOnly))
		{
			qWarning () << "cannot open file"
				<< filename
				<< basename;
			return;
		}
		const QByteArray& data = file.readAll ();
		file.close ();

		QString emsg;
		int eline;
		int ecol;
		if (!Document_->setContent (data, &emsg, &eline, &ecol))
		{
			qWarning () << "Could not parse file, line"
				<< eline
				<< "; column"
				<< ecol
				<< emsg;
			return;
		}
		const QDomElement& root = Document_->documentElement ();
		if (root.tagName () != "settings")
		{
			qWarning () << "Bad settings file";
			return;
		}

		{
			auto initGuard = obj->EnterInitMode ();

			QDomElement declaration = root.firstChildElement ("declare");
			while (!declaration.isNull ())
			{
				HandleDeclaration (declaration);
				declaration = declaration.nextSiblingElement ("declare");
			}

			QDomElement pageChild = root.firstChildElement ("page");
			while (!pageChild.isNull ())
			{
				ParsePage (pageChild);
				pageChild = pageChild.nextSiblingElement ("page");
			}
		}

		obj->installEventFilter (this);

		UpdateXml (true);

		connect (obj,
				SIGNAL (showPageRequested (Util::BaseSettingsManager*, QString)),
				this,
				SLOT (handleShowPageRequested (Util::BaseSettingsManager*, QString)));
	}

	BaseSettingsManager* XmlSettingsDialog::GetManagerObject () const
	{
		return WorkingObject_;
	}

	QString XmlSettingsDialog::GetXml () const
	{
		return Document_->toString ();
	}

	void XmlSettingsDialog::MergeXml (const QByteArray& newXml)
	{
		QDomDocument newDoc;
		newDoc.setContent (newXml);

		QList<QByteArray> props = WorkingObject_->dynamicPropertyNames ();

		QDomNodeList nodes = newDoc.elementsByTagName ("item");
		for (int i = 0; i < nodes.size (); ++i)
		{
			const QDomElement& elem = nodes.at (i).toElement ();
			if (elem.isNull ())
				continue;

			const QString& propName = elem.attribute ("property");
			if (!props.contains (propName.toLatin1 ()))
				continue;

			const QVariant& value = GetValue (elem);
			if (value.isNull ())
				continue;

			WorkingObject_->setProperty (propName.toLatin1 ().constData (), value);

			QWidget *object = findChild<QWidget*> (propName);
			if (!object)
			{
				qWarning () << Q_FUNC_INFO
					<< "could not find object for property"
					<< propName;
				continue;
			}
			HandlersManager_->SetValue (object, value);
		}

		UpdateXml ();
		HandlersManager_->ClearNewValues ();
	}

	namespace
	{
		QList<QWidget*> FindDirectChildren (QWidget *widget)
		{
			auto result = widget->findChildren<QWidget*> ();
			for (auto i = result.begin (); i != result.end (); )
			{
				const auto& sub = *i;
				if (sub->parentWidget () && sub->parentWidget () != widget)
					i = result.erase (i);
				else
					++i;
			}
			return result;
		}

		void EnableChildren (QWidget *widget)
		{
			Q_FOREACH (auto tab, widget->findChildren<QTabWidget*> ())
				for (int i = 0; i < tab->count (); ++i)
					tab->setTabEnabled (i, true);

			Q_FOREACH (auto child, widget->findChildren<QWidget*> () << widget)
				child->setEnabled (true);
		}

		bool HighlightWidget (QWidget *widget, const QString& query, ItemHandlerFactory *factory)
		{
			bool result = false;

			auto allChildren = FindDirectChildren (widget);

			const auto& terms = widget->property ("SearchTerms").toStringList ();
			if (!terms.isEmpty ())
			{
				Q_FOREACH (const auto& term, terms)
					if (term.contains (query, Qt::CaseInsensitive))
					{
						result = true;
						break;
					}
				if (result)
				{
					EnableChildren (widget);
					return true;
				}
			}

			Q_FOREACH (auto tab, widget->findChildren<QTabWidget*> ())
				for (int i = 0; i < tab->count (); ++i)
				{
					const bool tabTextMatches = tab->tabText (i).contains (query, Qt::CaseInsensitive);
					const bool tabMatches = tabTextMatches ||
							HighlightWidget (tab->widget (i), query, factory);
					tab->setTabEnabled (i, tabMatches);
					if (tabTextMatches)
						EnableChildren (tab->widget (i));

					Q_FOREACH (auto tabChild, tab->findChildren<QWidget*> ())
						allChildren.removeAll (tabChild);

					if (tabMatches)
					{
						tab->setEnabled (true);
						result = true;
					}
				}

			Q_FOREACH (auto child, allChildren)
				if (HighlightWidget (child, query, factory))
					result = true;

			widget->setEnabled (result);

			return result;
		}
	}

	QList<int> XmlSettingsDialog::HighlightMatches (const QString& query)
	{
		QList<int> result;
		if (query.isEmpty ())
		{
			for (int i = 0; i < Pages_->count (); ++i)
			{
				EnableChildren (Pages_->widget (i));
				result << i;
			}
			return result;
		}

		for (int i = 0; i < Pages_->count (); ++i)
		{
			if (Titles_.at (i).contains (query, Qt::CaseInsensitive))
			{
				EnableChildren (Pages_->widget (i));
				result << i;
				continue;
			}

			if (HighlightWidget (Pages_->widget (i), query, HandlersManager_))
				result << i;
		}

		return result;
	}

	void XmlSettingsDialog::SetCustomWidget (const QString& name, QWidget *widget)
	{
		const auto& widgets = findChildren<QWidget*> (name);
		if (!widgets.size ())
			throw std::runtime_error (qPrintable (QString ("Widget %1 not "
							"found").arg (name)));
		if (widgets.size () > 1)
			throw std::runtime_error (qPrintable (QString ("Widget %1 "
							"appears to exist more than once").arg (name)));

		widgets.at (0)->layout ()->addWidget (widget);
		Customs_ << widget;
		connect (widget,
				SIGNAL (destroyed (QObject*)),
				this,
				SLOT (handleCustomDestroyed ()));
	}

	void XmlSettingsDialog::SetDataSource (const QString& property,
			QAbstractItemModel *dataSource)
	{
		HandlersManager_->SetDataSource (property, dataSource, this);
	}

	void XmlSettingsDialog::SetPage (int page)
	{
		Pages_->setCurrentIndex (page);
	}

	QStringList XmlSettingsDialog::GetPages () const
	{
		return Titles_;
	}

	QIcon XmlSettingsDialog::GetPageIcon (int page) const
	{
		for (const auto& name : IconNames_.value (page))
		{
			const QString themeMarker { "theme://" };
			if (name.startsWith (themeMarker))
				return QIcon::fromTheme (name.mid (themeMarker.size ()));

			const QIcon icon { name };
			if (!icon.isNull ())
				return icon;
		}

		return {};
	}

	void XmlSettingsDialog::HandleDeclaration (const QDomElement& decl)
	{
		if (decl.hasAttribute ("defaultlang"))
			DefaultLang_ = decl.attribute ("defaultlang");
	}

	void XmlSettingsDialog::ParsePage (const QDomElement& page)
	{
		Titles_ << GetLabel (page);

		QStringList icons;
		if (page.hasAttribute ("icon"))
			icons << page.attribute ("icon");
		auto iconElem = page
				.firstChildElement ("icons")
				.firstChildElement ("icon");
		while (!iconElem.isNull ())
		{
			icons << iconElem.text ();
			iconElem = iconElem.nextSiblingElement ("icon");
		}
		IconNames_ << icons;

		const auto baseWidget = new QWidget;
		Pages_->addWidget (baseWidget);
		const auto lay = new QGridLayout;
		lay->setContentsMargins (0, 0, 0, 0);
		baseWidget->setLayout (lay);

		ParseEntity (page, baseWidget);

		bool foundExpanding = false;

		for (const auto w : baseWidget->findChildren<QWidget*> ())
			if (w->sizePolicy ().verticalPolicy () & QSizePolicy::ExpandFlag)
			{
				foundExpanding = true;
				break;
			}

		if (!foundExpanding)
			lay->addItem (new QSpacerItem (0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding),
					lay->rowCount (), 0, 1, 2);
	}

	void XmlSettingsDialog::ParseEntity (const QDomElement& entity, QWidget *baseWidget)
	{
		QDomElement item = entity.firstChildElement ("item");
		while (!item.isNull ())
		{
			ParseItem (item, baseWidget);
			item = item.nextSiblingElement ("item");
		}

		auto gbox = entity.firstChildElement ("groupbox");
		while (!gbox.isNull ())
		{
			const auto box = new QGroupBox (GetLabel (gbox));
			box->setSizePolicy (QSizePolicy::Expanding, QSizePolicy::Preferred);
			const auto groupLayout = new QGridLayout ();
			groupLayout->setContentsMargins (2, 2, 2, 2);
			box->setLayout (groupLayout);

			ParseEntity (gbox, box);

			const auto lay = qobject_cast<QGridLayout*> (baseWidget->layout ());
			lay->addWidget (box, lay->rowCount (), 0);

			gbox = gbox.nextSiblingElement ("groupbox");
		}

		auto scroll = entity.firstChildElement ("scrollarea");
		while (!scroll.isNull ())
		{
			const auto area = new QScrollArea ();
			if (scroll.hasAttribute ("horizontalScroll"))
			{
				const auto& attr = scroll.attribute ("horizontalScroll");
				if (attr == "on")
					area->setHorizontalScrollBarPolicy (Qt::ScrollBarAlwaysOn);
				else if (attr == "off")
					area->setHorizontalScrollBarPolicy (Qt::ScrollBarAlwaysOff);
			}
			if (scroll.hasAttribute ("verticalScroll"))
			{
				const auto& attr = scroll.attribute ("verticalScroll");
				if (attr == "on")
					area->setVerticalScrollBarPolicy (Qt::ScrollBarAlwaysOn);
				else if (attr == "off")
					area->setVerticalScrollBarPolicy (Qt::ScrollBarAlwaysOff);
			}

			area->setSizePolicy (QSizePolicy::Expanding, QSizePolicy::Expanding);

			const auto areaWidget = new QFrame;
			areaWidget->setSizePolicy (QSizePolicy::Expanding, QSizePolicy::Expanding);
			const auto areaLayout = new QGridLayout;
			areaWidget->setLayout (areaLayout);
			ParseEntity (scroll, areaWidget);
			area->setWidget (areaWidget);
			area->setWidgetResizable (true);
			areaWidget->show ();

			const auto lay = qobject_cast<QGridLayout*> (baseWidget->layout ());
			const auto thisRow = lay->rowCount ();
			lay->addWidget (area, thisRow, 0, 1, 2);
			lay->setRowStretch (thisRow, 1);

			scroll = scroll.nextSiblingElement ("scrollarea");
		}

		auto tab = entity.firstChildElement ("tab");
		if (!tab.isNull ())
		{
			const auto tabs = new QTabWidget;
			const auto lay = qobject_cast<QGridLayout*> (baseWidget->layout ());
			lay->addWidget (tabs, lay->rowCount (), 0, 1, 2);
			while (!tab.isNull ())
			{
				const auto page = new QWidget;
				const auto widgetLay = new QGridLayout;
				widgetLay->setContentsMargins (0, 0, 0, 0);
				page->setLayout (widgetLay);
				tabs->addTab (page, GetLabel (tab));
				ParseEntity (tab, page);
				tab = tab.nextSiblingElement ("tab");

				widgetLay->addItem (new QSpacerItem (0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding),
						widgetLay->rowCount (), 0, 1, 1);
			}
		}
	}

	void XmlSettingsDialog::ParseItem (const QDomElement& item, QWidget *baseWidget)
	{
		const QString& type = item.attribute ("type");

		const QString& property = item.attribute ("property");

		if (type.isEmpty () || type.isNull ())
			return;

		if (!HandlersManager_->Handle (item, baseWidget))
			qWarning () << Q_FUNC_INFO << "unhandled type" << type;

		WorkingObject_->setProperty (property.toLatin1 ().constData (), GetValue (item));
	}

#if defined (Q_OS_WIN32)
#include <QCoreApplication>
#include <QLocale>
	namespace
	{
		QString GetLanguageHack ()
		{
			QSettings settings (QCoreApplication::organizationName (),
					QCoreApplication::applicationName ());

			auto localeName = settings.value ("Language", "system").toString ();

			if (localeName == "system")
			{
				localeName = QString (::getenv ("LANG")).left (5);
				if (localeName.isEmpty () || localeName.size () != 5)
					localeName = QLocale::system ().name ();
			}

			return localeName.left (2);
		}
	};
#endif

	QString XmlSettingsDialog::GetLabel (const QDomElement& item) const
	{
		QString result;
		const auto& label = item.firstChildElement ("label");
		if (!label.isNull ())
			result = label.attribute ("value");
		return QCoreApplication::translate (qPrintable (TrContext_),
				result.toUtf8 ().constData (),
#if QT_VERSION < 0x050000
				0,
				QCoreApplication::Encoding::UnicodeUTF8);
#else
				0);
#endif
	}

	QString XmlSettingsDialog::GetDescription (const QDomElement& item) const
	{
		const auto& label = item.firstChildElement ("tooltip");
		const auto& text = label.text ().simplified ();
		return QCoreApplication::translate (qPrintable (TrContext_),
				text.toUtf8 ().constData ());
	}

	void XmlSettingsDialog::SetTooltip (QWidget *widget, const QDomElement& from) const
	{
		const auto& descr = GetDescription (from);
		if (!descr.isEmpty ())
			widget->setToolTip (descr);
	}

	XmlSettingsDialog::LangElements XmlSettingsDialog::GetLangElements (const QDomElement& parent) const
	{
		LangElements returning;
		returning.Valid_ = true;

		auto getElem = [&parent, this] (const QString& elemName) -> QPair<bool, QString>
		{
			const auto& label = parent.firstChildElement (elemName);
			if (label.isNull ())
				return {};

			return
			{
				true,
				QCoreApplication::translate (qPrintable (TrContext_),
						label.attribute ("value").toUtf8 ().constData (),
#if QT_VERSION < 0x050000
						0,
						QCoreApplication::Encoding::UnicodeUTF8)
#else
						0)
#endif
			};
		};

		returning.Label_ = getElem ("label");
		returning.Suffix_ = getElem ("suffix");
		returning.SpecialValue_ = getElem ("specialValue");
		return returning;
	}

	QString XmlSettingsDialog::GetBasename () const
	{
		return Basename_;
	}

	QVariant XmlSettingsDialog::GetValue (const QDomElement& item, bool ignoreObject) const
	{
		const auto& property = item.attribute ("property");

		QVariant value;
		if (ignoreObject)
		{
			auto def = item.attribute ("default");
			if (item.attribute ("translatable") == "true")
				def = QCoreApplication::translate (qPrintable (TrContext_),
						def.toUtf8 ().constData ());
			value = def;
		}
		else
		{
			const auto& tmpValue = WorkingObject_->property (property.toLatin1 ().constData ());
			if (tmpValue.isValid ())
				return tmpValue;
		}

		return HandlersManager_->GetValue (item, value);
	}

	QList<QImage> XmlSettingsDialog::GetImages (const QDomElement& item) const
	{
		QList<QImage> result;
		auto binary = item.firstChildElement ("binary");
		while (!binary.isNull ())
		{
			QByteArray data;
			if (binary.attribute ("place") == "rcc")
			{
				QFile file (binary.text ());
				if (!file.open (QIODevice::ReadOnly))
				{
					qWarning () << Q_FUNC_INFO
						<< "could not open file"
						<< binary.text ()
						<< ", because"
						<< file.errorString ();

					binary = binary.nextSiblingElement ("binary");

					continue;
				}
				data = file.readAll ();
			}
			else
			{
				const auto& base64 = binary.text ().toLatin1 ();
				data = QByteArray::fromBase64 (base64);
			}
			if (binary.attribute ("type") == "image")
			{
				const auto& image = QImage::fromData (data);
				if (!image.isNull ())
					result << image;
			}
			binary = binary.nextSiblingElement ("binary");
		}
		return result;
	}

	void XmlSettingsDialog::UpdateXml (bool whole)
	{
		const auto& nodes = Document_->elementsByTagName ("item");
		if (whole)
			for (int i = 0; i < nodes.size (); ++i)
			{
				auto elem = nodes.at (i).toElement ();
				if (!elem.hasAttribute ("property"))
					continue;

				const auto& name = elem.attribute ("property");
				const auto& value = WorkingObject_->property (name.toLatin1 ().constData ());

				UpdateSingle (name, value, elem);
			}
		else
			for (const auto& pair : Util::Stlize (HandlersManager_->GetNewValues ()))
			{
				QDomElement element;
				const auto& name = pair.first;
				for (int j = 0, size = nodes.size (); j < size; ++j)
				{
					const auto& e = nodes.at (j).toElement ();
					if (e.isNull ())
						continue;
					if (e.attribute ("property") == name)
					{
						element = e;
						break;
					}
				}
				if (element.isNull ())
				{
					qWarning () << Q_FUNC_INFO << "element for property" << name << "not found";
					return;
				}

				UpdateSingle (name, pair.second, element);
			}
	}

	void XmlSettingsDialog::UpdateSingle (const QString&,
			const QVariant& value, QDomElement& element)
	{
		HandlersManager_->UpdateSingle (element, value);
	}

	void XmlSettingsDialog::SetValue (QWidget *object, const QVariant& value)
	{
		HandlersManager_->SetValue (object, value);
	}

	bool XmlSettingsDialog::eventFilter (QObject *obj, QEvent *event)
	{
		if (event->type () == QEvent::DynamicPropertyChange)
		{
			const auto& name = static_cast<QDynamicPropertyChangeEvent*> (event)->propertyName ();

			if (const auto widget = findChild<QWidget*> (name))
				SetValue (widget, obj->property (name));

			return false;
		}
		else
			return QWidget::eventFilter (obj, event);
	}

	void XmlSettingsDialog::accept ()
	{
		for (const auto& pair : Util::Stlize (HandlersManager_->GetNewValues ()))
			WorkingObject_->setProperty (pair.first.toLatin1 ().constData (), pair.second);

		UpdateXml ();

		HandlersManager_->ClearNewValues ();

		for (auto widget : Customs_)
			QMetaObject::invokeMethod (widget, "accept");
	}

	void XmlSettingsDialog::reject ()
	{
		for (const auto& pair : Util::Stlize (HandlersManager_->GetNewValues ()))
		{
			const auto object = findChild<QWidget*> (pair.first);
			if (!object)
			{
				qWarning () << Q_FUNC_INFO
					<< "could not find object for property"
					<< pair.first;
				continue;
			}

			SetValue (object,
					WorkingObject_->property (pair.first.toLatin1 ().constData ()));
		}

		HandlersManager_->ClearNewValues ();

		for (const auto widget : Customs_)
			QMetaObject::invokeMethod (widget, "reject");
	}

	void XmlSettingsDialog::handleCustomDestroyed ()
	{
		Customs_.removeAll (qobject_cast<QWidget*> (sender ()));
	}

	void XmlSettingsDialog::handleMoreThisStuffRequested ()
	{
		emit moreThisStuffRequested (sender ()->objectName ());
	}

	void XmlSettingsDialog::handlePushButtonReleased ()
	{
		emit pushButtonClicked (sender ()->objectName ());
	}

	void XmlSettingsDialog::handleShowPageRequested (BaseSettingsManager *bsm, const QString& name)
	{
		emit showPageRequested (bsm, name);

		if (name.isEmpty ())
			return;

		auto child = findChild<QWidget*> (name);
		if (!child)
		{
			qWarning () << Q_FUNC_INFO
					<< Basename_
					<< "cannot find child"
					<< name;
			return;
		}

		QWidget *lastTabChild = nullptr;

		while (auto parent = child->parentWidget ())
		{
			const auto nextGuard = Util::MakeScopeGuard ([&child, parent] { child = parent; });

			const auto pgIdx = Pages_->indexOf (parent);
			if (pgIdx >= 0)
			{
				Pages_->setCurrentIndex (pgIdx);
				continue;
			}

			if (qobject_cast<QStackedWidget*> (parent))
				lastTabChild = child;
			else if (auto tw = qobject_cast<QTabWidget*> (parent))
				tw->setCurrentWidget (lastTabChild);
		}
	}
}
}
