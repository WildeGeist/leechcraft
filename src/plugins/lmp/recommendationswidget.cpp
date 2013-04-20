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

#include "recommendationswidget.h"
#include <QtDebug>
#include <interfaces/core/ipluginsmanager.h>
#include <interfaces/media/irecommendedartists.h>
#include <interfaces/media/iaudioscrobbler.h>
#include <interfaces/media/ipendingsimilarartists.h>
#include "core.h"
#include "xmlsettingsmanager.h"
#include "util.h"

namespace LeechCraft
{
namespace LMP
{
	RecommendationsWidget::RecommendationsWidget (QWidget *parent)
	: QWidget (parent)
	{
		Ui_.setupUi (this);
	}

	void RecommendationsWidget::InitializeProviders ()
	{
		const auto& lastProv = ShouldRememberProvs () ?
				XmlSettingsManager::Instance ()
					.Property ("LastUsedRecsProvider", QString ()).toString () :
				QString ();

		bool lastFound = false;

		const auto& roots = Core::Instance ().GetProxy ()->GetPluginsManager ()->
				GetAllCastableRoots<Media::IRecommendedArtists*> ();
		Q_FOREACH (auto root, roots)
		{
			auto scrob = qobject_cast<Media::IAudioScrobbler*> (root);
			if (!scrob)
				continue;

			Ui_.RecProvider_->addItem (scrob->GetServiceName ());
			ProvRoots_ << root;
			Providers_ << qobject_cast<Media::IRecommendedArtists*> (root);

			if (scrob->GetServiceName () == lastProv)
			{
				const int idx = Providers_.size () - 1;
				Ui_.RecProvider_->setCurrentIndex (idx);
				on_RecProvider__activated (idx);
				lastFound = true;
			}
		}

		if (!lastFound)
			Ui_.RecProvider_->setCurrentIndex (-1);
	}

	void RecommendationsWidget::handleGotRecs ()
	{
		auto pending = qobject_cast<Media::IPendingSimilarArtists*> (sender ());
		if (!pending)
		{
			qWarning () << Q_FUNC_INFO
					<< "not a pending sender"
					<< sender ();
			return;
		}
		const auto& similars = pending->GetSimilar ();

		Ui_.RecView_->SetSimilarArtists (similars);
	}

	void RecommendationsWidget::on_RecProvider__activated (int index)
	{
		if (index < 0 || index >= Providers_.size ())
			return;

		auto pending = Providers_.at (index)->RequestRecommended (10);
		connect (pending->GetQObject (),
				SIGNAL (ready ()),
				this,
				SLOT (handleGotRecs ()));

		auto scrob = qobject_cast<Media::IAudioScrobbler*> (ProvRoots_.at (index));
		XmlSettingsManager::Instance ()
				.setProperty ("LastUsedRecsProvider", scrob->GetServiceName ());
	}
}
}
