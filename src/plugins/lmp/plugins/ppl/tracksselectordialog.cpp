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

#include "tracksselectordialog.h"
#include <numeric>
#include <QAbstractItemModel>
#include <QApplication>
#include <QDesktopWidget>
#include <QtDebug>
#include <util/sll/prelude.h>
#include <util/sll/views.h>
#include <util/sll/util.h>

namespace LeechCraft
{
namespace LMP
{
namespace PPL
{
	class TracksSelectorDialog::TracksModel : public QAbstractItemModel
	{
		const QStringList HeaderLabels_;

		enum Header : uint8_t
		{
			ScrobbleSummary,
			Artist,
			Album,
			Track,
			Date
		};
		static constexpr uint8_t MaxPredefinedHeader = Header::Date;

		const Media::IAudioScrobbler::BackdatedTracks_t Tracks_;

		QVector<QVector<bool>> Scrobble_;
	public:
		TracksModel (const Media::IAudioScrobbler::BackdatedTracks_t&,
				const QList<Media::IAudioScrobbler*>&, QObject* = nullptr);

		QModelIndex index (int, int, const QModelIndex& = {}) const override;
		QModelIndex parent (const QModelIndex&) const override;
		int rowCount (const QModelIndex& = {}) const override;
		int columnCount (const QModelIndex&) const override;
		QVariant data (const QModelIndex&, int) const override;

		QVariant headerData (int, Qt::Orientation, int) const override;

		Qt::ItemFlags flags (const QModelIndex& index) const override;
		bool setData (const QModelIndex& index, const QVariant& value, int role) override;

		QList<TracksSelectorDialog::SelectedTrack> GetSelectedTracks () const;

		void MarkAll ();
		void UnmarkAll ();
		void SetMarked (const QList<QModelIndex>&, bool);
	private:
		template<typename Summary, typename Specific>
		auto WithCheckableColumns (const QModelIndex& index, Summary&& summary, Specific&& specific) const
		{
			switch (index.column ())
			{
			case Header::Artist:
			case Header::Album:
			case Header::Track:
			case Header::Date:
				using ResultType_t = std::result_of_t<Summary (int)>;
				if constexpr (std::is_same_v<ResultType_t, void>)
					return;
				else
					return ResultType_t {};
			case Header::ScrobbleSummary:
				return summary (index.row ());
			}

			return specific (index.row (), index.column () - (MaxPredefinedHeader + 1));
		}

		void MarkRow (const QModelIndex&, bool);
	};

	TracksSelectorDialog::TracksModel::TracksModel (const Media::IAudioScrobbler::BackdatedTracks_t& tracks,
			const QList<Media::IAudioScrobbler*>& scrobblers, QObject *parent)
	: QAbstractItemModel { parent }
	, HeaderLabels_
	{
		[&scrobblers]
		{
			const QStringList predefined
			{
				{},
				tr ("Artist"),
				tr ("Album"),
				tr ("Title"),
				tr ("Date")
			};
			const auto& scrobbleNames = Util::Map (scrobblers, &Media::IAudioScrobbler::GetServiceName);
			return predefined + scrobbleNames;
		} ()
	}
	, Tracks_ { tracks }
	, Scrobble_ { tracks.size (), QVector<bool> (scrobblers.size (), true) }
	{
	}

	QModelIndex TracksSelectorDialog::TracksModel::index (int row, int column, const QModelIndex& parent) const
	{
		return parent.isValid () ?
				QModelIndex {} :
				createIndex (row, column);
	}

	QModelIndex TracksSelectorDialog::TracksModel::parent (const QModelIndex&) const
	{
		return {};
	}

	int TracksSelectorDialog::TracksModel::rowCount (const QModelIndex& parent) const
	{
		return parent.isValid () ?
				0 :
				Tracks_.size () + 1;
	}

	int TracksSelectorDialog::TracksModel::columnCount (const QModelIndex& parent) const
	{
		return parent.isValid () ?
				0 :
				HeaderLabels_.size ();
	}

	namespace
	{
		template<typename H, typename D>
		auto WithIndex (const QModelIndex& index, H&& header, D&& data)
		{
			if (!index.row ())
				return header (index);
			else
				return data (index.sibling (index.row () - 1, index.column ()));
		}

		QVariant PartialCheck (int enabled, int total)
		{
			if (!enabled)
				return Qt::Unchecked;
			else if (enabled == total)
				return Qt::Checked;
			else
				return Qt::PartiallyChecked;
		}
	}

