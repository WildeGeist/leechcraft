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

#include "actionsstructs.h"
#include <QAction>
#include "aggregator.h"

namespace LeechCraft
{
namespace Aggregator
{
	void AppWideActions::SetupActionsStruct (QWidget *parent)
	{
		ActionAddFeed_ = new QAction (tr ("Add feed..."),
				parent);
		ActionAddFeed_->setObjectName ("ActionAddFeed_");
		ActionAddFeed_->setProperty ("ActionIcon", "list-add");

		ActionUpdateFeeds_ = new QAction (tr ("Update all feeds"),
				parent);
		ActionUpdateFeeds_->setProperty ("ActionIcon", "mail-receive");

		ActionRegexpMatcher_ = new QAction (tr ("Regexp matcher..."),
				parent);
		ActionRegexpMatcher_->setObjectName ("ActionRegexpMatcher_");
		ActionRegexpMatcher_->setProperty ("ActionIcon", "view-filter");

		ActionImportOPML_ = new QAction (tr ("Import from OPML..."),
				parent);
		ActionImportOPML_->setObjectName ("ActionImportOPML_");
		ActionImportOPML_->setProperty ("ActionIcon", "document-import");

		ActionExportOPML_ = new QAction (tr ("Export to OPML..."),
				parent);
		ActionExportOPML_->setObjectName ("ActionExportOPML_");
		ActionExportOPML_->setProperty ("ActionIcon", "document-export");

		ActionImportBinary_ = new QAction (tr ("Import from binary..."),
				parent);
		ActionImportBinary_->setObjectName ("ActionImportBinary_");
		ActionImportBinary_->setProperty ("ActionIcon", "svn-update");

		ActionExportBinary_ = new QAction (tr ("Export to binary..."),
				parent);
		ActionExportBinary_->setObjectName ("ActionExportBinary_");
		ActionExportBinary_->setProperty ("ActionIcon", "svn-commit");

		ActionExportFB2_ = new QAction (tr ("Export to FB2..."),
				parent);
		ActionExportFB2_->setObjectName ("ActionExportFB2_");
		ActionExportFB2_->setProperty ("ActionIcon", "application-xml");

		ActionMarkAllAsRead_ = new QAction (tr ("Mark all channels as read"),
				parent);
		ActionMarkAllAsRead_->setObjectName ("ActionMarkAllAsRead_");
		ActionMarkAllAsRead_->setProperty ("ActionIcon", "mail-mark-read");
	}

	void ChannelActions::SetupActionsStruct (QWidget *parent)
	{
		ActionRemoveFeed_ = new QAction (tr ("Remove feed"),
				parent);
		ActionRemoveFeed_->setObjectName ("ActionRemoveFeed_");
		ActionRemoveFeed_->setProperty ("ActionIcon", "list-remove");

		ActionUpdateSelectedFeed_ = new QAction (tr ("Update selected feed"),
				parent);
		ActionUpdateSelectedFeed_->setObjectName ("ActionUpdateSelectedFeed_");
		ActionUpdateSelectedFeed_->setProperty ("ActionIcon", "view-refresh");

		ActionMarkChannelAsRead_ = new QAction (tr ("Mark channel as read"),
				parent);
		ActionMarkChannelAsRead_->setObjectName ("ActionMarkChannelAsRead_");
		ActionMarkChannelAsRead_->setProperty ("ActionIcon", "mail-mark-read");

		ActionMarkChannelAsUnread_ = new QAction (tr ("Mark channel as unread"),
				parent);
		ActionMarkChannelAsUnread_->setObjectName ("ActionMarkChannelAsUnread_");
		ActionMarkChannelAsUnread_->setProperty ("ActionIcon", "mail-mark-unread");

		ActionRemoveChannel_ = new QAction (tr ("Remove channel"),
				parent);
		ActionRemoveChannel_->setObjectName ("ActionRemoveChannel_");

		ActionChannelSettings_ = new QAction (tr ("Settings..."),
				parent);
		ActionChannelSettings_->setObjectName ("ActionChannelSettings_");
		ActionChannelSettings_->setProperty ("ActionIcon", "configure");
	}
}
}
