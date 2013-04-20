/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2013  Georg Rudoy
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

#include "cloudwidget.h"
#include <algorithm>
#include <iterator>
#include <interfaces/lmp/icloudstorageplugin.h>
#include "sync/uploadmodel.h"
#include "sync/clouduploadmanager.h"
#include "sync/transcodingparams.h"
#include "core.h"
#include "localcollection.h"

namespace LeechCraft
{
namespace LMP
{
	CloudWidget::CloudWidget (QWidget *parent)
	: QWidget (parent)
	, DevUploadModel_ (new UploadModel (this))
	{
		Ui_.setupUi (this);
		Ui_.TranscodingOpts_->SetMaskVisible (false);

		DevUploadModel_->setSourceModel (Core::Instance ().GetLocalCollection ()->GetCollectionModel ());
		Ui_.OurCollection_->setModel (DevUploadModel_);

		Ui_.SyncTabs_->setEnabled (false);

		connect (&Core::Instance (),
				SIGNAL (cloudStoragePluginsChanged ()),
				this,
				SLOT (handleCloudStoragePlugins ()));
		handleCloudStoragePlugins ();

		Ui_.TSProgress_->hide ();
		Ui_.UploadProgress_->hide ();

		connect (Core::Instance ().GetCloudUploadManager (),
				SIGNAL (uploadLog (QString)),
				this,
				SLOT (appendUpLog (QString)));

		connect (Core::Instance ().GetCloudUploadManager (),
				SIGNAL (transcodingProgress (int, int)),
				this,
				SLOT (handleTranscodingProgress (int, int)));
		connect (Core::Instance ().GetCloudUploadManager (),
				SIGNAL (uploadProgress (int, int)),
				this,
				SLOT (handleUploadProgress (int, int)));
	}

	void CloudWidget::on_CloudSelector__activated (int idx)
	{
		Ui_.AccountSelector_->clear ();
		Ui_.SyncTabs_->setEnabled (false);
		if (idx < 0)
			return;

		auto cloud = qobject_cast<ICloudStoragePlugin*> (Clouds_.at (idx));
		const auto& accounts = cloud->GetAccounts ();
		if (accounts.isEmpty ())
			return;

		Ui_.AccountSelector_->addItems (accounts);
		Ui_.SyncTabs_->setEnabled (true);
	}

	void CloudWidget::handleCloudStoragePlugins ()
	{
		Ui_.CloudSelector_->clear ();

		Clouds_ = Core::Instance ().GetCloudStoragePlugins ();
		Q_FOREACH (QObject *cloudObj, Clouds_)
		{
			auto cloud = qobject_cast<ICloudStoragePlugin*> (cloudObj);
			Ui_.CloudSelector_->addItem (cloud->GetCloudIcon (), cloud->GetCloudName ());

			connect (cloudObj,
					SIGNAL (accountsChanged ()),
					this,
					SLOT (handleAccountsChanged ()),
					Qt::UniqueConnection);
		}

		if (!Clouds_.isEmpty ())
			on_CloudSelector__activated (0);
	}

	void CloudWidget::handleAccountsChanged ()
	{
		const int idx = Ui_.CloudSelector_->currentIndex ();
		if (idx < 0 || sender () != Clouds_.at (idx))
			return;

		on_CloudSelector__activated (idx);
	}

	void CloudWidget::on_UploadButton__released ()
	{
		const int idx = Ui_.CloudSelector_->currentIndex ();
		const auto& accName = Ui_.AccountSelector_->currentText ();
		if (idx < 0 || accName.isEmpty ())
			return;

		const auto& selected = DevUploadModel_->GetSelectedIndexes ();
		QStringList paths;
		std::transform (selected.begin (), selected.end (), std::back_inserter (paths),
				[] (const QModelIndex& idx) { return idx.data (LocalCollection::Role::TrackPath).toString (); });
		paths.removeAll (QString ());

		Ui_.UploadLog_->clear ();

		auto cloud = qobject_cast<ICloudStoragePlugin*> (Clouds_.at (idx));
		Core::Instance ().GetCloudUploadManager ()->AddFiles (cloud,
				accName, paths, Ui_.TranscodingOpts_->GetParams ());
	}

	void CloudWidget::appendUpLog (QString text)
	{
		text.prepend (QTime::currentTime ().toString ("[HH:mm:ss.zzz] "));
		Ui_.UploadLog_->append ("<code>" + text + "</code>");
	}

	void CloudWidget::handleTranscodingProgress (int done, int total)
	{
		Ui_.TSProgress_->setVisible (done < total);
		Ui_.TSProgress_->setMaximum (total);
		Ui_.TSProgress_->setValue (done);
	}

	void CloudWidget::handleUploadProgress (int done, int total)
	{
		Ui_.UploadProgress_->setVisible (done < total);
		Ui_.UploadProgress_->setMaximum (total);
		Ui_.UploadProgress_->setValue (done);
	}
}
}
