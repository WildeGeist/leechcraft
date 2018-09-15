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

#include "account.h"
#include <stdexcept>
#include <QUuid>
#include <QDataStream>
#include <QInputDialog>
#include <QMutex>
#include <QStandardItemModel>
#include <QTimer>
#include <util/xpc/util.h>
#include <util/xpc/passutils.h>
#include <util/sll/slotclosure.h>
#include <util/sll/visitor.h>
#include <util/sll/qtutil.h>
#include <util/sll/prelude.h>
#include <util/threads/monadicfuture.h>
#include <util/xpc/notificationactionhandler.h>
#include <util/gui/sslcertificateinfowidget.h>
#include <interfaces/core/ientitymanager.h>
#include "core.h"
#include "accountconfigdialog.h"
#include "accountthread.h"
#include "accountthreadworker.h"
#include "accountdatabase.h"
#include "storage.h"
#include "accountfoldermanager.h"
#include "mailmodel.h"
#include "foldersmodel.h"
#include "mailmodelsmanager.h"
#include "accountlogger.h"
#include "threadpool.h"
#include "accountthreadnotifier.h"
#include "progresslistener.h"
#include "progressmanager.h"
#include "vmimeconversions.h"

Q_DECLARE_METATYPE (QList<QStringList>)
Q_DECLARE_METATYPE (QList<QByteArray>)

namespace LeechCraft
{
namespace Snails
{
	namespace
	{
		void HandleCertificateException (IEntityManager *iem, const QString& accountName,
				const vmime::security::cert::certificateException& err)
		{
			auto entity = Util::MakeNotification ("Snails",
					Account::tr ("Connection failed for account %1: certificate check failed. %2")
						.arg ("<em>" + accountName + "</em>")
						.arg (QString::fromUtf8 (err.what ())),
					Priority::Critical);

			const auto& qCerts = ToSslCerts (err.getCertificate ());
			qDebug () << Q_FUNC_INFO
					<< qCerts;
			if (qCerts.size () == 1)
			{
				const auto nah = new Util::NotificationActionHandler { entity };
				nah->AddFunction (Account::tr ("View certificate..."),
						[qCerts]
						{
							const auto dia = Util::MakeCertificateViewerDialog (qCerts.at (0));
							dia->setAttribute (Qt::WA_DeleteOnClose);
							dia->show ();
						});
			}

			iem->HandleEntity (entity);
		}
	}

	Account::Account (Storage *st, ProgressManager *pm, QObject *parent)
	: QObject (parent)
	, Logger_ (new AccountLogger (this))
	, WorkerPool_ (new ThreadPool (this, st))
	, AccMutex_ (new QMutex (QMutex::Recursive))
	, ID_ (QUuid::createUuid ().toByteArray ())
	, FolderManager_ (new AccountFolderManager (this))
	, FoldersModel_ (new FoldersModel (this))
	, MailModelsManager_ (new MailModelsManager (this, st))
	, NoopNotifier_ (std::make_shared<AccountThreadNotifier<int>> ())
	, ProgressMgr_ (pm)
	, Storage_ (st)
	{
		connect (FolderManager_,
				SIGNAL (foldersUpdated ()),
				this,
				SLOT (handleFoldersUpdated ()));

		WorkerPool_->AddThreadInitializer ([this] (AccountThread *t)
				{
					t->Schedule (TaskPriority::Low, &AccountThreadWorker::SetNoopTimeoutChangeNotifier, NoopNotifier_);
				});

		UpdateNoopInterval ();

		Util::Sequence (this, WorkerPool_->TestConnectivity ()) >>
				[this] (const auto& result)
				{
					if (const auto left = result.MaybeLeft ())
					{
						const auto iem = Core::Instance ().GetProxy ()->GetEntityManager ();
						const auto emitErr = [=] (QString text)
						{
							if (!text.endsWith ('.'))
								text += '.';

							iem->HandleEntity (Util::MakeNotification ("Snails",
									tr ("Connection failed for account %1: %2")
										.arg ("<em>" + AccName_ + "</em>")
										.arg (text),
									Priority::Critical));
						};

						Util::Visit (*left,
								[&] (const vmime::exceptions::authentication_error& err)
								{
									emitErr (tr ("authentication failed: %1")
											.arg (QString::fromUtf8 (err.what ())));
								},
								[&] (const vmime::security::cert::certificateException& err)
								{
									HandleCertificateException (iem, AccName_, err);
								},
								[] (const auto& e) { qWarning () << Q_FUNC_INFO << e.what (); });
					}
				};
	}

