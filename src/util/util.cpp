/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2014  Georg Rudoy
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE or copy at https://www.boost.org/LICENSE_1_0.txt)
 **********************************************************************/

#include "util.h"
#include <functional>
#include <stdexcept>
#include <type_traits>
#include <QString>
#include <QApplication>
#include <QTranslator>
#include <QLocale>
#include <QFile>
#include <QDir>
#include <QTime>
#include <QSettings>
#include <QTextCodec>
#include <QUrl>
#include <QAction>
#include <QBuffer>
#include <QPainter>
#include <QAction>
#include <QtDebug>

QString LC::Util::GetAsBase64Src (const QImage& pix)
{
	QBuffer buf;
	buf.open (QIODevice::ReadWrite);
	pix.save (&buf, "PNG", 100);
	return QString ("data:image/png;base64,%1")
			.arg (QString (buf.buffer ().toBase64 ()));
}

QString LC::Util::GetUserText (const Entity& p)
{
	QString string = QObject::tr ("Too long to show");
	if (p.Additional_.contains ("UserVisibleName") &&
			p.Additional_ ["UserVisibleName"].canConvert<QString> ())
		string = p.Additional_ ["UserVisibleName"].toString ();
	else if (p.Entity_.canConvert<QByteArray> ())
	{
		QByteArray entity = p.Entity_.toByteArray ();
		if (entity.size () < 100)
			string = QTextCodec::codecForName ("UTF-8")->toUnicode (entity);
	}
	else if (p.Entity_.canConvert<QUrl> ())
	{
		string = p.Entity_.toUrl ().toString ();
		if (string.size () > 100)
			string = string.left (97) + "...";
	}
	else
		string = QObject::tr ("Binary entity");

	if (!p.Mime_.isEmpty ())
		string += QObject::tr ("<br /><br />of type <code>%1</code>").arg (p.Mime_);

	if (!p.Additional_ ["SourceURL"].toUrl ().isEmpty ())
	{
		QString urlStr = p.Additional_ ["SourceURL"].toUrl ().toString ();
		if (urlStr.size () > 63)
			urlStr = urlStr.left (60) + "...";
		string += QObject::tr ("<br />from %1")
			.arg (urlStr);
	}

	return string;
}

namespace
{
	QString MakePrettySizeWith (qint64 sourceSize, const QStringList& units)
	{
		int strNum = 0;
		long double size = sourceSize;

		for (; strNum < 3 && size >= 1024; ++strNum, size /= 1024)
			;

		return QString::number (size, 'f', 1) + units.value (strNum);
	}
}

QString LC::Util::MakePrettySize (qint64 sourcesize)
{
	static QStringList units
	{
		QObject::tr (" b"),
		QObject::tr (" KiB"),
		QObject::tr (" MiB"),
		QObject::tr (" GiB")
	};

	return MakePrettySizeWith (sourcesize, units);
}

QString LC::Util::MakePrettySizeShort (qint64 sourcesize)
{
	static const QStringList units
	{
		QObject::tr ("b", "Short one-character unit for bytes."),
		QObject::tr ("K", "Short one-character unit for kilobytes."),
		QObject::tr ("M", "Short one-character unit for megabytes."),
		QObject::tr ("G", "Short one-character unit for gigabytes.")
	};

	return MakePrettySizeWith (sourcesize, units);
}

QString LC::Util::MakeTimeFromLong (ulong time)
{
	int d = time / 86400;
	time -= d * 86400;
	QString result;
	if (d)
		result += QObject::tr ("%n day(s), ", "", d);
	result += QTime (0, 0, 0).addSecs (time).toString ();
	return result;
}

