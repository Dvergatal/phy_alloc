#ifndef __DRV_DEFINE_H__
#define __DRV_DEFINE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>
#include <sys/module.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/uio.h>
#include <sys/conf.h>
#include <sys/malloc.h>
#include <vm/vm.h>
#include <vm/pmap.h>
#include <sys/limits.h>
#include <sys/mutex.h>

#include <x86/include/x86_var.h>

#include "../phy_alloc.h"

#ifdef __DEBUG_MODE__
#define printk printf
#define KERN_WARNING "Warning:"
#define KERN_DEBUG "Debug:"
#define KERN_ERR "Error:"
#define KERN_INFO "Info:"
#define KERN_NOTICE "Notice:"
#define KDBG(m,...)      printk (KERN_DEBUG m, ##__VA_ARGS__)
#else
#define KDBG(m,...)
#endif

#pragma pack(1)

typedef struct _st_obj ST_OBJ;
struct _st_obj {
  ST_OBJ *pNext; // Next record, if "null" that's end of records.
  ST_OBJ *pLast; // Last record, if "null" that's first of records.
  unsigned int Index; // Allocate memory index for check and search with user mode information.
  unsigned long Size; // Allocated physical memory size
  unsigned long long KernelVirtualAddress; // Allocated physical memory virtual address in kernel space
  unsigned long long KernelLogicalAddress; // Allocated physical memory physical address in kernel space
  unsigned char *pBuffer; // Virtual address for user space allocated memory
};
extern ST_OBJ *gpObjList;
static struct mtx race_mtx;

#pragma pack()

static d_ioctl_t drv_ioctl;

static inline unsigned long
copy_from_user (void *to, void *from, unsigned long n);
static inline unsigned long
copy_to_user (void *to, void *from, unsigned long n);
static inline void *vmalloc(unsigned long size);
static inline void vfree(void *addr);
static inline vm_paddr_t virt_to_phys(void *va);
static inline void *kmalloc (size_t n, size_t size);
static inline void kfree (void *addr, size_t size);
static void cleanup(void);

MALLOC_DECLARE(M_STOBJ);
MALLOC_DEFINE(M_STOBJ, "StObj", "Kernel memory buffer");

static inline int DrvIoctl(unsigned int cmd, unsigned long arg);

static inline void* DrvKmalloc(size_t size)
{
  return kmalloc(size, 1);
}

static inline void DrvKFree(void *addr, size_t size)
{
  kfree(addr, size);
}

static inline void DrvFreePage(void *addr, size_t size)
{
  DrvKFree(addr, size);
}

static inline void DrvSleep(unsigned long ms)
{
  pause("WaitIhisi", ms / 50);
}

static int
drv_ioctl (struct cdev *dev, u_long cmd, caddr_t arg, int fflag, struct thread *td)
{
  mtx_lock(&race_mtx);
  int retn = DrvIoctl((unsigned int)cmd, (unsigned long)arg);
  mtx_unlock(&race_mtx);
  return retn;
}

// translate function from linux to unix

inline unsigned long copy_from_user (void *to, void *from, unsigned long n)
{
  return copyin(from, to, n);
}

inline unsigned long copy_to_user (void *to, void *from, unsigned long n)
{
  return copyout(from, to, n);
}

inline void *vmalloc (unsigned long size)
{
  return malloc(size, M_STOBJ, M_WAITOK);
}

inline void vfree (void *addr)
{
  free(addr, M_STOBJ);
}

static inline vm_paddr_t virt_to_phys(void *va)
{
  uintptr_t v = (uintptr_t)va;
  return (vtophys(v & ~PAGE_MASK)+(v&PAGE_MASK));
}

static inline void *kmalloc (size_t n, size_t size)
{
  if (size != 0 && n > ULONG_MAX / size)
    return NULL;

  return contigmalloc (n * size, M_STOBJ, M_WAITOK | M_ZERO, 0ul,
                       ~0ul, PAGE_SIZE, 0);
}

static inline void kfree (void *addr, size_t size)
{
  contigfree(addr, size, M_STOBJ);
}

inline unsigned long getargs (void *to, void *from, unsigned long n)
{
  memcpy(to, from, n);
  return 0;
}

inline unsigned long setargs (void *to, void *from, unsigned long n)
{
  memcpy(to, from, n);
  return 0;
}

/*
 * allow user processes to MMAP some memory sections
 * instead of going through read/write
 */
/* ARGSUSED */
static int drv_memmmap(struct cdev *dev, vm_ooffset_t offset, vm_paddr_t *paddr,
    int prot __unused, vm_memattr_t *memattr __unused)
{
      if (offset > cpu_getmaxphyaddr())
          return (-1);
      *paddr = offset;
      return (0);
}

// Module Init

static void cleanup(void)
{
  ST_OBJ *pObj = NULL;
  ST_OBJ *pNext = NULL;
  int    iOrder = -1;

  if (gpObjList) {
    pObj = gpObjList;
    while(true) {
      pNext = pObj->pNext;
      while ( (1 << ++iOrder)* PAGE_SIZE < pObj->Size);
      kfree (pObj->pBuffer, pObj->Size);
      vfree(pObj);
      if (pNext==NULL) {
        break;
      } else {
        pObj = pNext;
      }
    }
    gpObjList = NULL;
  }
}

static struct cdev *drv_dev;

static struct cdevsw drv_cdevsw = {
  .d_version = D_VERSION,
  .d_name = DEVICE_NAME,
  .d_ioctl = drv_ioctl,
  .d_mmap = drv_memmmap
};

static int
drv_loader (struct module *m, int num, void *arg)
{
  long Ret = 0;

  KDBG ("num=0x%x\n", num);

  switch (num) {
    case MOD_LOAD:
      mtx_init(&race_mtx, "race config lock", NULL, MTX_DEF);
      drv_dev = make_dev (&drv_cdevsw, 0, UID_ROOT, GID_WHEEL, 0644, "phy_alloc");
      break;

    case MOD_UNLOAD:
      destroy_dev(drv_dev);
      mtx_lock(&race_mtx);
      cleanup();
      mtx_unlock(&race_mtx);
      mtx_destroy(&race_mtx);
      break;
  }

  return Ret;
}

static moduledata_t drv_mod = {
  "phy_alloc",
  drv_loader,
  NULL
};

DECLARE_MODULE (phy_alloc, drv_mod, SI_SUB_KLD, SI_ORDER_ANY);

#ifdef __cplusplus
}
#endif

#endif