	QByteArray Account::GetID () const
	{
		return ID_;
	}

	QString Account::GetName () const
	{
		QMutexLocker l (GetMutex ());
		return AccName_;
	}

	QString Account::GetServer () const
	{
		return InHost_ + ':' + QString::number (InPort_);
	}

	QString Account::GetUserName () const
	{
		return UserName_;
	}

	QString Account::GetUserEmail () const
	{
		return UserEmail_;
	}

	bool Account::ShouldLogToFile () const
	{
		return LogToFile_;
	}

	AccountLogger* Account::GetLogger () const
	{
		return Logger_;
	}

	AccountDatabase_ptr Account::GetDatabase () const
	{
		return Storage_->BaseForAccount (this);
	}

	AccountFolderManager* Account::GetFolderManager () const
	{
		return FolderManager_;
	}

	MailModelsManager* Account::GetMailModelsManager () const
	{
		return MailModelsManager_;
	}

	FoldersModel* Account::GetFoldersModel () const
	{
		return FoldersModel_;
	}

	QFuture<Account::SynchronizeResult_t> Account::Synchronize ()
	{
		auto folders = FolderManager_->GetSyncFolders ();
		if (folders.isEmpty ())
			folders << QStringList ("INBOX");

		return SynchronizeImpl (folders, {}, TaskPriority::Low);
	}

	QFuture<Account::SynchronizeResult_t> Account::Synchronize (const QStringList& path,
			const QByteArray& last)
	{
		return SynchronizeImpl ({ path }, last, TaskPriority::High);
	}

	QFuture<Account::SynchronizeResult_t> Account::SynchronizeImpl (const QList<QStringList>& folders,
			const QByteArray& last, TaskPriority prio)
	{
		const auto& future = WorkerPool_->Schedule (prio, &AccountThreadWorker::Synchronize, folders, last);
		return future * Util::Visitor
				{
					[=] (const AccountThreadWorker::SyncResult& right)
					{
						HandleGotFolders (right.AllFolders_);

						SyncStats stats;

						for (const auto& pair : Util::Stlize (right.Messages_))
						{
							const auto& folder = pair.first;
							const auto& msgs = pair.second;

							HandleMessagesRemoved (msgs.RemovedIds_, folder);
							HandleGotOtherMessages (msgs.OtherIds_, folder);
							HandleMsgHeaders (msgs.NewHeaders_, folder);
							HandleUpdatedMessages (msgs.UpdatedMsgs_, folder);

							UpdateFolderCount (folder);

							stats.NewMsgsCount_ += msgs.NewHeaders_.size ();
						}

						return SynchronizeResult_t::Right (stats);
					},
					[=] (auto err)
					{
						qWarning () << Q_FUNC_INFO
								<< "error synchronizing"
								<< folders
								<< "to"
								<< last
								<< ":"
								<< Util::Visit (err, [] (auto e) { return e.what (); });
						return SynchronizeResult_t::Left (err);
					}
				};
	}

	Account::FetchWholeMessageResult_t Account::FetchWholeMessage (const Message_ptr& msg)
	{
		auto future = WorkerPool_->Schedule (TaskPriority::High, &AccountThreadWorker::FetchWholeMessage, msg);
		Util::Sequence (this, future) >>
				Util::Visitor
				{
					[this] (const Message_ptr& msg)
					{
						for (const auto& folder : msg->GetFolders ())
							Storage_->SaveMessages (this, folder, { msg });
					},
					Util::Visitor { [] (auto e) { qWarning () << Q_FUNC_INFO << e.what (); } }
				};

		return future;
	}

