/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2014  Georg Rudoy
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE or copy at https://www.boost.org/LICENSE_1_0.txt)
 **********************************************************************/

#include "contextwrapper.h"
#include <QSortFilterProxyModel>
#include <QSettings>
#include <QStringList>
#include <QCoreApplication>
#include <QtDebug>
#include <util/qml/unhidelistviewbase.h>
#include <util/qml/unhidelistmodel.h>
#include <util/gui/autoresizemixin.h>
#include <util/models/rolenamesmixin.h>
#include "plotmanager.h"
#include "sensorsgraphmodel.h"

namespace LC
{
namespace HotSensors
{
	class SensorsFilterModel : public Util::RoleNamesMixin<QSortFilterProxyModel>
	{
		QStringList Hidden_;
	public:
		SensorsFilterModel (QObject *parent)
		: RoleNamesMixin<QSortFilterProxyModel> (parent)
		{
		}

		void SetSourceWithRoles (QAbstractItemModel *model)
		{
			setSourceModel (model);
			setRoleNames (model->roleNames ());
		}

		void SetHidden (const QStringList& hidden)
		{
			Hidden_ = hidden;
			invalidateFilter ();
		}

	protected:
		bool filterAcceptsRow (int row, const QModelIndex&) const
		{
			const auto& idx = sourceModel ()->index (row, 0);
			return !Hidden_.contains (idx.data (SensorsGraphModel::Role::SensorName).toString ());
		}
	};

	ContextWrapper::ContextWrapper (QAbstractItemModel *model, ICoreProxy_ptr proxy, QObject *parent)
	: QObject (parent)
	, Proxy_ (proxy)
	, Filter_ (new SensorsFilterModel (this))
	{
		Filter_->setDynamicSortFilter (true);
		Filter_->SetSourceWithRoles (model);
	}

	QStringList ContextWrapper::LoadHiddenNames () const
	{
		QSettings settings (qApp->organizationName (), qApp->applicationName () + "_HotSensors");
		settings.beginGroup (Context_);
		const auto& list = settings.value ("Hidden").toStringList ();
		settings.endGroup ();
		return list;
	}

	void ContextWrapper::SaveHiddenNames (const QStringList& names) const
	{
		QSettings settings (qApp->organizationName (), qApp->applicationName () + "_HotSensors");
		settings.beginGroup (Context_);
		settings.setValue ("Hidden", names);
		settings.endGroup ();
	}

	void ContextWrapper::setContext (const QString& context)
	{
		Context_ = context;

		const auto& list = LoadHiddenNames ();
		Filter_->SetHidden (list);
	}

	QObject* ContextWrapper::getModel () const
	{
		return Filter_;
	}

	void ContextWrapper::sensorUnhideListRequested (int x, int y, const QRect& rect)
	{
		if (CurrentList_)
		{
			CurrentList_->deleteLater ();
			return;
		}

		QList<QStandardItem*> items;
		for (const auto& name : LoadHiddenNames ())
		{
			auto item = new QStandardItem;
			item->setData (name, Util::UnhideListModel::ItemClass);
			item->setData (name, Util::UnhideListModel::ItemName);
			item->setData (QUrl (), Util::UnhideListModel::ItemIcon);
			item->setData (QString (), Util::UnhideListModel::ItemDescription);
			items << item;
		}
		if (items.isEmpty ())
			return;

		auto list = new Util::UnhideListViewBase (Proxy_,
				[&items] (QStandardItemModel *model)
					{ model->invisibleRootItem ()->appendRows (items); });
		connect (list,
				SIGNAL (itemUnhideRequested (QString)),
				this,
				SLOT (unhideSensor (QString)));
		new Util::AutoResizeMixin ({ x, y }, [rect] () { return rect; }, list);
		list->show ();
		CurrentList_ = list;
	}

	void ContextWrapper::unhideSensor (const QString& name)
	{
		auto list = LoadHiddenNames ();
		if (!list.removeAll (name))
			return;

		SaveHiddenNames (list);
		Filter_->SetHidden (list);
	}

	void ContextWrapper::hideSensor (const QString& name)
	{
		auto list = LoadHiddenNames ();
		list << name;
		list.removeDuplicates ();
		SaveHiddenNames (list);

		Filter_->SetHidden (list);
	}
}
}
