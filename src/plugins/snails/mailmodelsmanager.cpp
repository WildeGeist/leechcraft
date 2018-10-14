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

#include "mailmodelsmanager.h"
#include "account.h"
#include "mailmodel.h"
#include "core.h"
#include "storage.h"
#include "messagelistactionsmanager.h"

namespace LeechCraft
{
namespace Snails
{
	MailModelsManager::MailModelsManager (Account *acc, Storage *st)
	: QObject { acc }
	, Acc_ { acc }
	, Storage_ { st }
	, MsgListActionsMgr_ { new MessageListActionsManager { Acc_, this } }
	{
	}

	std::unique_ptr<MailModel> MailModelsManager::CreateModel ()
	{
		auto model = std::make_unique<MailModel> (MsgListActionsMgr_, Acc_);
		Models_ << model.get ();

		connect (model.get (),
				&QObject::destroyed,
				this,
				[this, obj = model.get ()] { Models_.removeAll (obj); });

		return model;
	}

	void MailModelsManager::ShowFolder (const QStringList& path, MailModel *mailModel)
	{
		if (!Models_.contains (mailModel))
		{
			qWarning () << Q_FUNC_INFO
					<< "unmanaged model"
					<< mailModel
					<< Models_;
			return;
		}

		mailModel->Clear ();

		qDebug () << Q_FUNC_INFO << path;
		if (path.isEmpty ())
			return;

		mailModel->SetFolder (path);

		const auto& ids = Storage_->LoadIDs (Acc_, path);

		try
		{
			mailModel->Append (Storage_->LoadMessages (Acc_, path, ids));
		}
		catch (const std::exception& e)
		{
			qWarning () << Q_FUNC_INFO
					<< e.what ();
		}

		Acc_->Synchronize (path);
	}

	void MailModelsManager::Append (const QList<Message_ptr>& messages)
	{
		for (const auto model : Models_)
			model->Append (messages);
	}

	void MailModelsManager::Remove (const QList<QByteArray>& ids)
	{
		for (const auto model : Models_)
			for (const auto& id : ids)
				model->Remove (id);
	}

	void MailModelsManager::UpdateReadStatus (const QStringList& folderId, const QList<QByteArray>& msgIds, bool read)
	{
		for (const auto model : Models_)
			if (model->GetCurrentFolder () == folderId)
				model->UpdateReadStatus (msgIds, read);
	}
}
}
