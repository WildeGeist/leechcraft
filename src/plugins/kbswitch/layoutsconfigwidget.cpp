/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2014  Georg Rudoy
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE or copy at https://www.boost.org/LICENSE_1_0.txt)
 **********************************************************************/

#include "layoutsconfigwidget.h"
#include <QStandardItemModel>
#include <QStyledItemDelegate>
#include <QComboBox>
#include <QtDebug>
#include <QSettings>
#include <util/models/modeliterator.h>
#include <util/sll/prelude.h>
#include "kbctl.h"
#include "flagiconprovider.h"
#include "rulesstorage.h"

namespace LC
{
namespace KBSwitch
{
	namespace
	{
		QList<QStringList> ToSortedList (const QHash<QString, QString>& hash)
		{
			QList<QStringList> result;
			for (auto i = hash.begin (); i != hash.end (); ++i)
				result.append ({ i.key (), i.value () });

			std::sort (result.begin (), result.end (),
					Util::ComparingBy ([] (const auto& list) { return list.at (0); }));

			return result;
		}

		QList<QStandardItem*> List2Row (const QStringList& list)
		{
			FlagIconProvider flagProv;

			QList<QStandardItem*> row;
			for (const auto& item : list)
				row << new QStandardItem (item);

			const auto& img = flagProv.requestPixmap (list.at (0), nullptr, {});
			row.first ()->setIcon ({ img });

			row.value (0)->setEditable (false);
			row.value (1)->setEditable (false);

			return row;
		}

		void SetList (const QList<QStringList>& lists, QStandardItemModel *model)
		{
			for (const auto& list : lists)
				model->appendRow (List2Row (list));
		}

		class EnabledItemDelegate : public QStyledItemDelegate
		{
		public:
			EnabledItemDelegate (QObject *parent = 0)
			: QStyledItemDelegate (parent)
			{
			}

			QWidget* createEditor (QWidget *parent,
					const QStyleOptionViewItem& option, const QModelIndex& index) const
			{
				if (index.column () != LayoutsConfigWidget::EnabledColumn::EnabledVariant)
					return QStyledItemDelegate::createEditor (parent, option, index);

				return new QComboBox (parent);
			}

			void setEditorData (QWidget *editor, const QModelIndex& index) const
			{
				if (index.column () != LayoutsConfigWidget::EnabledColumn::EnabledVariant)
					return QStyledItemDelegate::setEditorData (editor, index);

				const auto& codeIdx = index.sibling (index.row (),
						LayoutsConfigWidget::EnabledColumn::EnabledCode);
				const auto& code = codeIdx.data ().toString ();

				const auto& variants = KBCtl::Instance ()
						.GetRulesStorage ()->GetLayoutVariants (code);

				auto box = qobject_cast<QComboBox*> (editor);
				box->clear ();
				box->addItem ({});
				box->addItems (variants);
			}

			void setModelData (QWidget *editor, QAbstractItemModel *model, const QModelIndex& index) const
			{
				if (index.column () != LayoutsConfigWidget::EnabledColumn::EnabledVariant)
					return QStyledItemDelegate::setModelData (editor, model, index);

				const auto& item = qobject_cast<QComboBox*> (editor)->currentText ();
				model->setData (index, item, Qt::DisplayRole);
			}
		};
	}

	LayoutsConfigWidget::LayoutsConfigWidget (QWidget *parent)
	: QWidget (parent)
	, AvailableModel_ (new QStandardItemModel (this))
	, EnabledModel_ (new QStandardItemModel (this))
	{
		QStringList availHeaders { tr ("Code"), tr ("Description") };
		AvailableModel_->setHorizontalHeaderLabels (availHeaders);
		EnabledModel_->setHorizontalHeaderLabels (availHeaders << tr ("Variant"));

		FillModels ();

		Ui_.setupUi (this);
		Ui_.AvailableView_->setModel (AvailableModel_);
		Ui_.EnabledView_->setModel (EnabledModel_);

		Ui_.EnabledView_->setItemDelegate (new EnabledItemDelegate (Ui_.EnabledView_));

		connect (Ui_.AvailableView_->selectionModel (),
				SIGNAL (currentRowChanged (QModelIndex, QModelIndex)),
				this,
				SLOT (updateActionsState ()));
		connect (Ui_.EnabledView_->selectionModel (),
				SIGNAL (currentRowChanged (QModelIndex, QModelIndex)),
				this,
				SLOT (updateActionsState ()));
		updateActionsState ();
	}

	void LayoutsConfigWidget::FillModels ()
	{
		if (auto rc = AvailableModel_->rowCount ())
			AvailableModel_->removeRows (0, rc);
		if (auto rc = EnabledModel_->rowCount ())
			EnabledModel_->removeRows (0, rc);

		auto layouts = KBCtl::Instance ().GetRulesStorage ()->GetLayoutsN2D ();

		const auto& enabledGroups = KBCtl::Instance ().GetEnabledGroups ();

		QList<QStringList> enabled;
		for (auto i = 0; i != enabledGroups.size (); ++i)
		{
			const auto& name = enabledGroups.at (i);

			const QStringList enabledRow
			{
				name,
				layouts.value (name),
				KBCtl::Instance ().GetGroupVariant (i)
			};
			enabled << enabledRow;
		}

		Layouts_ = ToSortedList (layouts);
		SetList (Layouts_, AvailableModel_);
		SetList (enabled, EnabledModel_);
	}

	void LayoutsConfigWidget::accept ()
	{
		QStringList codes;
		QStringList variants;
		for (int i = 0; i < EnabledModel_->rowCount (); ++i)
		{
			const auto& code = EnabledModel_->item (i, EnabledColumn::EnabledCode)->text ();
			codes << code;

			const auto& variant = EnabledModel_->item (i, EnabledColumn::EnabledVariant)->text ();
			variants << variant;
		}
		KBCtl::Instance ().SetEnabledGroups (codes);
		KBCtl::Instance ().SetGroupVariants (variants);
	}

	void LayoutsConfigWidget::reject ()
	{
		FillModels ();
	}

	void LayoutsConfigWidget::on_Enable__released ()
	{
		const auto& toEnableIdx = Ui_.AvailableView_->currentIndex ();
		if (!toEnableIdx.isValid ())
			return;

		auto row = List2Row (Layouts_.value (toEnableIdx.row ()));
		row << new QStandardItem;
		EnabledModel_->appendRow (row);
	}

	void LayoutsConfigWidget::on_Disable__released ()
	{
		const auto& toDisableIdx = Ui_.EnabledView_->currentIndex ();
		if (!toDisableIdx.isValid ())
			return;

		EnabledModel_->removeRow (toDisableIdx.row ());
	}

	void LayoutsConfigWidget::updateActionsState ()
	{
		const auto& availIdx = Ui_.AvailableView_->currentIndex ();
		const auto maxEnabled = KBCtl::Instance ().GetMaxEnabledGroups ();
		Ui_.Enable_->setEnabled (availIdx.isValid () &&
				EnabledModel_->rowCount () < maxEnabled);

		Ui_.Disable_->setEnabled (Ui_.EnabledView_->currentIndex ().isValid ());
	}
}
}
