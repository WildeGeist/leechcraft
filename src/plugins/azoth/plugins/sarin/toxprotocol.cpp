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

#include "toxprotocol.h"
#include <QIcon>
#include <QSettings>
#include <QCoreApplication>
#include <QtDebug>
#include "accregisterdetailspage.h"
#include "toxaccount.h"

namespace LeechCraft
{
namespace Azoth
{
namespace Sarin
{
	ToxProtocol::ToxProtocol (const ICoreProxy_ptr& proxy, QObject* parentPlugin)
	: QObject { parentPlugin }
	, CoreProxy_ { proxy }
	, ParentProtocol_ { parentPlugin }
	{
		LoadAccounts ();
	}

	QObject* ToxProtocol::GetQObject ()
	{
		return this;
	}

	IProtocol::ProtocolFeatures ToxProtocol::GetFeatures () const
	{
		return PFNone;
	}

	QList<QObject*> ToxProtocol::GetRegisteredAccounts ()
	{
		QList<QObject*> result;
		for (auto acc : Accounts_)
			result << acc;
		return result;
	}

	QObject* ToxProtocol::GetParentProtocolPlugin () const
	{
		return ParentProtocol_;
	}

	QString ToxProtocol::GetProtocolName () const
	{
		return "Tox";
	}

	QIcon ToxProtocol::GetProtocolIcon () const
	{
		return {};
	}

	QByteArray ToxProtocol::GetProtocolID () const
	{
		return "org.LeechCraft.Azoth.Tox";
	}

	QList<QWidget*> ToxProtocol::GetAccountRegistrationWidgets (IProtocol::AccountAddOptions)
	{
		return { new AccRegisterDetailsPage };
	}

	void ToxProtocol::RegisterAccount (const QString& name, const QList<QWidget*>& widgets)
	{
		const auto detailsPage = qobject_cast<AccRegisterDetailsPage*> (widgets.value (0));

		auto acc = new ToxAccount { name, this };
		acc->SetNickname (detailsPage->GetNickname ());
		saveAccount (acc);

		Accounts_ << acc;
		emit accountAdded (acc);

		connect (acc,
				SIGNAL (accountChanged (ToxAccount*)),
				this,
				SLOT (saveAccount (ToxAccount*)));
	}

	QWidget* ToxProtocol::GetMUCJoinWidget ()
	{
		return nullptr;
	}

	void ToxProtocol::RemoveAccount (QObject *accObj)
	{
		const auto account = qobject_cast<ToxAccount*> (accObj);
		if (!Accounts_.contains (account))
			return;

		QSettings settings { QSettings::IniFormat, QSettings::UserScope,
				QCoreApplication::organizationName (),
				QCoreApplication::applicationName () + "_Azoth_Saren_Accounts" };
		settings.remove (account->GetAccountID ());

		Accounts_.removeOne (account);
		emit accountRemoved (accObj);
	}

	const ICoreProxy_ptr& ToxProtocol::GetCoreProxy () const
	{
		return CoreProxy_;
	}

	void ToxProtocol::LoadAccounts ()
	{
		QSettings settings { QSettings::IniFormat, QSettings::UserScope,
				QCoreApplication::organizationName (),
				QCoreApplication::applicationName () + "_Azoth_Saren_Accounts" };

		for (const auto& key : settings.childKeys ())
		{
			const auto& serialized = settings.value (key).toByteArray ();
			if (const auto acc = ToxAccount::Deserialize (serialized, this))
				Accounts_ << acc;
			else
				qWarning () << Q_FUNC_INFO
						<< "cannot deserialize"
						<< key;
		}
	}

	void ToxProtocol::saveAccount (ToxAccount *account)
	{
		QSettings settings { QSettings::IniFormat, QSettings::UserScope,
				QCoreApplication::organizationName (),
				QCoreApplication::applicationName () + "_Azoth_Saren_Accounts" };
		settings.setValue (account->GetAccountID (), account->Serialize ());
	}
}
}
}
