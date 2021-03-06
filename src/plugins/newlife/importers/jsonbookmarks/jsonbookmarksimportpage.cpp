/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2014  Georg Rudoy
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE or copy at https://www.boost.org/LICENSE_1_0.txt)
 **********************************************************************/

#include "jsonbookmarksimportpage.h"
#include <QFile>
#include <QMessageBox>
#include <QFileDialog>
#include <QtDebug>
#include <util/sll/parsejson.h>
#include <util/sll/prelude.h>
#include <util/xpc/util.h>

namespace LC
{
namespace NewLife
{
namespace Importers
{
	JsonBookmarksImportPage::JsonBookmarksImportPage (const ICoreProxy_ptr& proxy, QWidget *parent)
	: EntityGeneratingPage { proxy, parent }
	{
		Ui_.setupUi (this);

		connect (Ui_.Browse_,
				&QPushButton::released,
				this,
				&JsonBookmarksImportPage::BrowseFile);

		connect (Ui_.Path_,
				&QLineEdit::textChanged,
				this,
				&QWizardPage::completeChanged);
	}

	int JsonBookmarksImportPage::nextId () const
	{
		return -1;
	}

	void JsonBookmarksImportPage::initializePage ()
	{
		connect (wizard (),
				&QWizard::accepted,
				this,
				&JsonBookmarksImportPage::HandleAccepted);

		if (!isComplete ())
			BrowseFile ();
	}

	bool JsonBookmarksImportPage::isComplete () const
	{
		const auto& filePath = Ui_.Path_->text ();
		if (!QFile::exists (filePath))
			return false;

		QFile file { filePath };
		if (!file.open (QIODevice::ReadOnly))
		{
			qDebug () << Q_FUNC_INFO
					<< file.errorString ();
			return false;
		}

		const auto& map = Util::ParseJson (&file, Q_FUNC_INFO).toMap ();
		return map.value ("roots").toMap ().size ();
	}

	namespace
	{
		struct Bookmark
		{
			QString Title_;
			QString URL_;
			QStringList Tags_;
		};

		template<typename T>
		QList<Bookmark> CollectBookmarks (const T& roots, const QStringList& tags)
		{
			QList<Bookmark> result;

			for (const auto& childVar : roots)
			{
				const auto& child = childVar.toMap ();

				const auto& name = child.value ("name").toString ();
				const auto& type = child.value ("type").toString ();

				if (type == "folder")
				{
					auto childTags = tags;
					if (!name.isEmpty ())
					{
						childTags << name;
						childTags.removeDuplicates ();
					}

					result += CollectBookmarks (child.value ("children").toList (), childTags);
				}
				else if (type == "url")
					result.push_back ({ name, child.value ("url").toString (), tags });
				else
					qWarning () << Q_FUNC_INFO
							<< "unknown element type"
							<< type;
			}

			return result;
		}

		auto BuildOccurencesMap (const QList<Bookmark>& items)
		{
			QHash<QString, int> result;
			for (const auto& item : items)
				for (const auto& tag : item.Tags_)
					++result [tag];
			return result;
		}

		QList<Bookmark> FixTags (QList<Bookmark> items)
		{
			const auto& occurences = BuildOccurencesMap (items);

			for (auto& item : items)
				for (const auto& tag : QStringList { item.Tags_ })
					if (occurences [tag] == items.size ())
						item.Tags_.removeOne (tag);

			return items;
		}
	}

	void JsonBookmarksImportPage::HandleAccepted ()
	{
		const auto& filePath = Ui_.Path_->text ();

		QFile file { filePath };
		if (!file.open (QIODevice::ReadOnly))
		{
			qWarning () << Q_FUNC_INFO
					<< "unable to open file"
					<< filePath
					<< file.errorString ();
			QMessageBox::critical (this,
					"LeechCraft",
					tr ("Unable to open file %1: %2.")
							.arg (filePath)
							.arg (file.errorString ()));
			return;
		}

		const auto& map = Util::ParseJson (&file, Q_FUNC_INFO).toMap ();
		const auto& roots = map.value ("roots").toMap ();

		const auto& bms = FixTags (CollectBookmarks (roots, {}));

		auto entity = Util::MakeEntity ({},
				{},
				FromUserInitiated,
				"x-leechcraft/browser-import-data");
		entity.Additional_ ["BrowserBookmarks"] = Util::Map (bms,
				[] (const Bookmark& bm) -> QVariant
				{
					return QVariantMap
					{
						{ "Title", bm.Title_ },
						{ "URL", bm.URL_ },
						{ "Tags", bm.Tags_ }
					};
				});
		SendEntity (entity);
	}

	void JsonBookmarksImportPage::BrowseFile ()
	{
		const auto& path = QFileDialog::getOpenFileName (this,
				tr ("Select JSON bookmarks file"),
				QDir::homePath (),
				tr ("JSON files (*.json);;All files (*.*)"));

		if (!path.isEmpty ())
			Ui_.Path_->setText (path);
	}
}
}
}
