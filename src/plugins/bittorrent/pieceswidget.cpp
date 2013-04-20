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

#include "pieceswidget.h"
#include <QPainter>
#include <QPaintEvent>
#include <QVector>
#include <QtDebug>
#include <QApplication>
#include <QPalette>

namespace LeechCraft
{
	namespace Plugins
	{
		namespace BitTorrent
		{
			PiecesWidget::PiecesWidget (QWidget *parent)
			: QLabel (parent)
			{
			}
			
			void PiecesWidget::setPieceMap (const libtorrent::bitfield& pieces)
			{
				Pieces_ = pieces;
			
				update ();
			}
			
			QList<QPair<int, int>> FindTrues (const libtorrent::bitfield& pieces)
			{
				QList<QPair<int, int>> result;
				bool prevVal = pieces [0];
				int prevPos = 0;
				int size = static_cast<int> (pieces.size ());
				for (int i = 1; i < size; ++i)
					if (pieces [i] != prevVal)
					{
						if (prevVal)
							result << qMakePair (prevPos, i);
						prevPos = i;
						prevVal = 1 - prevVal;
					}
			
				if (!prevPos && prevVal)
					result << qMakePair<int, int> (0, pieces.size ());
				else if (prevVal && result.size () && result.last ().second != size - 1)
					result << qMakePair<int, int> (prevPos, size);
				else if (prevVal && !result.size ())
					result << qMakePair<int, int> (0, size);
			
				return result;
			}
			
			void PiecesWidget::paintEvent (QPaintEvent *e)
			{
				int s = Pieces_.size ();
				QPainter painter (this);
				painter.setRenderHints (QPainter::Antialiasing |
						QPainter::SmoothPixmapTransform);
				if (!s)
				{
					painter.setBackgroundMode (Qt::OpaqueMode);
					painter.setBackground (Qt::white);
					painter.end ();
					return;
				}
			
				const QPalette& palette = QApplication::palette ();
				const QColor& backgroundColor = palette.color (QPalette::Base);
				const QColor& downloadedPieceColor = palette.color (QPalette::Highlight);

				QPixmap tempPicture (s, 1);
				QPainter tempPainter (&tempPicture);
				tempPainter.setPen (backgroundColor);
				tempPainter.drawLine (0, 0, s, 0);
				QList<QPair<int, int>> trues = FindTrues (Pieces_);
				for (int i = 0; i < trues.size (); ++i)
				{
					QPair<int, int> pair = trues.at (i);
			
					tempPainter.setPen (downloadedPieceColor);
					tempPainter.drawLine (pair.first, 0, pair.second, 0);
				}
				tempPainter.end ();
			
				painter.drawPixmap (QRect (0, 0, width (), height ()), tempPicture);
				painter.end ();
			
				e->accept ();
			}
			
		};
	};
};

