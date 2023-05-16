/*
 * Physical memory allocate Driver
 * File: phy_alloc.c
 *
 * Copyright (C) 2013 - 2015 Insyde Software Corp.
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
#include "phy_alloc.h"
#ifdef __FreeBSD__
#include "bsd/drv.h"
#else
#include "linux/drv.h"
#endif

static unsigned int gAllocatedQuantity = 0;

unsigned int t_dwEAX;
unsigned int t_dwEBX;
unsigned int t_dwECX;
unsigned int t_dwEDX;
unsigned int t_dwEDI;
unsigned int t_dwESI;

#pragma pack(1)

//
// Internal record list for allocate physcial detail information.
//
ST_OBJ *gpObjList = NULL;

#pragma pack()

//
// Allocate physical memory
//
static int Alloc_Physical_Memory (unsigned long arg)
{
  ST_OBJ *pstObj = NULL;
  ST_OBJ *pLastObj = NULL;
  ST_PHY_ALLOC stPhyAlloc = {0};
  ST_PHY_ALLOC *pstPhyAlloc = &stPhyAlloc;
  unsigned char *pBuffer = NULL;
  unsigned int Index = 0;

  if (getargs (pstPhyAlloc, (void *) arg, sizeof (ST_PHY_ALLOC))) {
    return DRV_FAILED;
  }

  KDBG ("pstPhyAlloc->Size=0x%x\n", pstPhyAlloc->Size);

  // Allocate size must beigger then 0.
  if(!pstPhyAlloc->Size) 
    return ARGUMENT_FAIL;

  // Allocate physical memory.
  pBuffer = (void *) DrvKmalloc(pstPhyAlloc->Size);
  if(!pBuffer) {
    // Allocate physical memory return fail.
    KDBG ("Alloc buffer failed\n");
    return ALLOCATE_FAIL;
  }
  memset (pBuffer, 0, pstPhyAlloc->Size);

  // Add a new record into internal record list
  if(gpObjList==NULL) { // First record
    KDBG ("Allocate root list\n");
    gpObjList = vmalloc(sizeof(ST_OBJ));
    if (!gpObjList) {
      DrvKFree (pBuffer, pstPhyAlloc->Size);
      KDBG ("Allocate root list failed\n");
      return ALLOCATE_FAIL;
    }
    memset(gpObjList, 0, sizeof(ST_OBJ));
    pstObj=gpObjList;
  } else { // Add a new record follow record list
    pstObj=gpObjList;
    while(pstObj->pNext) {
      pstObj=pstObj->pNext;
      if (pstObj->Index>=Index) {
        Index=pstObj->Index;
      }
    }
    Index++;
    KDBG ("Allocate next node\n");
    pstObj->pNext=(ST_OBJ*)vmalloc(sizeof(ST_OBJ));
    if (!pstObj->pNext) {
      DrvKFree (pBuffer, pstPhyAlloc->Size);
      KDBG ("Allocate node failed\n");
      return ALLOCATE_FAIL;
    }
    memset(pstObj->pNext, 0, sizeof(ST_OBJ));
    pLastObj = pstObj;
    pstObj=pstObj->pNext;
  }

  KDBG ("Update node...\n");
    // Update record informations
  pstObj->pLast = pLastObj;
  pstObj->pBuffer = pBuffer;
  pstObj->KernelLogicalAddress = virt_to_phys(pstObj->pBuffer);
  pstObj->KernelVirtualAddress = (unsigned long)pstObj->pBuffer;
  pstObj->Index = Index;
  pstObj->Size = pstPhyAlloc->Size;
  KDBG("Allocate Buffer = 0x%llx\n", (unsigned long long)pBuffer);

  // Update information for user space application
  pstPhyAlloc->VirtualAddress  = pstObj->KernelVirtualAddress;
  pstPhyAlloc->PhysicalAddress = pstObj->KernelLogicalAddress;
  pstPhyAlloc->Index = pstObj->Index;

  if (setargs ((void *) arg, pstPhyAlloc, sizeof (ST_PHY_ALLOC))) {
    return DRV_FAILED;
  }

  gAllocatedQuantity++;
  return DRV_SUCCESS;
}


//
// Release allocated physical memory
//
static int Free_Physical_Memory (unsigned long arg)
{
  ST_OBJ *pstObj = NULL;
  ST_OBJ *pLastObj = NULL;
  ST_OBJ *pNextObj = NULL;
  ST_PHY_ALLOC stPhyAlloc = {0};
  ST_PHY_ALLOC *pstPhyAlloc = &stPhyAlloc;

  if (getargs (pstPhyAlloc, (void *) arg, sizeof (ST_PHY_ALLOC))) {
    return DRV_FAILED;
  }

  if(gpObjList) {
    pstObj=gpObjList;
    while(true) {
      if(pstObj->KernelLogicalAddress==pstPhyAlloc->PhysicalAddress) {
        if ((pstObj->pNext==NULL) && (pstObj->pLast==NULL)) { /* Only one record */
          gpObjList = NULL;
        } else if ((pstObj->pNext!=NULL) && (pstObj->pLast!=NULL)) { /* Record  betweet head and last */
          pLastObj = pstObj->pLast;
          pNextObj = pstObj->pNext;
          pLastObj->pNext = pNextObj;
          pNextObj->pLast = pLastObj;
        } else if (pstObj->pNext==NULL) { /* Last record */
          pLastObj = pstObj->pLast;
          pLastObj->pNext = NULL;
        } else if (pstObj->pLast==NULL) { /* First record */
          gpObjList = pstObj->pNext;
        }

        DrvFreePage (pstObj->pBuffer, pstPhyAlloc->Size);
        vfree(pstObj);
        gAllocatedQuantity--;
        return DRV_SUCCESS;
      } else if (pstObj->pNext) {
        pstObj = pstObj->pNext;
      } else {
        return ALLOCATE_FAIL;
      }
    }   
  }

  return ALLOCATE_FAIL;
}


