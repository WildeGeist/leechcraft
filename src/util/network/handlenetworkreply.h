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

#include <QNetworkReply>
#include <util/sll/slotclosure.h>
#include <util/sll/either.h>
#include <util/sll/overload.h>
#include <util/sll/void.h>
#include <util/threads/futures.h>

namespace LeechCraft
{
namespace Util
{
	template<typename F>
	void HandleNetworkReply (QObject *context, QNetworkReply *reply, F f)
	{
		new Util::SlotClosure<Util::DeleteLaterPolicy>
		{
			[reply, f]
			{
				reply->deleteLater ();
				f (reply->readAll ());
			},
			reply,
			SIGNAL (finished ()),
			context
		};
	}

	template<typename Err = Util::Void>
	auto HandleReply (QNetworkReply *reply, QObject *context)
	{
		using Result_t = Util::Either<Err, QByteArray>;
		QFutureInterface<Result_t> promise;
		promise.reportStarted ();

		QObject::connect (reply,
				&QNetworkReply::finished,
				context,
				[promise, reply] () mutable
				{
					reply->deleteLater ();
					Util::ReportFutureResult (promise, Result_t::Right (reply->readAll ()));
				});
		QObject::connect (reply,
				Util::Overload<QNetworkReply::NetworkError> (&QNetworkReply::error),
				context,
				[promise, reply] () mutable
				{
					reply->deleteLater ();
					auto report = [&] (const Err& val) { Util::ReportFutureResult (promise, Result_t::Left (val)); };

					if constexpr (std::is_same_v<Err, QString>)
						report (reply->errorString ());
					else if constexpr (std::is_same_v<Err, Util::Void>)
						report ({});
					else
						static_assert (std::is_same_v<Err, struct Dummy>, "Unsupported error type");
				});

		return promise.future ();
	}

	template<typename Err = Util::Void>
	auto HandleReplySeq (QNetworkReply *reply, QObject *context)
	{
		return Sequence (context, HandleReply<Err> (reply, context));
	}
}
}
