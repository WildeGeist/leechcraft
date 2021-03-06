/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2014  Georg Rudoy
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE or copy at https://www.boost.org/LICENSE_1_0.txt)
 **********************************************************************/

#include "extensionsdataimpl.h"
#include <QIcon>
#include <QMutex>
#include <QtDebug>
#include <util/sll/util.h>
#include <qt_windows.h>
#include <shellapi.h>

namespace LC
{
namespace Util
{
	namespace
	{
		bool ReadValue (HKEY hKey, const QString& valueName, QString& result)
		{
			DWORD buffSize {};
			if (ERROR_SUCCESS == RegQueryValueExW (hKey,
					valueName.toStdWString ().c_str (),
					nullptr,
					nullptr,
					nullptr,
					&buffSize))
			{
				std::vector<wchar_t> buff (buffSize / sizeof (wchar_t));
				if (ERROR_SUCCESS == RegQueryValueExW (hKey,
						valueName.toStdWString ().c_str (),
						nullptr,
						nullptr,
						reinterpret_cast<LPBYTE> (&buff [0]),
						&buffSize))
				{
					result = QString::fromWCharArray (buff.data ());
					return true;
				}
			}
			return false;
		}

		bool ReadDefaultValue (const QString& fullKey, QString& result)
		{
			DWORD buffSize {};
			HKEY hKey {};
			if (ERROR_SUCCESS != RegOpenKeyW (HKEY_CLASSES_ROOT,
					fullKey.toStdWString ().c_str (),
					&hKey))
				return false;

			const auto regGuard = Util::MakeScopeGuard ([&hKey] { RegCloseKey (hKey); });

			DWORD type { REG_SZ };

			if (ERROR_SUCCESS == RegQueryValueExW (hKey,
					nullptr,
					nullptr,
					&type,
					nullptr,
					&buffSize))
			{
				std::vector<wchar_t> buff (buffSize / sizeof (wchar_t));
				if (ERROR_SUCCESS == RegQueryValueExW (hKey,
						nullptr,
						nullptr,
						&type,
						reinterpret_cast<LPBYTE> (&buff [0]),
						&buffSize))
				{
					result = QString::fromWCharArray (buff.data ());

					if (REG_EXPAND_SZ == type)
					{
						buffSize = ExpandEnvironmentStringsW (result.toStdWString ().c_str (), nullptr, 0);
						buff.resize (buffSize);
						if (!ExpandEnvironmentStringsW (result.toStdWString ().c_str (), &buff [0], buffSize))
						{
							return false;
						}
						result = QString::fromWCharArray (buff.data ());
					}
					return true;
				}
			}
			return false;
		}

		bool GetSubkeyNames (HKEY hKey, QList<QString>& result)
		{
			DWORD subKeys {};
			DWORD maxSubkeyLen {};

			if (ERROR_SUCCESS != RegQueryInfoKeyW (hKey,
					nullptr,
					nullptr,
					nullptr,
					&subKeys,
					&maxSubkeyLen,
					nullptr,
					nullptr,
					nullptr,
					nullptr,
					nullptr,
					nullptr))
			{
				qDebug () << Q_FUNC_INFO
					<< "RegQueryInfoKeyW failed.";
				return false;
			}
	
			result.clear ();
			std::vector<wchar_t> keyName (maxSubkeyLen + 1);
			for (DWORD i = 0; i < subKeys; ++i)
			{
				if (ERROR_SUCCESS != RegEnumKeyW (hKey,
						i,
						&keyName [0],
						keyName.size ()
						))
					break;
	
				result.append (QString::fromWCharArray (keyName.data ()));
			}
			return true;
		}

