#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/ioctl.h>
#include <unistd.h>
#include <errno.h>

#include "phy_alloc.h"

#define BUFF_SIZE 1024

ST_PHY_ALLOC gstPhyAlloc;

bool CheckFileExisting(const char *pFile)
{
  if (access(pFile, F_OK)==0)
  {
    return true;
  }
  else 
  {
    return false;
  }
}

int ExecuteCommand (const char *pCommand, char *pRetBuf, unsigned int BufSize)
{
    FILE *fp = NULL;
    printf( "ExecuteCommand(\"%s\")\n", pCommand);
    if ((fp = popen (pCommand, "w"))==NULL) {
      printf( "popen() return NULL\n");
        return -1;
    }
    if (fgets (pRetBuf, BufSize, fp)==NULL) {
      printf( "fgets() return NULL\n");
        return -1;
    }
    return pclose(fp);
}

#ifdef IOCTL_CLEAR_MEMORY
bool ClearPhyMemory ()
{          
    int fd;
    if ((fd = open(DEVICE_NAME_PATH, O_RDWR)) == -1)
    {
        printf("Open device node return fail.\n");
        return false;
    }

    ioctl(fd, IOCTL_CLEAR_MEMORY, &gstPhyAlloc);
    close(fd);
    return true;
}
#endif

bool FreePhyMemory ()
{
    int fd;
    if ((fd = open(DEVICE_NAME_PATH, O_RDWR)) == -1)
    {
        printf("Open device node return fail.\n");
        return false;
    }

    ioctl(fd, IOCTL_FREE_MEMORY, &gstPhyAlloc);
    close(fd);
    return true;
}  

bool AllocatePhyMemory(unsigned int Size)
{    
    int fd, ret;
    if ((fd = open(DEVICE_NAME_PATH, O_RDWR)) < 0)
    {
        printf("Open device node return fail.\n");
        return false;
    }

    gstPhyAlloc.Size = Size;
    printf ("Size=0x%x\n", Size);
    if ((ret = ioctl(fd, IOCTL_ALLOCATE_MEMORY, &gstPhyAlloc)) != 0)
    {
        printf("IO control of AllocatePhyMemory return fail(%d). errno=%d\n", ret, errno);
        return false;
    }

    close(fd);
    return true;
}  

bool ReadPhyMemory(void *ptr)
{         
    int fd, ret;
    if ((fd = open(DEVICE_NAME_PATH, O_RDWR)) == -1)
    {
        printf("Open device node return fail.\n");
        return false;
    }

    if ((ret = ioctl(fd, IOCTL_READ_MEMORY, ptr)) != 0)
    {
        printf("IO control of ReadPhyMemory return fail. (%d)\n", ret);
        return false;
    }
    close(fd);
    return true;
}

bool WritePhyMemory(const void *ptr)
{
    int fd, ret;
    if ((fd = open(DEVICE_NAME_PATH, O_RDWR)) == -1)
    {
        printf("Open device node return fail.\n");
        return false;
    }

    if ((ret = ioctl(fd, IOCTL_WRITE_MEMORY, ptr)) != 0)
    {
        printf("IO control of WritePhyMemory return fail. (%d)\n", ret);
        return false;
    }

    close(fd);
    return true;
}


void DisplayDriverOperateStructure(void)
{
  printf("Size = 0x%x, physical addr = 0x%lx\n", gstPhyAlloc.Size, gstPhyAlloc.PhysicalAddress);
}


int main(void)
{
  char CommandBuffer[BUFF_SIZE];
  char CommandResponse[BUFF_SIZE];
  unsigned int Count = 0;

  memset(&gstPhyAlloc, 0, sizeof(ST_PHY_ALLOC));

#if 0
  while (CheckFileExisting(DEVICE_NAME_PATH))
  {
    sprintf(CommandBuffer, "rmmod %s", DEVICE_NAME);
    ExecuteCommand(CommandBuffer, CommandResponse, BUFF_SIZE);
    sleep(2);
    if (Count>5) 
    {
      printf("Rmmod driver fail\n");
      return -1;
    }
    else 
    {
      Count++;
    }
  }

  while (CheckFileExisting(DEVICE_NAME_PATH)==false)
  {
    sprintf(CommandBuffer, "insmod %s.ko", DEVICE_NAME);
    ExecuteCommand(CommandBuffer, CommandResponse, BUFF_SIZE);
    sleep(2);
    if (Count>5) 
    {
      printf("Insmod driver fail\n");
      return -1;
    }
    else 
    {
      Count++;
    }
  }
#endif

  unsigned int TestBufSize = 0xf;
  unsigned char TestBuf[TestBufSize];
  memset(TestBuf, 0xff, TestBufSize);

  // Allocate physical and check return code.
  DisplayDriverOperateStructure();
  if (AllocatePhyMemory(TestBufSize)==false)
  {
    printf("AllocatePhyMemory() fail\n");
    DisplayDriverOperateStructure();
    return -1;  
  }

#ifdef IOCTL_CLEAR_MEMORY
  // Clear physical memory and read it, it should be fill zero.
  ClearPhyMemory();
  ReadPhyMemory(TestBuf);
  for (Count=0; Count<TestBufSize; Count++)
  {
    if (TestBuf[Count]!=0)
    {
      printf("ReadPhyMemory() fail\n");
      return -1;
    }
  }
#endif

  // Fill 0x01 to buffer and verify it.
  memset(TestBuf, 1, TestBufSize);
  WritePhyMemory(TestBuf);
  ReadPhyMemory(TestBuf);
  printf("Print physical memory after fill 0x01 in physical memory,\n");
  for (Count=0; Count<TestBufSize; Count++)
  {
    printf("0x%02x ", TestBuf[Count]);
    if (TestBuf[Count]!=1)
    {
      printf("WritePhyMemory() fail\n");
      return -1;
    }
  }
  printf("\n\n");


  FreePhyMemory();
#if 0
  while (CheckFileExisting(DEVICE_NAME_PATH))
  {
    sprintf(CommandBuffer, "rmmod %s", DEVICE_NAME);
    ExecuteCommand(CommandBuffer, CommandResponse, BUFF_SIZE);
    sleep(2);
    if (Count>5) 
    {
      printf("Rmmod driver fail\n");
      return -1;
    }
    else 
    {
      Count++;
    }
  }
#endif

  return 0;
}
