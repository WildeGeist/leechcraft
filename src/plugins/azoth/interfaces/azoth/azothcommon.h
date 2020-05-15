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

#include <QMetaType>
#include <interfaces/iactionsexporter.h>
#include <interfaces/azoth/imessage.h>

namespace LC
{
namespace Azoth
{
	/** @brief Describes possible presence states of an account or a
	 * contact.
	 */
	enum State
	{
		SOffline,
		SOnline,
		SAway,
		SXA,
		SDND,
		SChat,
		SInvisible,
		SProbe,
		SError,
		SInvalid,

		/** Makes sense only for account state. This state is used when
		 * account is connecting and the moment and hasn't connected
		 * successfully and neither has failed yet.
		 */
		SConnecting
	};

	/** @brief Compares two states according to the implied desire to
	 * have a conversation.
	 *
	 * State \em s1 is less than state \em s2 if a contact in state
	 * \em s1 is more likely to want or be ready to have a conversation
	 * than a contact in state \em s2.
	 *
	 * For instance, <code>IsLess(State::SOnline, State::SDND)</code>
	 * holds, just as <code>IsLess(State::SChat, State::SOnline)</code>
	 * does.
	 *
	 * @param[in] s1 First state to compare.
	 * @param[in] s2 Second state to compare.
	 * @return Whether \em s1 implies more desire to have a conversation
	 * than \em s2.
	 */
	inline bool IsLess (State s1, State s2)
	{
		constexpr int order [] = { 7, 3, 4, 5, 6, 1, 2, 8, 9, 10 };
		return order [s1] < order [s2];
	}

	/** Represents possible state of authorizations between two
	 * entities: our user and a remote contact.
	 *
	 * Modelled after RFC 3921, Section 9.
	 */
	enum AuthStatus
	{
		/** Contact and user are not subscribed to each other, and
		 * neither has requested a subscription from the other.
		 */
		ASNone = 0x00,

		/** Contact is subscribed to user (one-way).
		 */
		ASFrom = 0x01,

		/** User is subscribed to contact (one-way).
		 */
		ASTo = 0x02,

		/** User and contact are subscribed to each other (two-way).
		 */
		ASBoth = 0x03,

		/** Contact has requested our subscription.
		 */
		ASContactRequested = 0x08
	};

	/** Represents possible chat states.
	 *
	 * Modelled after XMPP XEP-085.
	 */
	enum ChatPartState
	{
		/** Unknown chat state.
		 */
		CPSNone,

		/** User is actively participating in the chat session.
		 */
		CPSActive,

		/** User has not been actively participating in the chat
		 * session.
		 */
		CPSInactive,

		/** User has effectively ended their participation in the chat
		 * session.
		 */
		CPSGone,

		/** User is composing a message.
		 */
		CPSComposing,

		/** User had been composing but now has stopped.
		 */
		CPSPaused
	};

	/** @brief A custom saved named status.
	 */
	struct CustomStatus
	{
		/** @brief The name of this status.
		 */
		QString Name_;

		/** @brief The state associated with this status.
		 */
		State State_;

		/** @brief The status text associated with this status.
		 */
		QString Text_;
	};
}
}

Q_DECLARE_METATYPE (LC::Azoth::State)
Q_DECLARE_METATYPE (LC::Azoth::ChatPartState)