		QHash<QString, QString> ParseMimeDatabase ()
		{
			HKEY hKey {};

			static const QString kDatabaseKey { "Mime\\Database\\Content Type" };
			if (ERROR_SUCCESS != RegOpenKeyW (HKEY_CLASSES_ROOT,
					kDatabaseKey.toStdWString ().c_str (),
					&hKey))
				return {};

			QList<QString> mimes;
			if (!GetSubkeyNames (hKey, mimes))
			{
				RegCloseKey (hKey);
				return {};
			}

			RegCloseKey (hKey);

			QHash<QString, QString> result;
			for (const QString& mime : mimes)
			{
				static const QString kFormat { "Mime\\Database\\Content Type\\%1" };
				static const QString kExtension { "Extension" };
				static const QChar kDot {'.'};

				const QString fullKeyPath = kFormat.arg (mime);

				if (ERROR_SUCCESS != RegOpenKeyW (HKEY_CLASSES_ROOT,
						fullKeyPath.toStdWString ().c_str (),
						&hKey))
					continue;

				QString ext;
				if (ReadValue (hKey, kExtension, ext) && !ext.isEmpty ())
				{
					if (ext.at (0) == kDot)
						ext.remove (0, 1);
					result.insert (ext, mime);
				}

				RegCloseKey (hKey);
			}

			if (ERROR_SUCCESS != RegOpenKeyW (HKEY_CLASSES_ROOT,
					nullptr,
					&hKey))
				return result;

			QList<QString> extensions;
			if (!GetSubkeyNames (hKey, extensions))
			{
				RegCloseKey (hKey);
				return result;
			}
			RegCloseKey (hKey);

			static const QString kValueContentType { "Content Type" };
			for (int i = 0; i < extensions.count (); ++i)
			{
				static const auto prefix = QChar {'.'};
				if (!extensions.at (i).startsWith (prefix))
					continue;

				extensions [i].remove (0, 1);

				if (result.contains (extensions.at (i)))
					continue;

				if (ERROR_SUCCESS != RegOpenKeyW (HKEY_CLASSES_ROOT,
						QString { ".%1" }.arg (extensions.at (i)).toStdWString ().c_str (),
						&hKey))
					continue;

				QString mime;
				if (ReadValue (hKey, kValueContentType, mime) && !mime.isEmpty ())
					result.insert (extensions.at (i), mime);

				RegCloseKey (hKey);
			}
			return result;
		}
	}

	class ExtensionsDataImpl::Details
	{
	public:
		const QHash<QString, QString> Extension2Mime_;
		QMultiHash<QString, QString> Mime2Extension_;
		QHash<QString, QIcon> Extension2Icon_;
		QMutex IconsLock_;

		Details ();

		QIcon GetExtensionIcon (const QString& extension);
		QString MimeByExtension (const QString& extension) const;
		QString ExtensionByMime (const QString& mime) const;
	};

	ExtensionsDataImpl::Details::Details()
	: Extension2Mime_ { ParseMimeDatabase () }
	{
		for (auto it = Extension2Mime_.constBegin (); it != Extension2Mime_.constEnd (); ++it)
			Mime2Extension_.insertMulti (it.value (), it.key ());
	}

	QString ExtensionsDataImpl::Details::MimeByExtension (const QString& extension) const
	{
		return Extension2Mime_.value (extension);
	}

	QString ExtensionsDataImpl::Details::ExtensionByMime (const QString& mime) const
	{
		return Mime2Extension_.values (mime).first ();
	}

	QIcon ExtensionsDataImpl::Details::GetExtensionIcon (const QString& extension)
	{
		QMutexLocker lock { &IconsLock_ };
		if (Extension2Icon_.contains (extension))
			return Extension2Icon_.value (extension);
		lock.unlock ();

		static const QString kKeyDefaultIcon { "DefaultIcon" };
		static const QChar kDot { '.' };
		static const QChar kComma { ',' };

		const QString dottedExt = kDot + extension;
		QString defIconKey = dottedExt + "\\" + kKeyDefaultIcon;

		QString defIconPath;
		if (!ReadDefaultValue (defIconKey, defIconPath))
		{
			QString defaultType;
			if (ReadDefaultValue (dottedExt, defaultType))
			{
				defIconKey = defaultType + "\\" + kKeyDefaultIcon;
				ReadDefaultValue (defIconKey, defIconPath);
			}
		}

		if (defIconPath.isEmpty ())
			return {};

		const QStringList parts = defIconPath.split (kComma);
		if (2 != parts.count ())
			return {};

		QString path = parts.at (0);
		path.replace ("\"", {});

		const UINT index = parts.count () > 1 ? parts.at (1).toUInt () : 0;
		const HICON hIcon = ExtractIconW (GetModuleHandle (nullptr), path.toStdWString ().c_str (), index);

		QIcon icon;
		if (hIcon)
		{
			QPixmap pixmap { QPixmap::fromWinHICON (hIcon) };
			if (!pixmap.isNull ())
				icon.addPixmap (pixmap);
			DestroyIcon (hIcon);
		}
		if (!icon.isNull ())
		{
			lock.relock ();
			Extension2Icon_.insert (extension, icon);
			lock.unlock ();
		}
		return icon;
	}

	ExtensionsDataImpl::ExtensionsDataImpl ()
	: Details_ { new Details }
	{
	}

	const QHash<QString, QString>& ExtensionsDataImpl::GetMimeDatabase () const
	{
		return Details_->Extension2Mime_;
	}

	QIcon ExtensionsDataImpl::GetExtIcon (const QString& extension) const
	{
		const auto& lowerExtension = extension.toLower ();
		return Details_->GetExtensionIcon (lowerExtension);
	}

	QIcon ExtensionsDataImpl::GetMimeIcon (const QString& mime) const
	{
		const auto& ext = Details_->ExtensionByMime (mime);
		if (ext.isEmpty ())
			return {};

		return Details_->GetExtensionIcon (ext);
	}
}
}