	QVariant TracksSelectorDialog::TracksModel::data (const QModelIndex& srcIdx, int role) const
	{
		return WithIndex (srcIdx,
				[&] (const QModelIndex& index) -> QVariant
				{
					if (role != Qt::CheckStateRole)
						return {};

					return WithCheckableColumns (index,
							[this] (int)
							{
								const auto enabled = std::accumulate (Scrobble_.begin (), Scrobble_.end (), 0,
										[] (int val, const auto& subvec)
										{
											return std::accumulate (subvec.begin (), subvec.end (), val);
										});
								const auto total = Scrobble_.size () * Scrobble_ [0].size ();
								return PartialCheck (enabled, total);
							},
							[this] (int, int column)
							{
								const auto enabled = std::accumulate (Scrobble_.begin (), Scrobble_.end (), 0,
										[column] (int val, const auto& subvec) { return val + subvec.at (column); });
								return PartialCheck (enabled, Scrobble_.size ());
							});
				},
				[&] (const QModelIndex& index) -> QVariant
				{
					switch (role)
					{
					case Qt::DisplayRole:
					{
						const auto& record = Tracks_.value (index.row ());

						switch (index.column ())
						{
						case Header::ScrobbleSummary:
							return {};
						case Header::Artist:
							return record.first.Artist_;
						case Header::Album:
							return record.first.Album_;
						case Header::Track:
							return record.first.Title_;
						case Header::Date:
							return record.second.toString ();
						}

						return {};
					}
					case Qt::CheckStateRole:
					{
						return WithCheckableColumns (index,
								[&] (int row)
								{
									const auto& flags = Scrobble_.value (row);
									const auto enabled = std::accumulate (flags.begin (), flags.end (), 0);
									return PartialCheck (enabled, flags.size ());
								},
								[&] (int row, int column) -> QVariant
								{
									return Scrobble_.value (row).value (column) ?
											Qt::Checked :
											Qt::Unchecked;
								});
					}
					default:
						return {};
					}
				});
	}

	QVariant TracksSelectorDialog::TracksModel::headerData (int section, Qt::Orientation orientation, int role) const
	{
		if (role != Qt::DisplayRole)
			return {};

		switch (orientation)
		{
		case Qt::Horizontal:
			return HeaderLabels_.value (section);
		case Qt::Vertical:
			return section ?
					QString::number (section) :
					tr ("All");
		default:
			return {};
		}
	}

	Qt::ItemFlags TracksSelectorDialog::TracksModel::flags (const QModelIndex& index) const
	{
		switch (index.column ())
		{
		case Header::Artist:
		case Header::Album:
		case Header::Track:
		case Header::Date:
			return QAbstractItemModel::flags (index);
		default:
			return Qt::ItemIsSelectable |
					Qt::ItemIsEnabled |
					Qt::ItemIsUserCheckable;
		}
	}

	bool TracksSelectorDialog::TracksModel::setData (const QModelIndex& srcIdx, const QVariant& value, int role)
	{
		if (role != Qt::CheckStateRole)
			return false;

		MarkRow (srcIdx, value.toInt () == Qt::Checked);
		return true;
	}

	QList<TracksSelectorDialog::SelectedTrack> TracksSelectorDialog::TracksModel::GetSelectedTracks () const
	{
		QList<TracksSelectorDialog::SelectedTrack> result;
		for (const auto& pair : Util::Views::Zip<std::pair> (Tracks_, Scrobble_))
			if (std::any_of (pair.second.begin (), pair.second.end (), Util::Id))
				result.push_back ({ pair.first, pair.second });
		return result;
	}

	void TracksSelectorDialog::TracksModel::MarkAll ()
	{
		for (int i = 0; i < rowCount (); ++i)
			MarkRow (index (i, Header::ScrobbleSummary), true);
	}

	void TracksSelectorDialog::TracksModel::UnmarkAll ()
	{
		for (int i = 0; i < rowCount (); ++i)
			MarkRow (index (i, Header::ScrobbleSummary), false);
	}

	void TracksSelectorDialog::TracksModel::SetMarked (const QList<QModelIndex>& indices, bool shouldScrobble)
	{
		for (const auto& idx : indices)
			MarkRow (idx, shouldScrobble);
	}