	QFuture<Account::SendMessageResult_t> Account::SendMessage (const Message_ptr& msg)
	{
		auto pair = msg->GetAddress (Message::Address::From);
		if (pair.first.isEmpty ())
			pair.first = UserName_;
		if (pair.second.isEmpty ())
			pair.second = UserEmail_;
		msg->SetAddress (Message::Address::From, pair);

		return WorkerPool_->Schedule (TaskPriority::High, &AccountThreadWorker::SendMessage, msg);
	}

	auto Account::FetchAttachment (const Message_ptr& msg,
			const QString& attName, const QString& path) -> QFuture<FetchAttachmentResult_t>
	{
		return WorkerPool_->Schedule (TaskPriority::Low, &AccountThreadWorker::FetchAttachment, msg, attName, path);
	}

	void Account::SetReadStatus (bool read, const QList<QByteArray>& ids, const QStringList& folder)
	{
		const auto& future = WorkerPool_->Schedule (TaskPriority::High,
				&AccountThreadWorker::SetReadStatus, read, ids, folder);
		Util::Sequence (this, future) >>
				Util::Visitor
				{
					[=] (const QList<Message_ptr>& msgs)
					{
						HandleUpdatedMessages (msgs, folder);
						UpdateFolderCount (folder);
					},
					[] (auto) {}
				};
	}

	QFuture<Account::CopyMessagesResult_t> Account::CopyMessages (const QList<QByteArray>& ids,
				const QStringList& from, const QList<QStringList>& to)
	{
		return WorkerPool_->Schedule (TaskPriority::High, &AccountThreadWorker::CopyMessages, ids, from, to);
	}

	void Account::MoveMessages (const QList<QByteArray>& ids,
			const QStringList& from, const QList<QStringList>& to)
	{
		Util::Sequence (this, CopyMessages (ids, from, to)) >>
				Util::Visitor
				{
					[=] (Util::Void) { DeleteFromFolder (ids, from); },
					Util::Visitor { [] (auto e) { qWarning () << Q_FUNC_INFO << e.what (); } }
				};
	}

	namespace
	{
		Account::DeleteBehaviour RollupBehaviour (Account::DeleteBehaviour behaviour, const QString& service)
		{
			if (behaviour != Account::DeleteBehaviour::Default)
				return behaviour;

			static const QStringList knownTrashes { "imap.gmail.com" };
			return knownTrashes.contains (service) ?
					Account::DeleteBehaviour::MoveToTrash :
					Account::DeleteBehaviour::Expunge;
		}
	}

	void Account::DeleteMessages (const QList<QByteArray>& ids, const QStringList& folder)
	{
		const auto& trashPath = FoldersModel_->GetFolderPath (FolderType::Trash);
		if (trashPath &&
				RollupBehaviour (DeleteBehaviour_, InHost_) == DeleteBehaviour::MoveToTrash)
			Util::Sequence (this, CopyMessages (ids, folder, { *trashPath })) >>
					Util::Visitor
					{
						[=] (Util::Void) { DeleteFromFolder (ids, folder); },
						Util::Visitor { [] (auto e) { qWarning () << Q_FUNC_INFO << e.what (); } }
					};
		else
			DeleteFromFolder (ids, folder);
	}

	QByteArray Account::Serialize () const
	{
		QMutexLocker l (GetMutex ());

		QByteArray result;

		QDataStream out { &result, QIODevice::WriteOnly };
		out << static_cast<quint8> (4);
		out << ID_
			<< AccName_
			<< Login_
			<< UseSASL_
			<< SASLRequired_
			<< UseTLS_
			<< UseSSL_
			<< InSecurityRequired_
			<< static_cast<qint8> (OutSecurity_)
			<< OutSecurityRequired_
			<< SMTPNeedsAuth_
			<< InHost_
			<< InPort_
			<< OutHost_
			<< OutPort_
			<< OutLogin_
			<< static_cast<quint8> (OutType_)
			<< UserName_
			<< UserEmail_
			<< FolderManager_->Serialize ()
			<< KeepAliveInterval_
			<< LogToFile_
			<< static_cast<quint8> (DeleteBehaviour_);

		return result;
	}

