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

#include <tuple>
#include <QtTest>
#include <QSqlError>
#include <oral/oral.h>

namespace lco = LeechCraft::Util::oral;

#define ORAL_FACTORY_SQLITE 1
#define ORAL_FACTORY_POSTGRES 2

#if ORAL_FACTORY == ORAL_FACTORY_SQLITE

using OralFactory = lco::SQLiteImplFactory;

#elif ORAL_FACTORY == ORAL_FACTORY_POSTGRES

#include <oral/pgimpl.h>

using OralFactory = lco::PostgreSQLImplFactory;

#else

#error "Unknown oral tests factory"

#endif

template<typename T, typename = decltype (T {}.AsTuple ())>
auto operator== (const T& left, const T& right)
{
	return left.AsTuple () == right.AsTuple ();
}

namespace LeechCraft::Util::oral
{
	template<typename T, typename... Args>
	char* toString (const PKey<T, Args...>& pkey)
	{
		return QTest::toString (pkey.Val_);
	}
}

#define TOSTRING(n) inline char* toString (const n& rec) { return toString (#n, rec); }

template<typename T, typename TupleType = decltype (T {}.AsTuple ())>
char* toString (const char *name, const T& t)
{
	using QTest::toString;

	QByteArray ba { name };
	ba.append (" { ");

	std::apply ([&ba] (const auto&... args) { (ba.append (toString (args)).append (", "), ...); }, t.AsTuple ());

	if (std::tuple_size<TupleType>::value >= 1)
		ba.chop (2);
	ba.append (" }");

	return qstrdup (ba.data ());
}

namespace LeechCraft::Util
{
	QSqlDatabase MakeDatabase (const QString& name = ":memory:")
	{
#if ORAL_FACTORY == ORAL_FACTORY_SQLITE
		auto db = QSqlDatabase::addDatabase ("QSQLITE", Util::GenConnectionName ("TestConnection"));
		db.setDatabaseName (name);
		if (!db.open ())
			throw std::runtime_error { "cannot create test database" };

		RunTextQuery (db, "PRAGMA foreign_keys = ON;");

		return db;
#elif ORAL_FACTORY == ORAL_FACTORY_POSTGRES
		Q_UNUSED (name)

		auto db = QSqlDatabase::addDatabase ("QPSQL", Util::GenConnectionName ("TestConnection"));

		db.setHostName ("localhost");
		db.setPort (5432);
		db.setUserName (qgetenv ("TEST_POSTGRES_USERNAME"));

		if (!db.open ())
		{
			DBLock::DumpError (db.lastError ());
			throw std::runtime_error { "cannot create test database" };
		}

		return db;
#endif
	}

	template<typename T>
	auto PrepareRecords (QSqlDatabase db, int count = 3)
	{
		auto adapted = Util::oral::AdaptPtr<T, OralFactory> (db);
		for (int i = 0; i < count; ++i)
			adapted->Insert ({ i, QString::number (i) });
		return adapted;
	}
}
