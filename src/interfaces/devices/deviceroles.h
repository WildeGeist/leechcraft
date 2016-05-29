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

#include <Qt>
#include "devicetypes.h"

namespace LeechCraft
{
	/** @brief Describes the various common partition types.
	 *
	 * This enum is used for USB Mass Storage devices and similar ones
	 * where the concept of a partition makes sense.
	 *
	 * @sa MassStorageRole::PartType
	 */
	enum PartitionType
	{
		/** @brief Something other than a partition.
		 */
		NonPartition = -1,

		/** @brief Empty partition without a type.
		 */
		Empty = 0x00,

		/** @brief FAT32 partition.
		 */
		Win95FAT32 = 0x0b,

		/** @brief FAT32 partition with LBA.
		 */
		Win95FAT32LBA = 0x0c
	};

	/** @brief Roles for both USB Mass Storage and generic USB devices.
	 */
	enum CommonDevRole
	{
		/** @brief The type of the device.
		 *
		 * This role is expected to contain a member of the DeviceType enum.
		 *
		 * @sa DeviceType
		 */
		DevType = Qt::UserRole + 1,

		/** @brief The unique device ID (QString).
		 */
		DevID,

		/** @brief The persistent unique device ID (QString).
		 */
		DevPersistentID,

		CommonDevRoleMax
	};

	/** @brief Roles specific to generic USB devices.
	 *
	 * The corresponding CommonDevRole::DevType is DeviceType::USBDevice.
	 */
	enum USBDeviceRole
	{
		/** @brief The general USB ID of the role (QByteArray).
		 */
		ID = CommonDevRole::CommonDevRoleMax + 1,

		/** @brief The bus this device is attached to (int).
		 */
		Busnum,

		/** @brief The device number on the given bus (int).
		 */
		Devnum,

		/** @brief The ID of the vendor (QString).
		 */
		VendorID,

		/** @brief The human-readable name of the vendor (QString).
		 */
		Vendor,

		/** @brief The ID of the model (QString).
		 */
		ModelID,

		/** @brief The human-readable name of the device model (QString).
		 */
		Model,

		/** @brief The system file representing the device (QString).
		 *
		 * This role should contain the system file path representing the
		 * device, if applicable.
		 */
		SysFile,

		USBDeviceRoleMax
	};

	/** @brief Roles specific to mass storage USB devices.
	 *
	 * The corresponding CommonDevRole::DevType is DeviceType::MassStorage.
	 */
	enum MassStorageRole
	{
		/** @brief The device file representing the device (QString).
		 *
		 * For example, it could be \em{/dev/sdc1} on a Linux system.
		 */
		DevFile = USBDeviceRole::USBDeviceRoleMax + 1,

		/** @brief The type of the partition.
		 *
		 * This role is expected to contain a member of the
		 * PartitionType enum.
		 *
		 * @sa PartitionType
		 */
		PartType,

		/** @brief Whether this item is removable (bool).
		 */
		IsRemovable,

		/** @brief Whether this item is a partition (bool).
		 */
		IsPartition,

		/** @brief Whether this item could be mounted (bool).
		 */
		IsMountable,

		/** @brief Whether this item is currently mounted (bool).
		 */
		IsMounted,

		/** @brief Whether this item contains media (bool).
		 *
		 * For example, a CD in a CD-ROM.
		 */
		IsMediaAvailable,

		/** @brief Human-readable name of the device (QString).
		 */
		VisibleName,

		/** @brief Available size in bytes (qint64).
		 *
		 * If the value is -1, the available size is not known. It is
		 * very likely the available size wouldn't be known for unmounted
		 * devices.
		 */
		AvailableSize,

		/** @brief Total size in bytes (qint64).
		 *
		 * If the value is -1, the available size is not known. It is
		 * very likely the total size wouldn't be known for unmounted
		 * devices.
		 */
		TotalSize,

		/** @brief The list of directories this item is mounted to
		 * (QStringList).
		 */
		MountPoints,

		MassStorageRoleMax
	};
}