	void Account::Deserialize (const QByteArray& arr)
	{
		QDataStream in (arr);
		quint8 version = 0;
		in >> version;

		if (version < 1 || version > 4)
			throw std::runtime_error { "Unknown version " + std::to_string (version) };

		quint8 outType = 0;
		qint8 type = 0;

		{
			QMutexLocker l (GetMutex ());
			in >> ID_
				>> AccName_
				>> Login_
				>> UseSASL_
				>> SASLRequired_
				>> UseTLS_
				>> UseSSL_
				>> InSecurityRequired_
				>> type
				>> OutSecurityRequired_;

			OutSecurity_ = static_cast<SecurityType> (type);

			in >> SMTPNeedsAuth_
				>> InHost_
				>> InPort_
				>> OutHost_
				>> OutPort_
				>> OutLogin_
				>> outType;

			OutType_ = static_cast<OutType> (outType);

			in >> UserName_
				>> UserEmail_;

			QByteArray fstate;
			in >> fstate;
			FolderManager_->Deserialize (fstate);

			handleFoldersUpdated ();

			if (version >= 2)
				in >> KeepAliveInterval_;

			if (version >= 3)
				in >> LogToFile_;

			if (version >= 4)
			{
				quint8 deleteBehaviour = 0;
				in >> deleteBehaviour;
				DeleteBehaviour_ = static_cast<DeleteBehaviour> (deleteBehaviour);
			}
		}
	}

	void Account::OpenConfigDialog (const std::function<void ()>& onAccepted)
	{
		auto dia = new AccountConfigDialog;
		dia->setAttribute (Qt::WA_DeleteOnClose);

		{
			QMutexLocker l (GetMutex ());
			dia->SetName (AccName_);
			dia->SetUserName (UserName_);
			dia->SetUserEmail (UserEmail_);
			dia->SetLogin (Login_);
			dia->SetUseSASL (UseSASL_);
			dia->SetSASLRequired (SASLRequired_);

			if (UseSSL_)
				dia->SetInSecurity (SecurityType::SSL);
			else if (UseTLS_)
				dia->SetInSecurity (SecurityType::TLS);
			else
				dia->SetInSecurity (SecurityType::No);

			dia->SetInSecurityRequired (InSecurityRequired_);

			dia->SetOutSecurity (OutSecurity_);
			dia->SetOutSecurityRequired (OutSecurityRequired_);

			dia->SetSMTPAuth (SMTPNeedsAuth_);
			dia->SetInHost (InHost_);
			dia->SetInPort (InPort_);
			dia->SetOutHost (OutHost_);
			dia->SetOutPort (OutPort_);
			dia->SetOutLogin (OutLogin_);
			dia->SetOutType (OutType_);

			dia->SetKeepAliveInterval (KeepAliveInterval_);
			dia->SetLogConnectionsToFile (LogToFile_);

			const auto& folders = FolderManager_->GetFoldersPaths ();
			dia->SetAllFolders (folders);
			const auto& toSync = FolderManager_->GetSyncFolders ();
			for (const auto& folder : folders)
			{
				const auto flags = FolderManager_->GetFolderFlags (folder);
				if (flags & AccountFolderManager::FolderOutgoing)
					dia->SetOutFolder (folder);
			}
			dia->SetFoldersToSync (toSync);

			dia->SetDeleteBehaviour (DeleteBehaviour_);
		}

		dia->show ();

		new Util::SlotClosure<Util::DeleteLaterPolicy>
		{
			[this, onAccepted, dia]
			{
				QMutexLocker l (GetMutex ());
				AccName_ = dia->GetName ();
				UserName_ = dia->GetUserName ();
				UserEmail_ = dia->GetUserEmail ();
				Login_ = dia->GetLogin ();
				UseSASL_ = dia->GetUseSASL ();
				SASLRequired_ = dia->GetSASLRequired ();

				UseSSL_ = false;
				UseTLS_ = false;
				switch (dia->GetInSecurity ())
				{
				case SecurityType::SSL:
					UseSSL_ = true;
					break;
				case SecurityType::TLS:
					UseTLS_ = true;
					break;
				case SecurityType::No:
					break;
				}

				InSecurityRequired_ = dia->GetInSecurityRequired ();

				OutSecurity_ = dia->GetOutSecurity ();
				OutSecurityRequired_ = dia->GetOutSecurityRequired ();

				SMTPNeedsAuth_ = dia->GetSMTPAuth ();
				InHost_ = dia->GetInHost ();
				InPort_ = dia->GetInPort ();
				OutHost_ = dia->GetOutHost ();
				OutPort_ = dia->GetOutPort ();
				OutLogin_ = dia->GetOutLogin ();
				OutType_ = dia->GetOutType ();

				if (KeepAliveInterval_ != dia->GetKeepAliveInterval ())
				{
					KeepAliveInterval_ = dia->GetKeepAliveInterval ();
					UpdateNoopInterval ();
				}

				LogToFile_ = dia->GetLogConnectionsToFile ();

				FolderManager_->ClearFolderFlags ();
				const auto& out = dia->GetOutFolder ();
				if (!out.isEmpty ())
					FolderManager_->AppendFolderFlags (out, AccountFolderManager::FolderOutgoing);

				for (const auto& sync : dia->GetFoldersToSync ())
					FolderManager_->AppendFolderFlags (sync, AccountFolderManager::FolderSyncable);

				DeleteBehaviour_ = dia->GetDeleteBehaviour ();

				emit accountChanged ();

				if (onAccepted)
					onAccepted ();
			},
			dia,
			SIGNAL (accepted ()),
			dia
		};
	}