	void TracksSelectorDialog::TracksModel::MarkRow (const QModelIndex& srcIdx, bool shouldScrobble)
	{
		WithIndex (srcIdx,
				[&] (const QModelIndex& index)
				{
					WithCheckableColumns (index,
							[&] (int)
							{
								for (auto& subvec : Scrobble_)
									std::fill (subvec.begin (), subvec.end (), shouldScrobble);
							},
							[&] (int, int column)
							{
								for (auto& subvec : Scrobble_)
									subvec [column] = shouldScrobble;
							});

					const auto lastRow = rowCount (index.parent ()) - 1;
					const auto lastColumn = columnCount (index.parent ()) - 1;
					emit dataChanged (createIndex (0, 0), createIndex (lastRow, lastColumn));
				},
				[&] (const QModelIndex& index)
				{
					auto& scrobbles = Scrobble_ [index.row ()];

					WithCheckableColumns (index,
							[&] (int) { std::fill (scrobbles.begin (), scrobbles.end (), shouldScrobble); },
							[&] (int, int column) { scrobbles [column] = shouldScrobble; });

					const auto lastColumn = columnCount (index.parent ()) - 1;
					emit dataChanged (createIndex (0, 0), createIndex (0, lastColumn));
					emit dataChanged (index.sibling (index.row (), 0), index.sibling (index.row (), lastColumn));
				});
	}

	TracksSelectorDialog::TracksSelectorDialog (const Media::IAudioScrobbler::BackdatedTracks_t& tracks,
			const QList<Media::IAudioScrobbler*>& scrobblers,
			QWidget *parent)
	: QDialog { parent }
	, Model_ { new TracksModel { tracks, scrobblers, this } }
	{
		Ui_.setupUi (this);
		Ui_.Tracks_->setModel (Model_);

		FixSize ();

		auto withSelected = [this] (bool shouldScrobble)
		{
			return [this, shouldScrobble]
			{
				const auto& current = Ui_.Tracks_->currentIndex ();
				const auto column = current.isValid () && (current.flags () & Qt::ItemIsUserCheckable) ?
						current.column () :
						0;

				auto indices = Ui_.Tracks_->selectionModel ()->selectedIndexes ();

				auto columnEnd = std::partition (indices.begin (), indices.end (),
						[&column] (const auto& index) { return index.column () == column; });

				std::sort (indices.begin (), columnEnd, Util::ComparingBy (&QModelIndex::row));
				std::sort (columnEnd, indices.end (), Util::ComparingBy (&QModelIndex::row));
				indices.erase (std::unique (columnEnd, indices.end (), Util::EqualityBy (&QModelIndex::row)),
						indices.end ());

				std::inplace_merge (indices.begin (), columnEnd, indices.end (), Util::ComparingBy (&QModelIndex::row));
				indices.erase (std::unique (indices.begin (), indices.end (), Util::EqualityBy (&QModelIndex::row)),
						indices.end ());

				Model_->SetMarked (indices, shouldScrobble);
			};
		};

		connect (Ui_.MarkAll_,
				&QPushButton::released,
				[this] { Model_->MarkAll (); });
		connect (Ui_.UnmarkAll_,
				&QPushButton::released,
				[this] { Model_->UnmarkAll (); });
		connect (Ui_.MarkSelected_,
				&QPushButton::released,
				withSelected (true));
		connect (Ui_.UnmarkSelected_,
				&QPushButton::released,
				withSelected (false));
	}

	void TracksSelectorDialog::FixSize ()
	{
		Ui_.Tracks_->resizeColumnsToContents ();

		const auto showGuard = Util::MakeScopeGuard ([this] { show (); });

		const auto Margin = 50;

		int totalWidth = Margin + Ui_.Tracks_->verticalHeader ()->width ();

		const auto header = Ui_.Tracks_->horizontalHeader ();
		for (int j = 0; j < Model_->columnCount ({}); ++j)
			totalWidth += std::max (header->sectionSize (j),
					Ui_.Tracks_->sizeHintForIndex (Model_->index (0, j, {})).width ());

		if (totalWidth < size ().width ())
			return;

		const auto desktop = qApp->desktop ();
		const auto& availableGeometry = desktop->availableGeometry (this);
		if (totalWidth > availableGeometry.width ())
			return;

		setGeometry (QStyle::alignedRect (Qt::LeftToRight,
				Qt::AlignCenter,
				{ totalWidth, height () },
				availableGeometry));
	}

	QList<TracksSelectorDialog::SelectedTrack> TracksSelectorDialog::GetSelectedTracks () const
	{
		return Model_->GetSelectedTracks ();
	}
}
}
}
