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

#pragma once

#include <util/utilconfig.h>
#include <interfaces/structures.h>

namespace LeechCraft
{
namespace Util
{
	/** @brief Creates an Advanced Notifications-enabled notify entity.
	 *
	 * Returns an entity with the given \em header, \em text and a bunch
	 * of other parameters that can be handled by Advanced Notifications.
	 *
	 * The returned entity will also be handled by a visual notifications
	 * plugin if AN is not present. To avoid this set the MIME type of
	 * the result to <em>x-leechcraft/notification+advanced</em>.
	 *
	 * Refer to the <a href="http://leechcraft.org/development-advanced-notifications">hand-written documentation</a>
	 * for more information about using Advanced Notifications.
	 *
	 * @param[in] header The header of the notification. This field will
	 * also be used if AN is not present.
	 * @param[in] text The text of the notification. This field will also
	 * be used if AN is not present.
	 * @param[in] priority The priority of this notification.
	 * @param[in] senderID The ID of the plugin sending this notification.
	 * @param[in] cat The category of this notification (one of Cat
	 * constants in interfaces/an/constants.h).
	 * @param[in] type The type of this notification (one of Type
	 * constants in interfaces/an/constants.h).
	 * @param[in] id The ID of this notification, used to group
	 * consecutive notifications about similar events like incoming
	 * message from the same IM contact.
	 * @param[in] visualPath The list of names for a menu-like structure
	 * wishing to show this notification.
	 * @param[in] delta The change of count of events with this id, or 0
	 * to use count.
	 * @param[in] count The total count of events with this id, used if
	 * delta is 0.
	 * @param[in] fullText The full text of this notification. If null,
	 * the text parameter is used.
	 * @param[in] extendedText The extended text of this notification. If
	 * null, the text parameter is used.
	 *
	 * @sa MakeANCancel()
	 */
	UTIL_API Entity MakeAN (const QString& header, const QString& text, Priority priority,
			const QString& senderID, const QString& cat, const QString& type,
			const QString& id, const QStringList& visualPath,
			int delta = 1, int count = 0,
			const QString& fullText = QString (), const QString& extendedText = QString ());
}
}
