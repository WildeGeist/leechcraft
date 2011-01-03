/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2010  Oleg Linkin
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 **********************************************************************/

#include "settings.h"
#include <QStandardItemModel>
#include <QFrame>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QCheckBox>
#include <QMessageBox>
#include <QDialog>
#include <QDateTime>
#include <plugininterface/util.h>
#include "onlinebookmarks.h"
#include "delicious/deliciousbookmarksservice.h"
#include "interfaces/structures.h"
#include "core.h"
#include "xmlsettingsmanager.h"

namespace LeechCraft
{
namespace Plugins
{
namespace Poshuku
{
namespace Plugins
{
namespace OnlineBookmarks
{
	Settings::Settings (QStandardItemModel *model, OnlineBookmarks *parent)
	: OnlineBookmarks_ (parent)
	, Model_ (model)
	{
		Ui_.setupUi (this);

		Ui_.AccauntsView_->setModel (model);
		Ui_.AccauntsView_->expandAll ();

		Ui_.Edit_->setEnabled (false);

		Ui_.Services_->setCurrentIndex (0);
		Ui_.LoginFrame_->hide ();
	
		connect (Ui_.Login_,
				SIGNAL (textChanged (QString)),
				this,
				SLOT (handleLoginTextChanged (QString)));

		connect (Ui_.Password_,
				SIGNAL (textChanged (QString)),
				this,
				SLOT (handlePasswordTextChanged (QString)));

		connect (Ui_.Apply_,
				SIGNAL (released ()),
				this,
				SLOT (handleStuff ()));
		
		ReadSettings ();

		BookmarksServices_ << new DeliciousBookmarksService (this);

		SetupServices ();
	}

	void Settings::ClearFrameState ()
	{
		Ui_.Login_->setText (QString ());
		Ui_.Password_->setText (QString ());
	}

	void Settings::SetupServices ()
	{
		ServicesModel_ = new QStandardItemModel (this);
		Ui_.ActiveServices_->setModel (ServicesModel_);

		Q_FOREACH (AbstractBookmarksService *as, BookmarksServices_)
		{
			Ui_.Services_->addItem (as->GetIcon (), as->GetName (),
					QVariant::fromValue<QObject*> (as));

			QStandardItem *item = new QStandardItem (as->GetIcon (), as->GetName ());
			item->setCheckable (true);
			ServicesModel_->appendRow (item);

			connect (as,
					SIGNAL (gotValidReply (bool)),
					this,
					SLOT (checkServiceAnswer (bool)));
		}
	}

	void Settings::SetPassword (const QString& password, const QString& account, const QString& service)
	{
		QList<QVariant> keys;
		keys << "org.LeechCraft.Poshuku.OnlineBookmarks." +
				service + "/" + account;

		QList<QVariant> passwordVar;
		passwordVar << password;
		QList<QVariant> values;
		values << QVariant (passwordVar);

		Entity e = Util::MakeEntity (keys,
				QString (),
				Internal,
				"x-leechcraft/data-persistent-save");
		e.Additional_ ["Values"] = values;
		e.Additional_ ["Overwrite"] = true;

		Core::Instance ().SendEntity (e);
	}

	QString Settings::GetPassword (const QString& account, const QString& service)
	{
		QList<QVariant> keys;
		keys << "org.LeechCraft.Poshuku.OnlineBookmarks." + service + "/" + account;
		const QVariantList& result =
				Util::GetPersistentData (keys, &Core::Instance ());
		if (result.size () != 1)
		{
			qWarning () << Q_FUNC_INFO
					<< "incorrect result size"
					<< result;
			return QString ();
		}

		const QVariantList& strVarList = result.at (0).toList ();
		if (strVarList.isEmpty () ||
				!strVarList.at (0).canConvert<QString> ())
		{
			qWarning () << Q_FUNC_INFO
					<< "invalid string variant list"
					<< strVarList;
			return QString ();
		}

		return strVarList.at (0).toString ();
	}

	void Settings::ReadSettings ()
	{
		Ui_.Bookmarks2Services_->setChecked (XmlSettingsManager::Instance ()->
				Property ("Sync/IsLocal2Service", 0).toBool ());
		Ui_.Bookmarks2ServicesPeriod_->setCurrentIndex (XmlSettingsManager::Instance ()->
				Property ("Sync/IsLocal2ServicePeriod", 0).toInt ());
		Ui_.Services2Bookmarks_->setChecked (XmlSettingsManager::Instance ()->
				Property ("Sync/IsService2Local", 1).toBool ());
		Ui_.Services2BookmarksPeriod_->setCurrentIndex (XmlSettingsManager::Instance ()->
				Property ("Sync/IsService2LocalPeriod", 0).toInt ());
	}

	void Settings::SetApplyEnabled (const QString& firestString, const QString& secondString)
	{
		Ui_.Apply_->setEnabled (!(firestString.isEmpty () ||
				secondString.isEmpty () ||
				!Ui_.Edit_->isChecked () &&
				!Model_->findItems (Ui_.Login_->text (),
						Qt::MatchFixedString | Qt::MatchRecursive).isEmpty ()));
	}

	QString Settings::GetSelectedName () const
	{
		return Ui_.Services_->currentText ();
	}

	void Settings::accept ()
	{
		XmlSettingsManager::Instance ()->
				setProperty ("Sync/IsLocal2Service", Ui_.Bookmarks2Services_->isChecked ());
		XmlSettingsManager::Instance ()->
				setProperty ("Sync/IsLocal2ServicePeriod", Ui_.Bookmarks2ServicesPeriod_->currentIndex ());
		XmlSettingsManager::Instance ()->
				setProperty ("Sync/IsLocal2ServiceLastSyncDate", QDateTime::currentDateTime ());
		XmlSettingsManager::Instance ()->
				setProperty ("Sync/IsService2Local", Ui_.Services2Bookmarks_->isChecked ());
		XmlSettingsManager::Instance ()->
				setProperty ("Sync/IsService2LocalPeriod", Ui_.Services2BookmarksPeriod_->currentIndex ());
		XmlSettingsManager::Instance ()->
				setProperty ("Sync/IsService2LocalLastSyncDate", QDateTime::currentDateTime ());
	}

