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

#include <QByteArray>
#include <QUrl>
#include <QtPlugin>
#include <util/sll/eitherfwd.h>
#include "structures.h"

template<typename>
class QFuture;

struct EntityTestHandleResult;

/** @brief Common interface for all the downloaders.
 *
 * Plugins which provide downloading capabilities and want to be visible
 * by LeechCraft and other plugins as download providers should
 * implement this interface.
 *
 * Plugins implementing this interface are expected to have following
 * signals:
 * - jobFinished (int id)
 *   Indicates that a job with a given id has finished.
 * - jobFinished (int id)
 *   Indicates that a job with a given id has been removed.
 * - handleJobError (int id, IDownload::Error error)
 *   Indicates that an error occured while downloading a job with the
 *   given id.
 *
 * In order to obtain IDs for the tasks plugins are expected to use
 * ICoreProxy::GetID() in order to avoid name clashes.
 *
 * @sa IJobHolder, IEntityHandler
 */
class Q_DECL_EXPORT IDownload
{
public:
	struct Error
	{
		enum class Type
		{
			Unknown,
			NoError,
			NotFound,
			AccessDenied,
			LocalError,
			UserCanceled
		} Type_;

		QString Message_;
	};

	struct Success {};

	using Result = LeechCraft::Util::Either<Error, Success>;

	/** @brief Returns download speed.
	 *
	 * Returns summed up download speed of the plugin. The value is
	 * primarily used in the interface as there are no ways of
	 * controlling of bandwidth's usage of a particular plugin.
	 *
	 * @return Download speed in bytes.
	 *
	 * @sa GetUploadSpeed
	 */
	virtual qint64 GetDownloadSpeed () const = 0;
	/** @brief Returns upload speed.
	 *
	 * Returns summed up upload speed of the plugin. The value is
	 * primarily used in the interface as there are no ways of
	 * controlling of bandwidth's usage of a particular plugin.
	 *
	 * @return Upload speed in bytes.
	 *
	 * @sa GetDownloadSpeed
	 */
	virtual qint64 GetUploadSpeed () const = 0;

	/** @brief Starts all tasks.
	 *
	 * This is called by LeechCraft when it wants all plugins to start
	 * all of its tasks.
	 */
	virtual void StartAll () = 0;
	/** @brief Stops all tasks.
	 *
	 * This is called by LeechCraft when it wants all plugins to stop
	 * all of its tasks.
	 */
	virtual void StopAll () = 0;

	/** @brief Returns whether plugin can handle given entity.
	 *
	 * This function is used to query every loaded plugin providing the
	 * IDownload interface whether it could handle the entity entered by
	 * user or generated automatically with given task parameters.
	 * Entity could be anything from file name to URL to all kinds of
	 * hashes like Magnet links.
	 *
	 * @param[in] entity A Entity structure.
	 * @return The result of the test whether the \em entity can be
	 * handled.
	 *
	 * @sa AddJob
	 * @sa LeechCraft::Entity
	 */
	virtual EntityTestHandleResult CouldDownload (const LeechCraft::Entity& entity) const = 0;

	/** @brief Adds the job with given parameters.
	 *
	 * Adds the job to the downloader and returns the ID of the newly
	 * added job back to identify it.
	 *
	 * @param[in] entity A Entity structure.
	 * @return ID of the job for the other plugins to use.
	 *
	 * @sa LeechCraft::Entity
	 */
	virtual QPair<int, QFuture<Result>> AddJob (LeechCraft::Entity entity) = 0;

	/** @brief Kills the task with the given id.
	 *
	 * Kills the task with the id previously returned from AddJob. If
	 * there is no such task, the function shall leave the downloader
	 * in a good state. Ignoring will do.
	 *
	 * @param[in] id ID of the task previously added with AddJob().
	 */
	virtual void KillTask (int id) = 0;

	/** @brief Virtual destructor.
	 */
	virtual ~IDownload () {}
};

Q_DECLARE_INTERFACE (IDownload, "org.Deviant.LeechCraft.IDownload/1.0")

Q_DECLARE_METATYPE (QList<QUrl>)
