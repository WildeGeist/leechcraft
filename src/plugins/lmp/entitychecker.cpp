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

#include "entitychecker.h"
#include <QApplication>
#include <QTextCodec>
#include <QFileInfo>
#include <QUrl>
#include <interfaces/structures.h>
#include "phonon.h"
#include "xmlsettingsmanager.h"

namespace LeechCraft
{
namespace LMP
{
	EntityChecker::EntityChecker (const LeechCraft::Entity& e)
	: Result_ (false)
	, Break_ (false)
	{
		struct MimeChecker
		{
			bool operator () (const QString& mime)
			{
				if (mime == "application/ogg")
					return true;
				if (mime.startsWith ("audio/"))
					return true;
				if (mime.startsWith ("video/"))
					return true;
				return false;
			}
		};

		if (e.Entity_.canConvert<QNetworkReply*> () &&
				MimeChecker () (e.Mime_))
		{
			Result_ = true;
			return;
		}
		if (e.Entity_.canConvert<QIODevice*> () &&
				e.Mime_ == "x-leechcraft/media-qiodevice")
		{
			Result_ = true;
			return;
		}
		if (e.Entity_.canConvert<QUrl> ())
		{
			QUrl url = e.Entity_.toUrl ();
			QString extension = QFileInfo (url.path ()).suffix ();

			QStringList goodExt = XmlSettingsManager::Instance ()->
				property ("TestExtensions").toString ()
				.split (' ', QString::SkipEmptyParts);

			Result_ = goodExt.contains (extension);
			return;
		}

		Result_ = false;
	}

	bool EntityChecker::Can () const
	{
		return Result_;
	}
}
}
