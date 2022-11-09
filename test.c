#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdint.h>
#include <sys/ioctl.h>

#include "phy_alloc.h"

#define BUFF_SIZE 1024

ST_PHY_ALLOC gstPhyAlloc;

bool FreePhyMemory ()
{
  int fd;

  if ( (fd = open (DEVICE_NAME_PATH, O_RDWR)) == -1) {
    printf ("Open device node return fail.\n");
    return false;
  }

  ioctl (fd, IOCTL_FREE_MEMORY, &gstPhyAlloc);
  close (fd);
  return true;
}

bool AllocatePhyMemory (unsigned int Size)
{
  int fd, ret;
  int status = true;

  if ( (fd = open (DEVICE_NAME_PATH, O_RDWR)) < 0) {
    printf ("Open device node return fail.\n");
    return false;
  }

  gstPhyAlloc.Size = Size;
  printf ("Size=0x%x\n", Size);

  if ( (ret = ioctl (fd, IOCTL_ALLOCATE_MEMORY, &gstPhyAlloc)) != 0) {
    printf ("IO control of AllocatePhyMemory return fail(%d). errno=%d\n", ret, errno);
    status = false;
  }
  
  gstPhyAlloc.pBuffer = malloc(Size);
  if (gstPhyAlloc.pBuffer != NULL) {
    memset(gstPhyAlloc.pBuffer, 0, Size);
  }
  else {
    status = false;
  }

  close (fd);
  return status;
}

bool ReadPhyMemory ()
{
  int fd, ret;
  int status = true;

  if ( (fd = open (DEVICE_NAME_PATH, O_RDWR)) == -1) {
    printf ("Open device node return fail.\n");
    return false;
  }

  if ( (ret = ioctl (fd, IOCTL_READ_MEMORY, &gstPhyAlloc)) != 0) {
    printf ("IO control of ReadPhyMemory return fail. (%d)\n", ret);
    status = false;
  }

  close (fd);
  return status;
}

bool WritePhyMemory ()
{
  int fd, ret;
  int status = true;

  if ( (fd = open (DEVICE_NAME_PATH, O_RDWR)) == -1) {
    printf ("Open device node return fail.\n");
    return false;
  }

  if ( (ret = ioctl (fd, IOCTL_WRITE_MEMORY, &gstPhyAlloc)) != 0) {
    printf ("IO control of WritePhyMemory return fail. (%d)\n", ret);
    status = false;
  }

  close (fd);
  return status;
}


void DisplayDriverOperateStructure (void)
{
  printf ("Size = 0x%x, physical addr = 0x%llx\n", gstPhyAlloc.Size, gstPhyAlloc.PhysicalAddress);
}

void checkVersion()
{
  int fd = 0, ret;
  unsigned int version = 0;

  if ( (fd = open (DEVICE_NAME_PATH, O_RDWR)) == -1) {
    printf ("Open device node return fail.\n");
    return;
  }

  if ( (ret = ioctl (fd, IOCTL_READ_VERSION, &version)) != 0) {
    printf ("IO control of GetVersion return fail. (%d)\n", ret);
  }
  
  if (version < VERSION_NUMBER_HEX) {
    printf ("Driver version is incorrect expect 0x%X, get 0x%X\n", VERSION_NUMBER_HEX, version);
  }

  close (fd);
}

int main (void)
{
  unsigned int Count = 0;
  unsigned int TestBufSize = 0xf;

  memset (&gstPhyAlloc, 0, sizeof (ST_PHY_ALLOC));

  checkVersion();

  if (AllocatePhyMemory (TestBufSize) == false) {
    printf ("AllocatePhyMemory() fail\n");
    DisplayDriverOperateStructure();
    return -1;
  }

  // Allocate physical and check return code.
  DisplayDriverOperateStructure();
  
  // Fill 0x01 to buffer and verify it.
  memset (gstPhyAlloc.pBuffer, 1, TestBufSize);
  WritePhyMemory ();
  memset(gstPhyAlloc.pBuffer, 0, TestBufSize);
  ReadPhyMemory ();
  printf ("Print physical memory after fill 0x01 in physical memory,\n");

  for (Count = 0; Count < TestBufSize; Count++) {
    printf ("0x%02x ", gstPhyAlloc.pBuffer[Count]);

    if (gstPhyAlloc.pBuffer[Count] != 1) {
      printf ("WritePhyMemory() fail\n");
      return -1;
    }
  }

  printf ("\n\n");
  FreePhyMemory();
  return 0;
}
