#ifndef  FS_DEV_NOR_MMAP_IMAGE_PRESENT
#define  FS_DEV_NOR_MMAP_IMAGE_PRESENT


#ifdef   FS_DEV_NOR_MMAP_MODULE
#define  FS_DEV_NOR_MMAP_MODULE
#else
#define  FS_DEV_NOR_MMAP_MODULE  extern
#endif



#include  <fs_dev.h>
#include  <NOR/fs_dev_nor.h>


extern  const  FS_DEV_NOR_PHY_API  FSDev_NOR_MMAP;


#endif                                                          /* End of NOR #### module include.                      */