//
// Read physical memory to virtual memory in user space
//
static int Read_Physical_Memory (unsigned long arg)
{
  ST_OBJ *pstObj = NULL;
  ST_PHY_ALLOC stPhyAlloc = {0};
  ST_PHY_ALLOC *pstPhyAlloc = &stPhyAlloc;

  if (getargs (pstPhyAlloc, (void *) arg, sizeof (ST_PHY_ALLOC))) {
    return DRV_FAILED;
  }

  if (pstPhyAlloc->pBuffer==NULL) {
    return ALLOCATE_FAIL;
  }

  if(gpObjList) {
    pstObj=gpObjList;
    while(true) {
      if (pstPhyAlloc->Index==pstObj->Index) {
        if (copy_to_user (pstPhyAlloc->pBuffer, pstObj->pBuffer, pstObj->Size)) {
          return DRV_FAILED;
        }
        return DRV_SUCCESS;
      } else if (pstObj->pNext) {
        pstObj = pstObj->pNext;
      } else {
        return ALLOCATE_FAIL;
      }
    }   
  }

  return ALLOCATE_FAIL;
}


//
// Write physical memory from virtual memory in user space
//
static int Write_Physical_Memory (const unsigned long arg)
{
  ST_OBJ *pstObj = NULL;
  ST_PHY_ALLOC stPhyAlloc = {0};
  ST_PHY_ALLOC *pstPhyAlloc = &stPhyAlloc;

  if (getargs (pstPhyAlloc, (void *) arg, sizeof (ST_PHY_ALLOC))) {
    return DRV_FAILED;
  }

  KDBG("Write Source Buffer = 0x%llx\n", (unsigned long long)pstPhyAlloc->pBuffer);
  KDBG("Write Size = 0x%llx\n", (unsigned long long)pstPhyAlloc->Size);

  if (pstPhyAlloc->pBuffer==NULL) {
    return ALLOCATE_FAIL;
  }

  if(gpObjList) {
    pstObj=gpObjList;
    while(true) {
      if (pstPhyAlloc->Index==pstObj->Index) {
        KDBG("Found Buffer = 0x%llx\n", (unsigned long long)pstObj->pBuffer);
        if (copy_from_user (pstObj->pBuffer, pstPhyAlloc->pBuffer, pstObj->Size)) {
          return DRV_FAILED;
        }
        return DRV_SUCCESS;
      } else if (pstObj->pNext) {
        pstObj = pstObj->pNext;
      } else {
        return ALLOCATE_FAIL;
      }
    }   
  }

  return ALLOCATE_FAIL;
}