	void Settings::on_Add__toggled (bool checked)
	{
		if (checked)
		{
			if (Ui_.Edit_->isChecked ())
				Ui_.Edit_->toggle ();
			Ui_.verticalLayout_2->insertWidget (1, Ui_.LoginFrame_);
			Ui_.LoginFrame_->show ();
		}
		else
		{
			Ui_.verticalLayout_2->removeWidget (Ui_.LoginFrame_);
			Ui_.LoginFrame_->hide ();
			ClearFrameState ();
		}
	}

	void Settings::on_Edit__toggled (bool checked)
	{
		if (checked)
		{
			if (Ui_.Add_->isChecked ())
				Ui_.Add_->toggle ();
			Ui_.verticalLayout_2->insertWidget (2, Ui_.LoginFrame_);
			Ui_.LoginFrame_->show ();
			Ui_.Login_->setText (Ui_.AccauntsView_->currentIndex ().data ().toString ());
			Ui_.Password_->setText (GetPassword (Ui_.Login_->text (),
					Ui_.AccauntsView_->currentIndex ().parent ().data ().toString ()));
		}
		else
		{
			Ui_.verticalLayout_2->removeWidget (Ui_.LoginFrame_);
			Ui_.LoginFrame_->hide ();
			ClearFrameState ();
		}
	}

	void Settings::on_Delete__released ()
	{
		if (Ui_.AccauntsView_->currentIndex ().parent () == QModelIndex ())
			return;
		else
		{
			QList<QVariant> keys;
			keys << "org.LeechCraft.Poshuku.OnlineBookmarks." +
					Ui_.AccauntsView_->currentIndex ().parent ().data ().toString () +
					"/" + Ui_.AccauntsView_->currentIndex ().data ().toString ();

			Entity e = Util::MakeEntity (keys,
					QString (),
					Internal,
					"x-leechcraft/data-persistent-clear");

			Ui_.LoginFrame_->hide ();
			Model_->removeRow (Ui_.AccauntsView_->currentIndex ().row (),
					Ui_.AccauntsView_->currentIndex ().parent ());

			if (Ui_.Add_->isChecked ())
				Ui_.Add_->toggle ();
			else if (Ui_.Edit_->isChecked ())
				Ui_.Edit_->toggle ();

			Core::Instance ().SendEntity (e);
		}
	}

	void Settings::handleStuff ()
	{
		QStringList names;
		Q_FOREACH (AbstractBookmarksService *item, BookmarksServices_)
			names << item->GetName ();

		int indexService = -1;
		if (Ui_.Add_->isChecked ())
			indexService = names.indexOf (GetSelectedName ());
		else if (Ui_.Edit_->isChecked ())
			indexService = names.indexOf (Ui_.AccauntsView_->currentIndex ().parent ().
					data ().toString ());

		BookmarksServices_.at (indexService)->
				CheckValidAccountData (Ui_.Login_->text (), Ui_.Password_->text ());

		ClearFrameState ();

		if (Ui_.Add_->isChecked ())
			Ui_.Add_->toggle ();
		else if (Ui_.Edit_->isChecked ())
			Ui_.Edit_->toggle ();
	}

	void Settings::handleLoginTextChanged (const QString& text)
	{
		SetApplyEnabled (text, Ui_.Password_->text ());
	}

	void Settings::handlePasswordTextChanged (const QString& text)
	{
		SetApplyEnabled (text, Ui_.Login_->text ());
	}

	void Settings::on_AccauntsView__clicked (const QModelIndex& index)
	{
		if (index.parent() == QModelIndex ())
		{
			if (Ui_.Edit_->isChecked ())
				Ui_.Edit_->toggle ();

			Ui_.Edit_->setEnabled (false);
			Ui_.Delete_->setEnabled (false);
		}
		else
		{
			Ui_.Edit_->setEnabled (true);
			Ui_.Delete_->setEnabled (true);
			Ui_.Login_->setText (Ui_.AccauntsView_->currentIndex ().data ().toString ());
		}
	}

	void Settings::checkServiceAnswer (bool valid)
	{
		if (valid)
		{
			QString service = "Account/" + Ui_.Services_->currentText ();
			if (XmlSettingsManager::Instance ()->property (service.toStdString ().c_str ()).isNull ())
				XmlSettingsManager::Instance ()->
						setProperty (service.toStdString ().c_str (), Ui_.Login_->text ());
			else
			{
				QStringList loginList = XmlSettingsManager::Instance ()->
						property (service.toStdString ().c_str ()).toStringList ();
				loginList << Ui_.Login_->text ();
				XmlSettingsManager::Instance ()->
						setProperty (service.toStdString ().c_str (), loginList);
			}

			QList<QStandardItem*> items = Model_->findItems (service);
			QStandardItem *serviceItem;
			if (!items.size ())
			{
				serviceItem = new QStandardItem (service);
				Model_->appendRow (serviceItem);
			}
			else
				 serviceItem = items.at (0);

			serviceItem->appendRow (new QStandardItem (Ui_.Login_->text ()));

			SetPassword (Ui_.Password_->text (), Ui_.Login_->text (), GetSelectedName ());
		}
		else
		{
			Entity e = Util::MakeNotification ("Poshuku", 
					tr ("Invalid account data"), 
					PWarning_);
			gotEntity (e);
		}
	}
}
}
}
}
}
