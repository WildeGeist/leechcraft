/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2013  Georg Rudoy
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 **********************************************************************/

#pragma once

#include <memory>
#include <QImage>
#include <QMetaType>
#include <QStringList>
#include <QDateTime>
#include "ilink.h"

class QUrl;

namespace LeechCraft
{
namespace Monocle
{
	/** @brief Document metadata.
	 *
	 * All fields in this structure can be null.
	 */
	struct DocumentInfo
	{
		/** @brief Document title.
		 */
		QString Title_;
		/** @brief The subject line of this document.
		 */
		QString Subject_;
		/** @brief The author of the document.
		 */
		QString Author_;

		/** @brief Genres of this document.
		 */
		QStringList Genres_;
		/** @brief Keywords corresponding to this document.
		 */
		QStringList Keywords_;

		/** @brief Date this document was created.
		 */
		QDateTime Date_;
	};

	/** @brief Basic interface for documents.
	 *
	 * This interface is the basic interface for documents returned from
	 * format backends.
	 *
	 * Pages actions (like rendering) are also performed via this
	 * interface. Pages indexes are zero-based.
	 *
	 * This class has some signals, and one can use the GetQObject()
	 * method to get an object of this class as a QObject and connect to
	 * those signals.
	 *
	 * The document can also implement IDynamicDocument, IHaveTextContent,
	 * ISaveableDocument, ISearchableDocument, ISupportAnnotations,
	 * IHaveToc and ISupportForms. The interface names are pretty
	 * obvious. See them for the details.
	 *
	 * @sa IBackendPlugin::LoadDocument()
	 * @sa IDynamicDocument, IHaveTextContent, ISaveableDocument
	 * @sa ISearchableDocument, ISupportAnnotations, ISupportForms
	 * @sa IHaveToc
	 */
	class IDocument
	{
	public:
		/** @brief Virtual destructor.
		 */
		virtual ~IDocument () {}

		/** @brief Returns the parent backend plugin.
		 *
		 * This function should return the instance object of the backend
		 * plugin that created this document.
		 *
		 * The returned value should obviously implement IBackendPlugin.
		 *
		 * @return The parent backend plugin instance object.
		 */
		virtual QObject* GetBackendPlugin () const = 0;

		/** @brief Returns this object as a QObject.
		 *
		 * This function can be used to connect to the signals of this
		 * class.
		 *
		 * @return This object as a QObject.
		 */
		virtual QObject* GetQObject () = 0;

		/** @brief Returns whether this document is valid.
		 *
		 * An invalid document is basically equivalent to a null pointer,
		 * all operations on it lead to undefined behavior.
		 *
		 * @return Whether this document is valid.
		 */
		virtual bool IsValid () const = 0;

		/** @brief Returns the document metadata.
		 *
		 * @return The document metadata.
		 */
		virtual DocumentInfo GetDocumentInfo () const = 0;

		/** @brief Returns the number of pages i this document.
		 *
		 * @return The number of pages in this document.
		 */
		virtual int GetNumPages () const = 0;

		/** @brief Returns the size in pixels of the given page.
		 *
		 * This function returns the physical dimensions of the given
		 * \em page in pixels.
		 *
		 * Some formats support different pages having different sizes in
		 * the same document, thus the size should be queried for each
		 * \em page.
		 *
		 * @param[in] page The index of the page to query.
		 * @return The size of the given \em page.
		 */
		virtual QSize GetPageSize (int page) const = 0;

		/** @brief Renders the given \em page at the given scale.
		 *
		 * This function should return an image with the given \em page
		 * rendered at the given \em xScale and \em yScale for <em>x</em>
		 * and <em>y</em> axises correspondingly. That is, the image's
		 * size should be equal to the following:
		 * \code
		 * auto size = GetPageSize (page);
		 * size.rwidth () *= xScale;
		 * size.rheight () *= yscale;
		 * \endcode
		 *
		 * @param[in] page The index of the page to render.
		 * @param[in] xScale The scale of the <em>x</em> axis.
		 * @param[in] yScale The scale of the <em>y</em> axis.
		 * @return The rendering of the given page.
		 */
		virtual QImage RenderPage (int page, double xScale, double yScale) = 0;

		/** @brief Returns the links found at the given \em page.
		 *
		 * If the format doesn't support links, an empty list should be
		 * returned.
		 *
		 * @param[in] page The page index to query.
		 *
		 * @sa ILink
		 */
		virtual QList<ILink_ptr> GetPageLinks (int page) = 0;

		/** @brief Returns the URL of the document.
		 *
		 * This method should return the URL of the document which has
		 * been opened.
		 */
		virtual QUrl GetDocURL () const = 0;
	protected:
		/** @brief Emitted when navigation is requested.
		 *
		 * For example, this signal is emitted when a navigation link in
		 * a table of contents has been triggered.
		 *
		 * If \em filename is empty, \em pageNum, \em x and \em y are all
		 * related to he current document. Otherwise \em filename should
		 * be loaded first.
		 *
		 * \em x and \em y coordinates are absolute, that is, between 0
		 * and <code>GetPageSize (pageNum).width ()</code> and
		 * <code>GetPageSize (pageNum).height ()</code> correspondingly.
		 *
		 * @param[out] filename The filename of the document to navigate
		 * to, or an empty string if current document should be used.
		 * @param[out] pageNum The index of the page to navigate to.
		 * @param[out] x The new \em x coordinate of the viewport.
		 * @param[out] y The new \em y coordinate of the viewport.
		 */
		virtual void navigateRequested (const QString& filename, int pageNum, double x, double y) = 0;

		/** @brief Emitted when printing is requested.
		 *
		 * This signal is emitted when printing is requested, for
		 * example, by a link action.
		 *
		 * @param[out] pages The list of pages to print, or an empty list
		 * to print all pages.
		 */
		virtual void printRequested (const QList<int>& pages) = 0;
	};

	/** @brief Shared pointer to a document.
	 */
	typedef std::shared_ptr<IDocument> IDocument_ptr;
}
}

Q_DECLARE_INTERFACE (LeechCraft::Monocle::IDocument,
		"org.LeechCraft.Monocle.IDocument/1.0");