static int Version (unsigned int* ptr)
{
  unsigned int version = VERSION_NUMBER_HEX;

  if (!ptr) {
    KDBG ("Version parameter wrong\n");
    return ARGUMENT_FAIL;
  }

  if (setargs (ptr, &version, sizeof (unsigned int))) {
    return DRV_FAILED;
  }

  return DRV_SUCCESS;
}


static int AllocatedQuantity (unsigned int* ptr)
{
  if (setargs (ptr, &gAllocatedQuantity, sizeof (unsigned int))) {
    return DRV_FAILED;
  }

  return DRV_SUCCESS;
}

static int DoIn(DRV_IO *io)
{
  switch (io->size) {
    case 1:
      io->data = inb(io->port) & 0xFF;
      break;
    case 2:
      io->data = inw(io->port) & 0xFFFF;
      break;
    case 4:
      io->data = inl(io->port) & 0xFFFFFFFFUL;
      break;
    case 8: {
      uint32_t *p = (uint32_t*)&io->data;
      p[0] = inl(io->port + 0);
      p[1] = inl(io->port + 4);
      break;
    }
  }
  return DRV_SUCCESS;
}

static int DoOut(DRV_IO *io)
{
  switch (io->size) {
    case 1:
      outb((uint8_t)(io->data & 0xFF), io->port);
      break;
    case 2:
      outw((uint16_t)(io->data & 0xFFFF), io->port);
      break;
    case 4:
      outl((uint32_t)(io->data & 0xFFFFFFFFUL), io->port);
      break;
    case 8: {
      uint32_t *p = (uint32_t*)&io->data;
      outl(p[0], io->port);
      outl(p[1], io->port + 4);
      break;
    }
  }
  return DRV_SUCCESS;
}

static int DoIO (unsigned int* ptr)
{
  DRV_IO io = {0};
  int status = DRV_SUCCESS;
  if (getargs (&io, ptr, sizeof (DRV_IO))) {
    return DRV_FAILED;
  }

  KDBG ("Do IO port = %X data = %lX, size = %d, mode = %d\n", io.port, io.data, io.size, io.mode);
  switch (io.mode) {
    case IO_MODE_READ:
      status = DoIn(&io);
      break;
    case IO_MODE_WRITE:
      status = DoOut(&io);
      break;
  }
  KDBG ("Do IO port = %X data = %lX, size = %d, mode = %d\n", io.port, io.data, io.size, io.mode);
  
  if (setargs (ptr, &io, sizeof (DRV_IO))) {
    return DRV_FAILED;
  }

  return status;
}

