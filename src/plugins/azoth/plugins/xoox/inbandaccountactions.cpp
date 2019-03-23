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

#include "inbandaccountactions.h"
#include <util/sll/slotclosure.h>
#include <interfaces/azoth/iproxyobject.h>
#include "accountsettingsholder.h"
#include "clientconnection.h"
#include "glooxaccount.h"
#include "util.h"
#include "xmppelementdescription.h"

namespace LeechCraft::Azoth::Xoox
{
	InBandAccountActions::InBandAccountActions (ClientConnection& conn, GlooxAccount& acc)
	: Conn_ { conn }
	, Acc_ { acc }
	{
	}

	void InBandAccountActions::UpdateServerPassword (const QString& newPass)
	{
		if (newPass.isEmpty ())
			return;

		const auto& jid = Acc_.GetSettings ()->GetJID ();
		const auto aPos = jid.indexOf ('@');

		XmppElementDescription queryDescr
		{
			.TagName_ = "query",
			.Attributes_ = { { "xmlns", XooxUtil::NsRegister } },
			.Children_ =
			{
				{ .TagName_ = "username", .Value_ = aPos > 0 ? jid.left (aPos) : jid },
				{ .TagName_ = "password", .Value_ = newPass },
			}
		};

		QXmppIq iq (QXmppIq::Set);
		iq.setTo (Acc_.GetDefaultReqHost ());
		iq.setExtensions ({ ToElement (queryDescr) });

		Conn_.SendPacketWCallback (iq,
				[this, newPass] (const QXmppIq& reply)
				{
					if (reply.type () != QXmppIq::Result)
						return;

					emit Acc_.serverPasswordUpdated (newPass);
					Acc_.GetParentProtocol ()->GetProxyObject ()->SetPassword (newPass, &Acc_);
					Conn_.SetPassword (newPass);
				});
	}

	namespace
	{
		QXmppIq MakeDeregisterIq ()
		{
			XmppElementDescription queryDescr
			{
				.TagName_ = "query",
				.Attributes_ = { { "xmlns", XooxUtil::NsRegister } },
				.Children_ = { { .TagName_ = "remove" } }
			};

			QXmppIq iq { QXmppIq::Set };
			iq.setExtensions ({ ToElement (queryDescr) });
			return iq;
		}
	}

	void InBandAccountActions::CancelRegistration ()
	{
		const auto worker = [this]
		{
			Conn_.SendPacketWCallback (MakeDeregisterIq (),
					[this] (const QXmppIq& reply)
					{
						if (reply.type () == QXmppIq::Result)
						{
							Acc_.GetParentProtocol ()->RemoveAccount (&Acc_);
							Acc_.ChangeState ({ SOffline, {} });
						}
						else
							qWarning () << Q_FUNC_INFO
									<< "unable to cancel the registration:"
									<< reply.type ();
					});
		};

		if (Acc_.GetState ().State_ != SOffline)
		{
			worker ();
			return;
		}

		Acc_.ChangeState ({ SOnline, {} });
		new Util::SlotClosure<Util::ChoiceDeletePolicy>
		{
			[this, worker]
			{
				switch (Acc_.GetState ().State_)
				{
				case SOffline:
				case SError:
				case SConnecting:
					return Util::ChoiceDeletePolicy::Delete::No;
				default:
					break;
				}

				worker ();

				return Util::ChoiceDeletePolicy::Delete::Yes;
			},
			&Acc_,
			SIGNAL (statusChanged (EntryStatus)),
			&Acc_
		};
	}
}
