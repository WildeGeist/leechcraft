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

#include <interfaces/an/constants.h>
#include "xpcconfig.h"

namespace LeechCraft::AN
{
	UTIL_XPC_API const QString CatEventCancel { "org.LC.AdvNotifications.Cancel" };

	UTIL_XPC_API const QString CatIM { "org.LC.AdvNotifications.IM" };
	UTIL_XPC_API const QString TypeIMAttention { CatIM + ".AttentionDrawn" };
	UTIL_XPC_API const QString TypeIMIncFile { CatIM + ".IncomingFile" };
	UTIL_XPC_API const QString TypeIMIncMsg { CatIM + ".IncomingMessage" };
	UTIL_XPC_API const QString TypeIMMUCHighlight { CatIM + ".MUCHighlightMessage" };
	UTIL_XPC_API const QString TypeIMMUCInvite { CatIM + ".MUCInvitation" };
	UTIL_XPC_API const QString TypeIMMUCMsg { CatIM + ".MUCMessage" };
	UTIL_XPC_API const QString TypeIMStatusChange { CatIM + ".StatusChange" };
	UTIL_XPC_API const QString TypeIMSubscrGrant { CatIM + ".Subscr.Granted" };
	UTIL_XPC_API const QString TypeIMSubscrRevoke { CatIM + ".Subscr.Revoked" };
	UTIL_XPC_API const QString TypeIMSubscrRequest { CatIM + ".Subscr.Requested" };
	UTIL_XPC_API const QString TypeIMSubscrSub { CatIM + ".Subscr.Subscribed" };
	UTIL_XPC_API const QString TypeIMSubscrUnsub { CatIM + ".Subscr.Unsubscribed" };
	UTIL_XPC_API const QString TypeIMEventTuneChange { CatIM + ".Event.Tune" };
	UTIL_XPC_API const QString TypeIMEventMoodChange { CatIM + ".Event.Mood" };
	UTIL_XPC_API const QString TypeIMEventActivityChange { CatIM + ".Event.Activity" };
	UTIL_XPC_API const QString TypeIMEventLocationChange { CatIM + ".Event.Location" };

	UTIL_XPC_API const QString CatOrganizer { "org.LC.AdvNotifications.Organizer" };
	UTIL_XPC_API const QString TypeOrganizerEventDue { CatOrganizer + ".EventDue" };

	UTIL_XPC_API const QString CatDownloads { "org.LC.AdvNotifications.Downloads" };
	UTIL_XPC_API const QString TypeDownloadFinished { CatDownloads + ".DownloadFinished" };
	UTIL_XPC_API const QString TypeDownloadError { CatDownloads + ".DownloadError" };

	UTIL_XPC_API const QString CatPackageManager { "org.LC.AdvNotifications.PackageManager" };
	UTIL_XPC_API const QString TypePackageUpdated { CatPackageManager + ".PackageUpdated" };

	UTIL_XPC_API const QString CatMediaPlayer { "org.LC.AdvNotifications.MediaPlayer" };
	UTIL_XPC_API const QString TypeMediaPlaybackStatus { CatMediaPlayer + ".PlaybackStatus" };

	UTIL_XPC_API const QString CatTerminal { "org.LC.AdvNotifications.Terminal" };
	UTIL_XPC_API const QString TypeTerminalBell { CatTerminal + ".Bell" };
	UTIL_XPC_API const QString TypeTerminalActivity { CatTerminal + ".Activity" };
	UTIL_XPC_API const QString TypeTerminalInactivity { CatTerminal + ".Inactivity" };

	UTIL_XPC_API const QString CatGeneric { "org.LC.AdvNotifications.Generic" };
	UTIL_XPC_API const QString TypeGeneric { CatGeneric + ".Generic" };

	namespace Field
	{
		UTIL_XPC_API const QString MediaPlayerURL { CatMediaPlayer + ".Fields.URL" };
		UTIL_XPC_API const QString MediaPlaybackStatus { CatMediaPlayer + ".Fields.PlaybackStatus" };
		UTIL_XPC_API const QString MediaTitle { CatMediaPlayer + ".Fields.Title" };
		UTIL_XPC_API const QString MediaArtist { CatMediaPlayer + ".Fields.Artist" };
		UTIL_XPC_API const QString MediaAlbum { CatMediaPlayer + ".Fields.Album" };
		UTIL_XPC_API const QString MediaLength { CatMediaPlayer + ".Fields.Length" };
		UTIL_XPC_API const QString TerminalActive { CatTerminal + ".Fields.Active" };
		UTIL_XPC_API const QString IMActivityGeneral { CatIM + ".Fields.Activity.General" };
		UTIL_XPC_API const QString IMActivitySpecific { CatIM + ".Fields.Activity.Specific" };
		UTIL_XPC_API const QString IMActivityText { CatIM + ".Fields.Activity.Text" };
		UTIL_XPC_API const QString IMMoodGeneral { CatIM + ".Fields.Mood.General" };
		UTIL_XPC_API const QString IMMoodText { CatIM + ".Fields.Mood.Text" };
		UTIL_XPC_API const QString IMLocationLongitude { CatIM + ".Fields.Location.Longitude" };
		UTIL_XPC_API const QString IMLocationLatitude { CatIM + ".Fields.Location.Latitude" };
		UTIL_XPC_API const QString IMLocationCountry { CatIM + ".Fields.Location.Country" };
		UTIL_XPC_API const QString IMLocationLocality { CatIM + ".Fields.Location.Locality" };
	}
}
