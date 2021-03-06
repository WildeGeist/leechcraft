/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2014  Georg Rudoy
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE or copy at https://www.boost.org/LICENSE_1_0.txt)
 **********************************************************************/

#pragma once

#include <QObject>
#include "interfaces/core/ientitymanager.h"

namespace LC
{
	class EntityManager : public QObject
						, public IEntityManager
	{
		Q_OBJECT
		Q_INTERFACES (IEntityManager)
	public:
		explicit EntityManager (QObject* = nullptr);

		DelegationResult DelegateEntity (Entity, QObject* = nullptr) override;
		Q_INVOKABLE bool HandleEntity (LC::Entity, QObject* = nullptr) override;
		Q_INVOKABLE bool CouldHandle (const LC::Entity&) override;
		QList<QObject*> GetPossibleHandlers (const Entity&) override;
	};
}