	bool Account::IsNull () const
	{
		return AccName_.isEmpty () ||
			Login_.isEmpty ();
	}

	QString Account::GetInUsername () const
	{
		return Login_;
	}

	QString Account::GetOutUsername () const
	{
		return OutLogin_;
	}

	ProgressListener_ptr Account::MakeProgressListener (const QString& context) const
	{
		return ProgressMgr_->MakeProgressListener (context);
	}

	QMutex* Account::GetMutex () const
	{
		return AccMutex_;
	}

	void Account::UpdateNoopInterval ()
	{
		NoopNotifier_->SetData (KeepAliveInterval_);
	}

	QFuture<QString> Account::BuildInURL ()
	{
		using Util::operator*;
		return GetPassword (Direction::In) *
				[this] (const QString& pass)
				{
					QMutexLocker l (GetMutex ());

					QString result { UseSSL_ ? "imaps://" : "imap://" };
					result += Login_;
					result += ":";
					result.replace ('@', "%40");
					result += pass + '@';
					result += InHost_;

					return result;
				};
	}

	QFuture<QString> Account::BuildOutURL ()
	{
		using Util::operator*;
		QMutexLocker l (GetMutex ());

		if (OutType_ == OutType::Sendmail)
			return Util::MakeReadyFuture (QString { "sendmail://localhost" });

		QString result { OutSecurity_ == SecurityType::SSL ? "smtps://" : "smtp://" };

		if (!SMTPNeedsAuth_)
			return Util::MakeReadyFuture (result + OutHost_);

		QFuture<QString> passFuture;
		if (OutLogin_.isEmpty ())
		{
			result += Login_;
			passFuture = GetPassword (Direction::In);
		}
		else
		{
			result += OutLogin_;
			passFuture = GetPassword (Direction::Out);
		}
		auto outHost = OutHost_;

		return passFuture *
				[result, outHost] (const QString& pass)
				{
					auto full = result + ":" + pass;

					full.replace ('@', "%40");
					full += '@';

					full += outHost;

					qDebug () << Q_FUNC_INFO << full;

					return full;
				};
	}

	QByteArray Account::GetStoreID (Account::Direction dir) const
	{
		QMutexLocker l (GetMutex ());

		QByteArray result = GetID ();
		if (dir == Direction::Out)
			result += "/out";
		return result;
	}

	void Account::DeleteFromFolder (const QList<QByteArray>& ids, const QStringList& folder)
	{
		const auto& future = WorkerPool_->Schedule (TaskPriority::High,
				&AccountThreadWorker::DeleteMessages, ids, folder);
		Util::Sequence (this, future) >>
				Util::Visitor
				{
					[=] (Util::Void)
					{
						HandleMessagesRemoved (ids, folder);
						UpdateFolderCount (folder);
					},
					[] (auto) {}
				};
	}

