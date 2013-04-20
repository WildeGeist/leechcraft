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

#ifndef PLUGINS_AZOTH_INTERFACES_IURIHANDLER_H
#define PLUGINS_AZOTH_INTERFACES_IURIHANDLER_H
#include <QUrl>
#include <QObject>

namespace LeechCraft
{
namespace Azoth
{
	/** @brief This interface is for protocols that may handle URIs and
	 * corresponding actions are dependent on an exact account.
	 * 
	 * The protocols that implement this interface are queried by the
	 * Azoth Core whenever an URI should be checked if it could be
	 * handled. If a protocol defines that it supports a given URI by
	 * returning true from SupportsURI() method, then Core queries for
	 * the list of accounts of this protocol and asks user to select one
	 * that should be used to handle the URI.
	 * 
	 * If several different protocols show that they support a given
	 * URI, then accounts from all of them would be suggested to the
	 * user.
	 */
	class IURIHandler
	{
	public:
		virtual ~IURIHandler () {}
		
		/** @brief Queries whether the given URI is supported.
		 * 
		 * @param[in] uri The URI to query.
		 * 
		 * @return Whether this URI could be handled by an account of
		 * this protocol.
		 */
		virtual bool SupportsURI (const QUrl& uri) const = 0;
		
		/** @brief Asks to handle the given URI by the given account.
		 * 
		 * The account is selected by the user from the list of accounts
		 * of this protocol if SupportsURI() returned true for the given
		 * URI.
		 * 
		 * @param[in] uri The URI to handle.
		 * @param[in] asAccount The account to use to handle this URI.
		 */
		virtual void HandleURI (const QUrl& uri, QObject *asAccount) = 0;
	};
}
}

Q_DECLARE_INTERFACE (LeechCraft::Azoth::IURIHandler,
		"org.Deviant.LeechCraft.Azoth.IURIHandler/1.0");

#endif
