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

#ifndef PLUGINS_AGGREGATOR_PROXYOBJECT_H
#define PLUGINS_AGGREGATOR_PROXYOBJECT_H
#include <QObject>
#include "interfaces/aggregator/iproxyobject.h"

namespace LeechCraft
{
namespace Aggregator
{
	class ProxyObject : public QObject
					  , public IProxyObject
	{
		Q_OBJECT
		Q_INTERFACES (LeechCraft::Aggregator::IProxyObject)
	public:
		ProxyObject (QObject* = 0);

		/** @brief Adds the given feed to the storage.
		 *
		 * This method adds the given feed, all its channels and items
		 * to the storage. The storage isn't checked whether it already
		 * contains this feed.
		 *
		 * If the feed's FeedID_ member is set to a non-zero value, that
		 * value is used as the feed's ID. In this case it's your duty
		 * to make sure that this is a correct ID. Otherwise, if
		 * Feed::FeedID_ is 0, a suitable ID is generated and used, and
		 * the feed is updated accordingly. This way one could know the
		 * assigned ID.
		 *
		 * This function may throw.
		 *
		 * @param[in,out] feed The feed to add.
		 *
		 * @exception std::exception
		 */
		void AddFeed (Feed_ptr feed);

		/** @brief Adds the given channel to the storage.
		 *
		 * This method adds the given channel and all its items to the
		 * storage. The storage isn't checked whether it already
		 * contains this channel.
		 *
		 * It's your duty to ensure that the feed mentioned as the
		 * channel's parent feed actually exists.
		 *
		 * The same applies to the channel's ChannelID_ as for FeedID_
		 * in AddFeed().
		 *
		 * This function may throw.
		 *
		 * @param[in,out] channel The channel to add.
		 *
		 * @exception std::exception
		 */
		void AddChannel (Channel_ptr channel);

		/** @brief Adds the given item to the storage.
		 *
		 * This method adds the given item to the storage. The storage
		 * isn't checked whether it already contains this item.
		 *
		 * It's your duty to ensure that the channel mentioned as the
		 * item's parent channel actually exists.
		 *
		 * The same applies to the item's ItemID_ as for FeedID_ in
		 * AddFeed().
		 *
		 * This function may throw.
		 *
		 * @param[in,out] item The item to add.
		 *
		 * @exception std::exception
		 */
		void AddItem (Item_ptr item);

		QList<Channel_ptr> GetAllChannels () const;
		int CountUnreadItems (IDType_t) const;
		QList<Item_ptr> GetChannelItems (IDType_t) const;
	};
}
}

#endif
