/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2013  Slava Barinov <rayslava@gmail.com>
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

#include "pasteorgruservice.h"
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QtDebug>
#include <QRegExp>

namespace LC::Azoth::Autopaste
{
	void PasteOrgRuService::Paste (const PasteParams& params)
	{
		const QByteArray& data = "type=1&code=" + params.Text_.toUtf8 ().toPercentEncoding ();

		QNetworkRequest req (QString ("http://paste.org.ru/?"));
		req.setHeader (QNetworkRequest::ContentLengthHeader, data.size ());
		req.setHeader (QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

		InitReply (params.NAM_->post (req, data));
	}

	void PasteOrgRuService::HandleMetadata (QNetworkReply *reply)
	{
		const auto code = reply->attribute (QNetworkRequest::HttpStatusCodeAttribute).toInt ();
		if (code >= 400)
		{
			// TODO handle error
			return;
		}

		const QString refreshRaw { reply->rawHeader ("Refresh") };
		const auto& path = refreshRaw.section (";URL=", 1);
		if (path.isEmpty ())
		{
			// TODO handle error
			return;
		}

		FeedURL ("http://paste.org.ru" + path);

		deleteLater ();
	}
}