	void Account::UpdateFolderCount (const QStringList& folder)
	{
		const auto totalCount = Storage_->GetNumMessages (this, folder);
		const auto unreadCount = Storage_->GetNumUnread (this, folder);
		FoldersModel_->SetFolderCounts (folder, unreadCount, totalCount);
	}

	QFuture<QString> Account::GetPassword (Direction dir)
	{
		QFutureInterface<QString> promise;
		promise.reportStarted ();
		QTimer::singleShot (0, this,
				[this, dir, promise] () mutable
				{
					Util::ReportFutureResult (promise,
							Util::GetPassword (GetStoreID (dir),
									tr ("Enter password for account %1:")
										.arg (GetName ()),
									Core::Instance ().GetProxy ()));
				});
		return promise.future ();
	}

	void Account::HandleMsgHeaders (const QList<MessageWHeaders_t>& messages, const QStringList& folder)
	{
		qDebug () << Q_FUNC_INFO << messages.size ();
		const auto& justMessages = Util::Map (messages, Util::Fst);
		Storage_->SaveMessages (this, folder, justMessages);

		const auto base = Storage_->BaseForAccount (this);
		for (const auto& [msg, header] : messages)
			base->SetMessageHeader (msg->GetMessageID (), SerializeHeader (header));

		MailModelsManager_->Append (justMessages);
	}

	void Account::HandleUpdatedMessages (const QList<Message_ptr>& messages, const QStringList& folder)
	{
		qDebug () << Q_FUNC_INFO << messages.size ();
		Storage_->SaveMessages (this, folder, messages);

		MailModelsManager_->Update (messages);
	}

	void Account::HandleGotOtherMessages (const QList<QByteArray>& ids, const QStringList& folder)
	{
		qDebug () << Q_FUNC_INFO << ids.size () << folder;
		if (ids.isEmpty ())
			return;

		const auto& msgs = Storage_->LoadMessages (this, folder, ids);

		MailModelsManager_->Append (msgs);
	}

	void Account::HandleMessagesRemoved (const QList<QByteArray>& ids, const QStringList& folder)
	{
		qDebug () << Q_FUNC_INFO << ids.size () << folder;
		for (const auto& id : ids)
			Storage_->RemoveMessage (this, folder, id);

		MailModelsManager_->Remove (ids);
	}

	void Account::RequestMessageCount (const QStringList& folder)
	{
		auto future = WorkerPool_->Schedule (TaskPriority::Low, &AccountThreadWorker::GetMessageCount, folder);
		Util::Sequence (this, future) >>
				Util::Visitor
				{
					[=] (QPair<int, int> counts) { HandleMessageCountFetched (counts.first, counts.second, folder); },
					Util::Visitor { [] (auto e) { qWarning () << Q_FUNC_INFO << e.what (); } }
				};
	}

	void Account::HandleMessageCountFetched (int count, int unread, const QStringList& folder)
	{
		const auto storedCount = Storage_->GetNumMessages (this, folder);
		if (storedCount && count != storedCount)
		{
			qDebug () << Q_FUNC_INFO
					<< "outdated local stored count"
					<< storedCount
					<< "vs"
					<< count;
			Synchronize (folder, {});
		}

		FoldersModel_->SetFolderCounts (folder, unread, count);
	}

	void Account::HandleGotFolders (const QList<Folder>& folders)
	{
		FolderManager_->SetFolders (folders);
	}

	void Account::handleFoldersUpdated ()
	{
		const auto& folders = FolderManager_->GetFolders ();
		FoldersModel_->SetFolders (folders);

		for (const auto& folder : folders)
		{
			int count = -1, unread = -1;
			try
			{
				count = Storage_->GetNumMessages (this, folder.Path_);
				unread = Storage_->GetNumUnread (this, folder.Path_);
			}
			catch (const std::exception&)
			{
			}

			FoldersModel_->SetFolderCounts (folder.Path_, unread, count);

			RequestMessageCount (folder.Path_);
		}
	}
}
}
