/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2014  Georg Rudoy
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE or copy at https://www.boost.org/LICENSE_1_0.txt)
 **********************************************************************/

#pragma once

#include "streamlistfetcherbase.h"

namespace LC
{
namespace HotStreams
{
	class RockRadioListFetcher : public StreamListFetcherBase
	{
	public:
		RockRadioListFetcher (QStandardItem*, QNetworkAccessManager*, QObject* = 0);
	protected:
		QList<StreamInfo> Parse (const QByteArray&);
	};
}
}
