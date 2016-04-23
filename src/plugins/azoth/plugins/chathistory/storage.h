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

#include <memory>
#include <QSqlQuery>
#include <QHash>
#include <QVariant>
#include <QDateTime>
#include <util/sll/void.h>
#include "storagestructures.h"

class QSqlDatabase;

namespace LeechCraft
{
namespace Azoth
{
class IMessage;

namespace ChatHistory
{
	class Storage : public QObject
	{
		Q_OBJECT

		std::shared_ptr<QSqlDatabase> DB_;
		QSqlQuery UserSelector_;
		QSqlQuery AccountSelector_;
		QSqlQuery UserIDSelector_;
		QSqlQuery AccountIDSelector_;
		QSqlQuery UserInserter_;
		QSqlQuery AccountInserter_;
		QSqlQuery MessageDumper_;
		QSqlQuery UsersForAccountGetter_;
		QSqlQuery RowID2Pos_;
		QSqlQuery Date2Pos_;
		QSqlQuery GetMonthDates_;
		QSqlQuery LogsSearcher_;
		QSqlQuery LogsSearcherWOContact_;
		QSqlQuery LogsSearcherWOContactAccount_;
		QSqlQuery HistoryGetter_;
		QSqlQuery HistoryClearer_;
		QSqlQuery UserClearer_;
		QSqlQuery EntryCacheSetter_;
		QSqlQuery EntryCacheGetter_;
		QSqlQuery EntryCacheClearer_;

		QHash<QString, qint32> Users_;
		QHash<QString, qint32> Accounts_;

		QHash<qint32, QString> EntryCache_;

		struct RawSearchResult
		{
			qint32 EntryID_ = 0;
			qint32 AccountID_ = 0;
			qint64 RowID_ = -1;

			RawSearchResult () = default;
			RawSearchResult (qint32 entryId, qint32 accountId, qint64 rowId);

			bool IsEmpty () const;
		};
	public:
		Storage (QObject* = nullptr);

		struct GeneralError
		{
			QString ErrorText_;
		};

		struct Corruption {};

		using InitializationError_t = boost::variant<Corruption, GeneralError>;
		using InitializationResult_t = Util::Either<InitializationError_t, Util::Void>;

		InitializationResult_t Initialize ();

		QStringList GetOurAccounts () const;
		UsersForAccountResult_t GetUsersForAccount (const QString&);
		ChatLogsResult_t GetChatLogs (const QString& accountId,
				const QString& entryId, int backpages, int amount);

		void AddMessage (const QString& accountId, const QString& entryId,
				const QString& visibleName, const LogItem&);

		SearchResult_t Search (const QString& accountId, const QString& entryId,
				const QString& text, int shift, bool cs);
		SearchResult_t SearchDate (const QString& accountId,
				const QString& entryId, const QDateTime& dt);

		DaysResult_t GetDaysForSheet (const QString& accountId, const QString& entryId, int year, int month);

		void RegenUsersCache ();
		void ClearHistory (const QString& accountId, const QString& entryId);
	private:
		boost::optional<InitializationError_t> CheckDB ();
		void InitializeTables ();
		void UpdateTables ();

		QHash<QString, qint32> GetUsers ();
		qint32 GetUserID (const QString&);
		void AddUser (const QString& id, const QString& accountId);

		void PrepareEntryCache ();

		QHash<QString, qint32> GetAccounts ();
		qint32 GetAccountID (const QString&);
		void AddAccount (const QString& id);
		RawSearchResult SearchImpl (const QString& accountId, const QString& entryId,
				const QString& text, int shift, bool cs);
		RawSearchResult SearchImpl (const QString& accountId, const QString& text, int shift, bool cs);
		RawSearchResult SearchImpl (const QString& text, int shift, bool cs);

		SearchResult_t SearchRowIdImpl (qint32, qint32, qint64);
		SearchResult_t SearchDateImpl (qint32, qint32, const QDateTime&);
	};
}
}
}
