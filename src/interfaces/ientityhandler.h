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

#ifndef INTERFACES_IENTITYHANDLER_H
#define INTERFACES_IENTITYHANDLER_H
#include <QByteArray>
#include <QtPlugin>
#include "structures.h"

struct EntityTestHandleResult;

/** @brief Interface for plugins able to handle entities.
 *
 * This is similar to IDownload, but it doesn't require to implement all
 * functions of IDownload, and IDownloaders and IEntityHandlers are
 * shown in different groups when presented to user, for example, in
 * entity handle dialog.
 *
 * @sa IDownload
 */
class Q_DECL_EXPORT IEntityHandler
{
public:
	/** @brief Returns whether plugin can handle given entity.
	 *
	 * This function is used to query every loaded plugin providing the
	 * IDownload interface whether it could handle the entity entered by
	 * user or generated automatically with given task parameters.
	 * Entity could be anything from file name to URL to all kinds of
	 * hashes like Magnet links.
	 *
	 * @param[in] entity A Entity structure that could possibly be
	 * handled by this plugin.
	 * @return The result of testing whether the \em entity can be
	 * handled.
	 *
	 * @sa Handle
	 * @sa LeechCraft::Entity
	 */
	virtual EntityTestHandleResult CouldHandle (const LeechCraft::Entity& entity) const = 0;

	/** @brief Notifies the plugin that it should handle the given entity.
	 *
	 * This function is called to make IEntityHandle know that it should
	 * handle the given entity. The entity is guaranteed to be checked
	 * previously against CouldHandle().
	 *
	 * @param[in] entity A Entity structure to be handled by
	 * this plugin.
	 *
	 * @sa LeechCraft::Entity
	 */
	virtual void Handle (LeechCraft::Entity entity) = 0;

	virtual ~IEntityHandler () {}
};

Q_DECLARE_INTERFACE (IEntityHandler, "org.Deviant.LeechCraft.IEntityHandler/1.0");

#endif

