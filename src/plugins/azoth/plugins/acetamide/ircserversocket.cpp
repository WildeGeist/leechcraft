/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2014  Georg Rudoy
 * Copyright (C) 2010-2012  Oleg Linkin
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE or copy at https://www.boost.org/LICENSE_1_0.txt)
 **********************************************************************/

#include "ircserversocket.h"
#include <QTcpSocket>
#include <QTextCodec>
#include <interfaces/core/icoreproxy.h>
#include <interfaces/core/ientitymanager.h>
#include <util/xpc/util.h>
#include <util/sll/visitor.h>
#include <util/sll/functional.h>
#include "ircserverhandler.h"
#include "clientconnection.h"

namespace LC
{
namespace Azoth
{
namespace Acetamide
{
	int elideWidth = 300;

	IrcServerSocket::IrcServerSocket (IrcServerHandler *ish)
	: QObject (ish)
	, ISH_ (ish)
	{
		if (ish->GetServerOptions ().SSL_)
			Socket_ = std::make_shared<QSslSocket> ();
		else
			Socket_ = std::make_shared<QTcpSocket> ();

		Init ();
	}

	IrcServerSocket::~IrcServerSocket ()
	{
		if (const auto socket = GetSocketPtr ())
		{
			QObject::disconnect (socket, 0, 0, 0);
			socket->abort ();
		}
	}

	void IrcServerSocket::ConnectToHost (const QString& host, int port)
	{
		Util::Visit (Socket_,
				[&] (const Tcp_ptr& ptr) { ptr->connectToHost (host, port); },
				[&] (const Ssl_ptr& ptr) { ptr->connectToHostEncrypted (host, port); });
	}

	void IrcServerSocket::DisconnectFromHost ()
	{
		if (const auto socket = GetSocketPtr ())
		{
			socket->disconnectFromHost ();
		}
	}

	void IrcServerSocket::Send (const QString& message)
	{
		const auto socket = GetSocketPtr ();
		if (!socket->isWritable ())
		{
			qWarning () << Q_FUNC_INFO
					<< socket->error ()
					<< socket->errorString ();
			return;
		}

		RefreshCodec ();

		if (socket->write (LastCodec_->fromUnicode (message)) == -1)
			qWarning () << Q_FUNC_INFO
					<< socket->error ()
					<< socket->errorString ();
	}

	void IrcServerSocket::Close ()
	{
		GetSocketPtr ()->close ();
	}

	void IrcServerSocket::Init ()
	{
		const auto socket = GetSocketPtr ();
		connect (socket,
				SIGNAL (readyRead ()),
				this,
				SLOT (readReply ()));

		connect (socket,
				SIGNAL (connected ()),
				ISH_,
				SLOT (connectionEstablished ()));

		connect (socket,
				SIGNAL (disconnected ()),
				ISH_,
				SLOT (connectionClosed ()));

		connect (socket,
				SIGNAL (error (QAbstractSocket::SocketError)),
				ISH_,
				SLOT (handleSocketError (QAbstractSocket::SocketError)));

		Util::Visit (Socket_,
				[this] (const Ssl_ptr& ptr)
				{
					connect (ptr.get (),
							SIGNAL (sslErrors (const QList<QSslError> &)),
							this,
							SLOT (handleSslErrors (const QList<QSslError>&)));
				},
				[] (auto) {});
	}

	void IrcServerSocket::RefreshCodec ()
	{
		const auto encoding = ISH_->GetServerOptions ().ServerEncoding_;
		if (LastCodec_ && LastCodec_->name () == encoding)
			return;

		const auto newCodec = encoding == "System" ?
				QTextCodec::codecForLocale () :
				QTextCodec::codecForName (encoding.toLatin1 ());
		if (newCodec)
		{
			LastCodec_ = newCodec;
			return;
		}

		qWarning () << Q_FUNC_INFO
				<< "unable to create codec for encoding `"
				<< encoding.toUtf8 ()
				<< "`; known codecs:"
				<< QTextCodec::availableCodecs ();

		const auto& notify = Util::MakeNotification ("Azoth Acetamide",
				tr ("Unknown encoding %1.")
					.arg ("<em>" + encoding + "</em>"),
				Priority::Critical);
		Core::Instance ().GetProxy ()->GetEntityManager ()->HandleEntity (notify);

		if (LastCodec_)
			return;

		qWarning () << Q_FUNC_INFO
				<< "no codec is set, will fall back to locale-default codec";

		LastCodec_ = QTextCodec::codecForLocale ();
	}

	QTcpSocket* IrcServerSocket::GetSocketPtr () const
	{
		return Util::Visit (Socket_,
				[] (const auto& ptr) { return Util::Upcast<QTcpSocket*> (ptr.get ()); });
	}

	void IrcServerSocket::readReply ()
	{
		const auto socket = GetSocketPtr ();
		while (socket->canReadLine ())
			ISH_->ReadReply (socket->readLine ());
	}

	namespace
	{
		class SslErrorsReaction final : public ICanHaveSslErrors::ISslErrorsReaction
		{
			std::weak_ptr<QSslSocket> Sock_;
		public:
			SslErrorsReaction (const std::shared_ptr<QSslSocket>& sock)
			: Sock_ { sock }
			{
			}

			void Ignore () override
			{
				if (const auto sock = Sock_.lock ())
					sock->ignoreSslErrors ();
			}

			void Abort () override
			{
			}
		};
	}

	void IrcServerSocket::HandleSslErrors (const std::shared_ptr<QSslSocket>& s, const QList<QSslError>& errors)
	{
		emit sslErrors (errors, std::make_shared<SslErrorsReaction> (s));
	}

	void IrcServerSocket::handleSslErrors (const QList<QSslError>& errors)
	{
		Util::Visit (Socket_,
				[&] (const Ssl_ptr& s) { HandleSslErrors (s, errors); },
				[] (auto) { qWarning () << Q_FUNC_INFO << "expected SSL socket"; });
	}
}
}
}
