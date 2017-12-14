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

#include "photourlstorage.h"
#include <QUrl>
#include <QDir>
#include <QSqlError>
#include <util/db/oral.h>
#include <util/db/dblock.h>
#include <util/sll/functor.h>
#include <util/sys/paths.h>

namespace LeechCraft
{
namespace Azoth
{
namespace Murm
{
	struct PhotoUrlStorage::Record
	{
		Util::oral::PKey<qulonglong, Util::oral::NoAutogen> UserNum_;
		QByteArray BigPhotoUrl_;

		static QString ClassName ()
		{
			return "PhotoUrls";
		}
	};
}
}
}

BOOST_FUSION_ADAPT_STRUCT (LeechCraft::Azoth::Murm::PhotoUrlStorage::Record,
		UserNum_,
		BigPhotoUrl_)

namespace LeechCraft
{
namespace Azoth
{
namespace Murm
{
	PhotoUrlStorage::PhotoUrlStorage (QObject *parent)
	: QObject { parent }
	, DB_ { QSqlDatabase::addDatabase ("QSQLITE",
			Util::GenConnectionName ("org.LeechCraft.Azoth.Murm.PhotoUrls")) }
	{
		const auto& cacheDir = Util::GetUserDir (Util::UserDir::Cache, "azoth/murm");
		DB_.setDatabaseName (cacheDir.filePath ("photourls.db"));
		if (!DB_.open ())
		{
			qWarning () << Q_FUNC_INFO
					<< "cannot open the database";
			Util::DBLock::DumpError (DB_.lastError ());
			throw std::runtime_error { "Cannot create database" };
		}

		Util::RunTextQuery (DB_, "PRAGMA synchronous = NORMAL;");
		Util::RunTextQuery (DB_, "PRAGMA journal_mode = WAL;");

		AdaptedRecord_ = Util::oral::AdaptPtr<Record> (DB_);
	}

	boost::optional<QUrl> PhotoUrlStorage::GetUserUrl (qulonglong userId)
	{
		using namespace Util::oral::sph;
		using namespace Util;

		return [] (const QByteArray& ba) { return QUrl::fromEncoded (ba); } *
				AdaptedRecord_->DoSelectOneByFields_ (_1, _0 == userId);
	}

	void PhotoUrlStorage::SetUserUrl (qulonglong userId, const QUrl& url)
	{
		AdaptedRecord_->DoInsert_ ({ userId, url.toEncoded () }, Util::oral::InsertAction::Replace);
	}
}
}
}
