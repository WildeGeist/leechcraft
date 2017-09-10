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

#include <deque>
#include <vector>
#include <QStringList>
#include <QDateTime>
#include <QStandardItemModel>
#include <interfaces/core/ihookproxy.h>
#include <interfaces/poshuku/poshukutypes.h>

class QTimer;
class QAction;

namespace LeechCraft
{
namespace Poshuku
{
	class HistoryModel : public QStandardItemModel
	{
		Q_OBJECT

		QTimer * const GarbageTimer_;
		history_items_t Items_;
	public:
		enum Columns
		{
			ColumnTitle
			, ColumnURL
			, ColumnDate
		};

		HistoryModel (QObject* = 0);
	public slots:
		void addItem (QString title, QString url, QDateTime datetime);
		QList<QMap<QString, QVariant>> getItemsMap () const;
	private:
		void Add (const HistoryItem&, int section);
	private slots:
		void loadData ();
		void collectGarbage ();
		void handleItemAdded (const HistoryItem&);
	signals:
		// Hook support signals
		/** @brief Called when an entry is going to be added to
			* history.
			*
			* If the proxy is cancelled, no addition takes place
			* at all. If it is not, the return value from the proxy
			* is considered as a list of QVariants. First element
			* (if any) would be converted to string and replace
			* title, second element (if any) would be converted to
			* string and replace url, third element (if any) would
			* be converted to QDateTime and replace the date.
			*/
		void hookAddingToHistory (LeechCraft::IHookProxy_ptr proxy,
				QString title, QString url, QDateTime date);
	};
}
}

Q_DECLARE_METATYPE (LeechCraft::Poshuku::HistoryItem)
