/*
 * Physical memory allocate Driver
 * File: phy_alloc.h
 *
 * Copyright (C) 2013 - 2020 Insyde Software Corp.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef _PHY_ALLOC_H_
#define _PHY_ALLOC_H_

#ifdef __FreeBSD__
#include <sys/ioccom.h>
#include <sys/stdint.h>
#endif

#ifdef OS_LINUX
#include <stdint.h>
#elif !defined(__FreeBSD__)
#include <linux/types.h>
#endif

#define DEVICE_NAME             "phy_alloc"                  //InsydeFlash buffer
#define DEVICE_NAME_PATH        "/dev/"DEVICE_NAME      // Open device node on user-space
#define DEVICE_MAJOR_NUM        100

#pragma pack(1)
typedef struct _ST_PHY_ALLOC
{
	unsigned int Index;
	unsigned int Size;
	unsigned long long PhysicalAddress;
	unsigned long long VirtualAddress;
	union
	{
		unsigned long long padding;
		unsigned char *pBuffer;
	};
} __attribute__ ((aligned (8))) ST_PHY_ALLOC;

typedef struct {
	unsigned int dwESI;
	unsigned int dwEDI;
	unsigned int dwECX;
	unsigned int dwEDX;
	unsigned int dwEAX;
	unsigned int dwEBX;
} __attribute__ ((aligned (8))) SMI_REGISTER;

typedef struct _DRV_IO
{
	uint16_t port;
	uint64_t data;
	uint8_t size;
	uint8_t mode;
} __attribute__ ((aligned (8))) DRV_IO;

enum {
	IO_MODE_READ = 0,
	IO_MODE_WRITE
};

#pragma pack()

// ioctl
#define IOCTL_ALLOCATE_MEMORY                _IOWR(DEVICE_MAJOR_NUM, 1, ST_PHY_ALLOC)
#define IOCTL_FREE_MEMORY                    _IOWR(DEVICE_MAJOR_NUM, 2, ST_PHY_ALLOC)
#define IOCTL_WRITE_MEMORY                   _IOWR(DEVICE_MAJOR_NUM, 3, ST_PHY_ALLOC)
#define IOCTL_READ_MEMORY                    _IOWR(DEVICE_MAJOR_NUM, 4, ST_PHY_ALLOC)
#define IOCTL_READ_VERSION                   _IOR(DEVICE_MAJOR_NUM, 5, unsigned int)
#define IOCTL_GET_ALLOCATED_QUENTITY         _IOR(DEVICE_MAJOR_NUM, 6, unsigned int)
#define IOCTL_SMI                            _IOWR(DEVICE_MAJOR_NUM, 7, SMI_REGISTER)
#define IOCTL_IO                             _IOWR(DEVICE_MAJOR_NUM, 8, DRV_IO)

// return
#define DRV_SUCCESS 0
#define NOT_ALLOCATE_MEMORY -1
#define ALLOCATE_MEMORY_EXISTING -2
#define ALLOCATE_FAIL -3
#define DRV_BE_USED -4
#define DRV_INITIAL_FAIL -5
#define ARGUMENT_FAIL -6
#define DRV_FAILED -7

#define VERSION_NUMBER_HEX  0x00000009

#endif 
