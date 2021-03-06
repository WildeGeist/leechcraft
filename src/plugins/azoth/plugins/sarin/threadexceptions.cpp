/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2014  Georg Rudoy
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE or copy at https://www.boost.org/LICENSE_1_0.txt)
 **********************************************************************/

#include "threadexceptions.h"

namespace LC
{
namespace Azoth
{
namespace Sarin
{
	TextExceptionBase::TextExceptionBase (const QString& str)
	: Msg_ { str.toUtf8 () }
	{
	}

	const char* TextExceptionBase::what () const noexcept
	{
		return Msg_.constData ();
	}
}
}
}