static void TriggerSmi(SMI_REGISTER* reg) {
	KDBG ("EAX: 0x%x\n", reg->dwEAX);
	KDBG ("EBX: 0x%x\n", reg->dwEBX);
	KDBG ("ECX: 0x%x\n", reg->dwECX);
	KDBG ("EDX: 0x%x\n", reg->dwEDX);
	KDBG ("ESI: 0x%x\n", reg->dwESI);
	KDBG ("EDI: 0x%x\n", reg->dwEDI);

	t_dwEAX = reg->dwEAX;
	t_dwEBX = reg->dwEBX;
	t_dwECX = reg->dwECX;
	t_dwEDX = reg->dwEDX;
	t_dwESI = reg->dwESI;
	t_dwEDI = reg->dwEDI;

	__asm__
	(
#ifdef __x86_64__ // for gcc
		"push   %rax    \n"
		"push   %rbx    \n"
		"push   %rcx    \n"
		"push   %rdx    \n"
		"push   %rdi    \n"
		"push   %rsi    \n"
		"xor    %rax,       %rax     \n"
		"xor    %rbx,       %rbx     \n"
#else
		"push   %eax    \n"
		"push   %ebx    \n"
		"push   %ecx    \n"
		"push   %edx    \n"
		"push   %edi    \n"
		"push   %esi    \n"
		"xor    %eax,       %eax     \n"
		"xor    %ebx,       %ebx     \n"
#endif

		"mov    t_dwEAX,    %eax    \n"
		"mov    t_dwEBX,    %ebx    \n"
		"mov    t_dwECX,    %ecx    \n"
		"mov    t_dwEDX,    %edx    \n"
		"mov    t_dwESI,    %esi    \n"
		"mov    t_dwEDI,    %edi    \n"
		"out    %al,        %dx     \n"

		"mov    %eax,       t_dwEAX  \n"
		"mov    %ebx,       t_dwEBX  \n"
		"mov    %ecx,       t_dwECX  \n"
		"mov    %edx,       t_dwEDX  \n"
		"mov    %esi,       t_dwESI  \n"
		"mov    %edi,       t_dwEDI  \n"

#ifdef __x86_64__ // for gcc
		"pop    %rsi    \n"
		"pop    %rdi    \n"
		"pop    %rdx    \n"
		"pop    %rcx    \n"
		"pop    %rbx    \n"
		"pop    %rax    \n"
#else
		"pop    %esi    \n"
		"pop    %edi    \n"
		"pop    %edx    \n"
		"pop    %ecx    \n"
		"pop    %ebx    \n"
		"pop    %eax    \n"
#endif
	);

    // DrvSleep(500);

	reg->dwEAX = t_dwEAX;
	reg->dwEBX = t_dwEBX;
	reg->dwECX = t_dwECX;
	reg->dwEDX = t_dwEDX;
	reg->dwESI = t_dwESI;
	reg->dwEDI = t_dwEDI;
}

static int SMI (unsigned char* arg)
{
	SMI_REGISTER SmiReg = {0};
	SMI_REGISTER* reg = &SmiReg;

	if (getargs (reg, arg, sizeof (SMI_REGISTER))) {
		return 1;
	}

	TriggerSmi(reg);

	if (setargs (arg, reg, sizeof (SMI_REGISTER))) {
		KDBG (KERN_WARNING "Copy Data back to user failed\n");
		return 1;
	}

	KDBG ("Result: 0x%x\n", t_dwEAX);

	return 0;
}

static inline int DrvIoctl(unsigned int cmd, unsigned long arg)
{
  long Ret = 0;

  KDBG ("num=0x%X\n", cmd);

  switch (cmd) {
    case IOCTL_ALLOCATE_MEMORY:
      Ret = Alloc_Physical_Memory ((unsigned long)arg);
      break;

    case IOCTL_FREE_MEMORY:
      Ret = Free_Physical_Memory ((unsigned long)arg);
      break;

    case IOCTL_WRITE_MEMORY:
      Ret = Write_Physical_Memory ((unsigned long)arg);
      break;

    case IOCTL_READ_MEMORY:
      Ret = Read_Physical_Memory ( (unsigned long)arg);
      break;

    case IOCTL_GET_ALLOCATED_QUENTITY:
      Ret = AllocatedQuantity ( (unsigned int *) arg);
      break;

    case IOCTL_IO:
      Ret = DoIO ( (unsigned int *) arg);
      break;

    case IOCTL_SMI:
      Ret = SMI ( (unsigned char *) arg);
      break;

    case IOCTL_READ_VERSION:
      Ret = Version ( (unsigned int *) arg);
      break;

    case IOCTL_TEST:
      break;

    default:
      KDBG ("Unsupported!\n");
      Ret = -1;
      break;
  }

  return Ret;
}