QTranslator* LC::Util::LoadTranslator (const QString& baseName,
		const QString& localeName,
		const QString& prefix,
		const QString& appName)
{
	auto filename = prefix;
	filename.append ("_");
	if (!baseName.isEmpty ())
		filename.append (baseName).append ("_");
	filename.append (localeName);

	auto transl = new QTranslator;
#ifdef Q_OS_WIN32
	Q_UNUSED (appName)
	if (transl->load (filename, ":/") ||
			transl->load (filename,
					QCoreApplication::applicationDirPath () + "/translations"))
#elif defined (Q_OS_MAC) && !defined (USE_UNIX_LAYOUT)
	Q_UNUSED (appName)
	if (transl->load (filename, ":/") ||
			transl->load (filename,
					QCoreApplication::applicationDirPath () + "/../Resources/translations"))
#elif defined (INSTALL_PREFIX)
	if (transl->load (filename, ":/") ||
			transl->load (filename,
					QString (INSTALL_PREFIX "/share/%1/translations").arg (appName)))
#else
	if (transl->load (filename, ":/") ||
			transl->load (filename,
					QString ("/usr/local/share/%1/translations").arg (appName)) ||
			transl->load (filename,
					QString ("/usr/share/%1/translations").arg (appName)))
#endif
		return transl;

	delete transl;

	return nullptr;
}

QTranslator* LC::Util::InstallTranslator (const QString& baseName,
		const QString& prefix,
		const QString& appName)
{
	const auto& localeName = GetLocaleName ();
	if (auto transl = LoadTranslator (baseName, localeName, prefix, appName))
	{
		qApp->installTranslator (transl);
		return transl;
	}

	qWarning () << Q_FUNC_INFO
			<< "could not load translation file for locale"
			<< localeName
			<< baseName
			<< prefix
			<< appName;
	return nullptr;
}

QString LC::Util::GetLocaleName ()
{
	QSettings settings (QCoreApplication::organizationName (),
			QCoreApplication::applicationName ());
	QString localeName = settings.value ("Language", "system").toString ();

	if (localeName == "system")
	{
		localeName = QString (::getenv ("LANG")).left (5);

		if (localeName == "C" || localeName.isEmpty ())
			localeName = "en_US";

		if (localeName.isEmpty () || localeName.size () != 5)
			localeName = QLocale::system ().name ();
		localeName = localeName.left (5);
	}

	if (localeName.size () == 2)
	{
		auto lang = QLocale (localeName).language ();
		const auto& cs = QLocale::countriesForLanguage (lang);
		if (cs.isEmpty ())
			localeName += "_00";
		else
			localeName = QLocale (lang, cs.at (0)).name ();
	}

	return localeName;
}

QString LC::Util::GetInternetLocaleName (const QLocale& locale)
{
	if (locale.language () == QLocale::AnyLanguage)
		return "*";

	auto locStr = locale.name ();
	locStr.replace ('_', '-');
	return locStr;
}

QString LC::Util::GetLanguage ()
{
	return GetLocaleName ().left (2);
}

QModelIndexList LC::Util::GetSummarySelectedRows (QObject *sender)
{
	QAction *senderAct = qobject_cast<QAction*> (sender);
	if (!senderAct)
	{
		QString debugString;
		{
			QDebug d (&debugString);
			d << "sender is not a QAction*"
					<< sender;
		}
		throw std::runtime_error (qPrintable (debugString));
	}

	return senderAct->
			property ("SelectedRows").value<QList<QModelIndex>> ();
}

QAction* LC::Util::CreateSeparator (QObject *parent)
{
	QAction *result = new QAction (parent);
	result->setSeparator (true);
	return result;
}

QPixmap LC::Util::DrawOverlayText (QPixmap px,
		const QString& text, QFont font, const QPen& pen, const QBrush& brush)
{
	const auto& iconSize = px.size () / px.devicePixelRatio ();

	const auto fontHeight = iconSize.height () * 0.45;
	font.setPixelSize (std::max (6., fontHeight));

	const QFontMetrics fm (font);
	const auto width = fm.horizontalAdvance (text) + 2. * iconSize.width () / 10.;
	const auto height = fm.height () + 2. * iconSize.height () / 10.;
	const bool tooSmall = width > iconSize.width ();

	const QRect textRect (iconSize.width () - width, iconSize.height () - height, width, height);

	QPainter p (&px);
	p.setBrush (brush);
	p.setFont (font);
	p.setPen (pen);
	p.setRenderHint (QPainter::Antialiasing);
	p.setRenderHint (QPainter::TextAntialiasing);
	p.drawRoundedRect (textRect, 4, 4);
	p.drawText (textRect,
			Qt::AlignCenter,
			tooSmall ? "#" : text);
	p.end ();

	return px;
}
