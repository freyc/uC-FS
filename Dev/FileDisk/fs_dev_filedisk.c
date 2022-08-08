/*
*********************************************************************************************************
*                                                uC/FS
*                                      The Embedded File System
*
*                    Copyright 2008-2021 Silicon Laboratories Inc. www.silabs.com
*
*                                 SPDX-License-Identifier: APACHE-2.0
*
*               This software is subject to an open source license and is distributed by
*                Silicon Laboratories Inc. pursuant to the terms of the Apache License,
*                    Version 2.0 available at www.apache.org/licenses/LICENSE-2.0.
*
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*
*                                      FILE SYSTEM DEVICE DRIVER
*
*                                              TEMPLATE
*
* Filename : fs_dev_template.c
* Version  : V4.08.01
*********************************************************************************************************
* Note(s)  : (a) Replace #### with the driver identifier (in the correct case).
*            (b) Replace $$$$ with code/definitions/etc.
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                               MODULE
*********************************************************************************************************
*/

#define  FS_DEV_FILEDISK_MODULE


/*
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*/

#include  <Source/fs.h>
#include  "fs_dev_filedisk.h"

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>


/*
*********************************************************************************************************
*                                            LOCAL DEFINES
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                          LOCAL DATA TYPES
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                     #### DEVICE DATA DATA TYPE
*********************************************************************************************************
*/

typedef  struct  fs_dev_filedisk_data  FS_DEV_FILEDISK_DATA;
struct  fs_dev_filedisk_data {
    CPU_SIZE_T             SecSize;
    CPU_SIZE_T             SecCnt;
    void*                  Data;

#if (FS_CFG_CTR_STAT_EN == DEF_ENABLED)
    FS_CTR                 StatRdCtr;
    FS_CTR                 StatWrCtr;
#endif

#if (FS_CFG_CTR_ERR_EN == DEF_ENABLED)
   /* $$$$ ERROR COUNTERS */
#endif

    FS_DEV_FILEDISK_DATA  *NextPtr;
};


/*
*********************************************************************************************************
*                                           LOCAL CONSTANTS
*********************************************************************************************************
*/

#define  FS_DEV_FILEDISK_NAME_LEN               4u

static  const  CPU_CHAR  FSDev_Template_Name[] = "file";

static  FS_QTY      FSDev_FileDisk_UnitCtr;

/*
*********************************************************************************************************
*                                            LOCAL TABLES
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                       LOCAL GLOBAL VARIABLES
*********************************************************************************************************
*/

static  FS_DEV_FILEDISK_DATA  *FSDev_Template_ListFreePtr;


/*
*********************************************************************************************************
*                                            LOCAL MACRO'S
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                      LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*/

                                                                /* --------------- DEV INTERFACE FNCTS ---------------- */
                                                                /* Get name of driver.                                  */
static  const  CPU_CHAR       *FSDev_FileDisk_NameGet (void);
                                                                /* Init driver.                                         */
static  void                   FSDev_FileDisk_Init    (FS_ERR                *p_err);
                                                                /* Open device.                                         */
static  void                   FSDev_FileDisk_Open    (FS_DEV                *p_dev,
                                                       void                  *p_dev_cfg,
                                                       FS_ERR                *p_err);
                                                                /* Close device.                                        */
static  void                   FSDev_FileDisk_Close   (FS_DEV                *p_dev);
                                                                /* Read from device.                                    */
static  void                   FSDev_FileDisk_Rd      (FS_DEV                *p_dev,
                                                       void                  *p_dest,
                                                       FS_SEC_NBR             start,
                                                       FS_SEC_QTY             cnt,
                                                       FS_ERR                *p_err);

#if (FS_CFG_RD_ONLY_EN == DEF_DISABLED)                         /* Write to device.                                     */
static  void                   FSDev_FileDisk_Wr      (FS_DEV                *p_dev,
                                                       void                  *p_src,
                                                       FS_SEC_NBR             start,
                                                       FS_SEC_QTY             cnt,
                                                       FS_ERR                *p_err);
#endif
                                                                /* Get device info.                                     */
static  void                   FSDev_FileDisk_Query   (FS_DEV                *p_dev,
                                                       FS_DEV_INFO           *p_info,
                                                       FS_ERR                *p_err);
                                                                /* Perform device I/O control.                          */
static  void                   FSDev_FileDisk_IO_Ctrl (FS_DEV                *p_dev,
                                                       CPU_INT08U             opt,
                                                       void                  *p_data,
                                                       FS_ERR                *p_err);

                                                                /* ------------------- LOCAL FNCTS -------------------- */
                                                                /* Free #### data.                                      */
static  void                   FSDev_FileDisk_DataFree(FS_DEV_FILEDISK_DATA  *p_template_data);
                                                                /* Allocate & init #### data.                           */
static  FS_DEV_FILEDISK_DATA  *FSDev_FileDisk_DataGet (void);

/*
*********************************************************************************************************
*                                         INTERFACE STRUCTURE
*********************************************************************************************************
*/

const  FS_DEV_API  FSDev_FileDisk = {
    FSDev_FileDisk_NameGet,
    FSDev_FileDisk_Init,
    FSDev_FileDisk_Open,
    FSDev_FileDisk_Close,
    FSDev_FileDisk_Rd,
#if (FS_CFG_RD_ONLY_EN == DEF_DISABLED)
    FSDev_FileDisk_Wr,
#endif
    FSDev_FileDisk_Query,
    FSDev_FileDisk_IO_Ctrl
};


/*
*********************************************************************************************************
*                                     LOCAL CONFIGURATION ERRORS
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*********************************************************************************************************
*                                     DRIVER INTERFACE FUNCTIONS
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                        FSDev_####_NameGet()
*
* Description : Return name of device driver.
*
* Argument(s) : none.
*
* Return(s)   : Pointer to string which holds device driver name.
*
* Note(s)     : (1) The name MUST NOT include the ':' character.
*
*               (2) The name MUST be constant; each time this function is called, the same name MUST be
*                   returned.
*********************************************************************************************************
*/

static  const  CPU_CHAR  *FSDev_FileDisk_NameGet (void)
{
    return (FSDev_Template_Name);
}


/*
*********************************************************************************************************
*                                          FSDev_####_Init()
*
* Description : Initialize the driver.
*
* Argument(s) : p_err       Pointer to variable that will receive the return error code from this function :
*
*                               FS_ERR_NONE    Device driver initialized successfully.
*
* Return(s)   : none.
*
* Note(s)     : (1) This function should initialize any structures, tables or variables that are common
*                   to all devices or are used to manage devices accessed with the driver.  This function
*                   SHOULD NOT initialize any devices; that will be done individually for each with
*                   device driver's 'Open()' function.
*********************************************************************************************************
*/

static  void  FSDev_FileDisk_Init (FS_ERR  *p_err)
{
    FSDev_FileDisk_UnitCtr     =  0u;
    FSDev_Template_ListFreePtr = (FS_DEV_FILEDISK_DATA *)0;

   *p_err = FS_ERR_NONE;
}


/*
*********************************************************************************************************
*                                          FSDev_####_Open()
*
* Description : Open (initialize) a device instance.
*
* Argument(s) : p_dev       Pointer to device to open.
*
*               p_dev_cfg   Pointer to device configuration.
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               FS_ERR_NONE                    Device opened successfully.
*                               FS_ERR_DEV_ALREADY_OPEN        Device is already opened.
*                               FS_ERR_DEV_INVALID_CFG         Device configuration specified invalid.
*                               FS_ERR_DEV_INVALID_LOW_FMT     Device needs to be low-level formatted.
*                               FS_ERR_DEV_INVALID_LOW_PARAMS  Device low-level device parameters invalid.
*                               FS_ERR_DEV_INVALID_UNIT_NBR    Device unit number is invalid.
*                               FS_ERR_DEV_IO                  Device I/O error.
*                               FS_ERR_DEV_NOT_PRESENT         Device is not present.
*                               FS_ERR_DEV_TIMEOUT             Device timeout.
*                               FS_ERR_MEM_ALLOC               Memory could not be allocated.
*
* Return(s)   : none.
*
* Note(s)     : (1) Tracking whether a device is open is not necessary, because this should NEVER be
*                   called when a device is already open.
*
*               (2) Some drivers may need to track whether a device has been previously opened
*                   (indicating that the hardware has previously been initialized).
*
*               (3) This function will be called EVERY time the device is opened.
*
*               (4) See 'FSDev_Open() Notes #3'.
*
*               (5) The driver should identify the device instance to be opened by checking
*                   'p_dev->UnitNbr'.  For example, if "template:2:" is to be opened, then
*                   'p_dev->UnitNbr' will hold the integer 2.
*********************************************************************************************************
*/
static  void  FSDev_FileDisk_Open (FS_DEV  *p_dev,
                                   void    *p_dev_cfg,
                                   FS_ERR  *p_err)
{
    FS_DEV_FILEDISK_CFG *fd_cfg = (FS_DEV_FILEDISK_CFG*)p_dev_cfg;
    *p_err = FS_ERR_NONE;

    int fd = open(fd_cfg->FileName, O_RDWR | O_CREAT,
                  S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);

    if(fd < 0) {
        *p_err = FS_ERR_DEV_INVALID_NAME;
        return;
    }

    struct stat statbuf;
    int err = fstat(fd, &statbuf);

    if (err < 0) {
        close(fd);
        *p_err = FS_ERR_DEV_INVALID_NAME;
        return;
    }

    if (fd_cfg->SecSize * fd_cfg->Size > (statbuf.st_size)) {
        ftruncate(fd, fd_cfg->SecSize * fd_cfg->Size);
    }

    void* data = mmap(NULL, fd_cfg->SecSize * fd_cfg->Size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    close(fd);

    if(data == MAP_FAILED) {
        *p_err = FS_ERR_DEV_IO;
        return;
    }

    FS_DEV_FILEDISK_DATA* dev_data = FSDev_FileDisk_DataGet();

    p_dev->DataPtr = dev_data;
    
    if (p_dev->DataPtr == DEF_NULL) {
        munmap(data, fd_cfg->SecSize * fd_cfg->Size);
       *p_err = FS_ERR_MEM_ALLOC;
        return;
    }

    dev_data->SecSize = fd_cfg->SecSize;
    dev_data->SecCnt = fd_cfg->Size;
    dev_data->Data = data;    
}


/*
*********************************************************************************************************
*                                         FSDev_####_Close()
*
* Description : Close (uninitialize) a device instance.
*
* Argument(s) : p_dev       Pointer to device to close.
*
* Return(s)   : none.
*
* Note(s)     : (1) Tracking whether a device is open is not necessary, because this should ONLY be
*                   called when a device is open.
*
*               (2) This function will be called EVERY time the device is closed.
*********************************************************************************************************
*/

static  void  FSDev_FileDisk_Close (FS_DEV  *p_dev)
{
    if (p_dev->DataPtr != DEF_NULL) {
        FS_DEV_FILEDISK_DATA* dev_data = (FS_DEV_FILEDISK_DATA *)p_dev->DataPtr;
        munmap(dev_data->Data, dev_data->SecCnt * dev_data->SecSize);
        FSDev_FileDisk_DataFree((FS_DEV_FILEDISK_DATA *)p_dev->DataPtr);
    }
}


/*
*********************************************************************************************************
*                                           FSDev_####_Rd()
*
* Description : Read from a device & store data in buffer.
*
* Argument(s) : p_dev       Pointer to device to read from.
*
*               p_dest      Pointer to destination buffer.
*
*               start       Start sector of read.
*
*               cnt         Number of sectors to read.
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               FS_ERR_NONE                    Sector(s) read.
*                               FS_ERR_DEV_INVALID_UNIT_NBR    Device unit number is invalid.
*                               FS_ERR_DEV_NOT_OPEN            Device is not open.
*                               FS_ERR_DEV_NOT_PRESENT         Device is not present.
*                               FS_ERR_DEV_IO                  Device I/O error.
*                               FS_ERR_DEV_TIMEOUT             Device timeout.
*
* Return(s)   : none.
*
* Note(s)     : (1) Tracking whether a device is open is not necessary, because this should ONLY be
*                   called when a device is open.
*********************************************************************************************************
*/

static  void  FSDev_FileDisk_Rd (FS_DEV      *p_dev,
                                 void        *p_dest,
                                 FS_SEC_NBR   start,
                                 FS_SEC_QTY   cnt,
                                 FS_ERR      *p_err)
{
    FS_DEV_FILEDISK_DATA* dev_data = (FS_DEV_FILEDISK_DATA*)p_dev->DataPtr;

    if(start + cnt > dev_data->SecCnt) {
        *p_err = FS_ERR_DEV_IO;
        return;
    }
    Mem_Copy(p_dest, dev_data->Data + start * dev_data->SecSize, cnt * dev_data->SecSize);
    *p_err = FS_ERR_NONE;
}


/*
*********************************************************************************************************
*                                           FSDev_####_Wr()
*
* Description : Write data to a device from a buffer.
*
* Argument(s) : p_dev       Pointer to device to write to.
*
*               p_src       Pointer to source buffer.
*
*               start       Start sector of write.
*
*               cnt         Number of sectors to write.
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               FS_ERR_NONE                    Sector(s) written.
*                               FS_ERR_DEV_INVALID_UNIT_NBR    Device unit number is invalid.
*                               FS_ERR_DEV_NOT_OPEN            Device is not open.
*                               FS_ERR_DEV_NOT_PRESENT         Device is not present.
*                               FS_ERR_DEV_IO                  Device I/O error.
*                               FS_ERR_DEV_TIMEOUT             Device timeout.
*
* Return(s)   : none.
*
* Note(s)     : (1) Tracking whether a device is open is not necessary, because this should ONLY be
*                   called when a device is open.
*********************************************************************************************************
*/

#if (FS_CFG_RD_ONLY_EN == DEF_DISABLED)
static  void  FSDev_FileDisk_Wr (FS_DEV      *p_dev,
                                 void        *p_src,
                                 FS_SEC_NBR   start,
                                 FS_SEC_QTY   cnt,
                                 FS_ERR      *p_err)
{
    FS_DEV_FILEDISK_DATA* dev_data = (FS_DEV_FILEDISK_DATA*)p_dev->DataPtr;
    if(start + cnt > dev_data->SecCnt) {
        *p_err = FS_ERR_DEV_IO;
        return;
    }
    Mem_Copy(dev_data->Data + start * dev_data->SecSize, p_src, cnt * dev_data->SecSize);
    *p_err = FS_ERR_NONE;
}
#endif


/*
*********************************************************************************************************
*                                         FSDev_####_Query()
*
* Description : Get information about a device.
*
* Argument(s) : p_dev       Pointer to device to query.
*
*               p_info      Pointer to structure that will receive device information.
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               FS_ERR_NONE                    Device information obtained.
*                               FS_ERR_DEV_INVALID_UNIT_NBR    Device unit number is invalid.
*                               FS_ERR_DEV_NOT_OPEN            Device is not open.
*                               FS_ERR_DEV_NOT_PRESENT         Device is not present.
*
* Return(s)   : none.
*
* Note(s)     : (1) Tracking whether a device is open is not necessary, because this should ONLY be
*                   called when a device is open.
*********************************************************************************************************
*/

static  void  FSDev_FileDisk_Query (FS_DEV       *p_dev,
                                    FS_DEV_INFO  *p_info,
                                    FS_ERR       *p_err)
{
    FS_DEV_FILEDISK_DATA* dev_data = (FS_DEV_FILEDISK_DATA*)p_dev->DataPtr;
    p_info->SecSize = dev_data->SecSize;                                       /* ------- $$$$ DETERMINE SECTOR SIZE OF DEVICE ------- */;
    p_info->Size    = dev_data->SecCnt;                                       /* ---- $$$$ DETERMINE NUMBER OF SECTOR IN DEVICE ----- */;
    p_info->Fixed   = DEF_YES;                                  /*  $$$$ INDICATE WHETHER DEVICE IS FIXED OR REMOVABLE  */;

   *p_err = FS_ERR_NONE;
}


/*
*********************************************************************************************************
*                                        FSDev_####_IO_Ctrl()
*
* Description : Perform device I/O control operation.
*
* Argument(s) : p_dev       Pointer to device to control.
*
*               opt         Control command.
*
*               p_data      Buffer which holds data to be used for operation.
*                              OR
*                           Buffer in which data will be stored as a result of operation.
*
*               p_err       Pointer to variable that will receive return the error code from this function :
*
*                               FS_ERR_NONE                    Control operation performed successfully.
*                               FS_ERR_DEV_INVALID_IO_CTRL     I/O control operation unknown to driver.
*                               FS_ERR_DEV_INVALID_UNIT_NBR    Device unit number is invalid.
*                               FS_ERR_DEV_IO                  Device I/O error.
*                               FS_ERR_DEV_NOT_OPEN            Device is not open.
*                               FS_ERR_DEV_NOT_PRESENT         Device is not present.
*                               FS_ERR_DEV_TIMEOUT             Device timeout.
*
* Return(s)   : none.
*
* Note(s)     : (1) Tracking whether a device is open is not necessary, because this should ONLY be
*                   called when a device is open.
*
*               (2) Defined I/O control operations are :
*
*                   (a) FS_DEV_IO_CTRL_REFRESH           Refresh device.
*                   (b) FS_DEV_IO_CTRL_LOW_FMT           Low-level format device.
*                   (c) FS_DEV_IO_CTRL_LOW_MOUNT         Low-level mount device.
*                   (d) FS_DEV_IO_CTRL_LOW_UNMOUNT       Low-level unmount device.
*                   (e) FS_DEV_IO_CTRL_LOW_COMPACT       Low-level compact device.
*                   (f) FS_DEV_IO_CTRL_LOW_DEFRAG        Low-level defragment device.
*                   (g) FS_DEV_IO_CTRL_SEC_RELEASE       Release data in sector.
*                   (h) FS_DEV_IO_CTRL_PHY_RD            Read  physical device.
*                   (i) FS_DEV_IO_CTRL_PHY_WR            Write physical device.
*                   (j) FS_DEV_IO_CTRL_PHY_RD_PAGE       Read  physical device page.
*                   (k) FS_DEV_IO_CTRL_PHY_WR_PAGE       Write physical device page.
*                   (l) FS_DEV_IO_CTRL_PHY_ERASE_BLK     Erase physical device block.
*                   (m) FS_DEV_IO_CTRL_PHY_ERASE_CHIP    Erase physical device.
*
*                   Not all of these operations are valid for all devices.
*********************************************************************************************************
*/

static  void  FSDev_FileDisk_IO_Ctrl (FS_DEV      *p_dev,
                                      CPU_INT08U   opt,
                                      void        *p_data,
                                      FS_ERR      *p_err)
{
   (void)p_data;
   int ret;

   *p_err = FS_ERR_NONE;

    FS_DEV_FILEDISK_DATA* dev_data = (FS_DEV_FILEDISK_DATA*)p_dev->DataPtr;

   switch(opt) {
    case FS_DEV_IO_CTRL_FILE_SYNC:
        ret = msync(dev_data->Data, dev_data->SecCnt * dev_data->SecSize, MS_SYNC);
        break;
    default:
        *p_err = FS_ERR_DEV_INVALID_IO_CTRL;
        break;
   }   
}


/*
*********************************************************************************************************
*********************************************************************************************************
*                                           LOCAL FUNCTIONS
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                       FSDev_####_DataFree()
*
* Description : Free a #### data object.
*
* Argument(s) : p_####_data   Pointer to a #### data object.
*               -----------   Argument validated by caller.
*
* Return(s)   : none.
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  FSDev_FileDisk_DataFree (FS_DEV_FILEDISK_DATA  *p_template_data)
{
    CPU_SR_ALLOC();


    CPU_CRITICAL_ENTER();
    p_template_data->NextPtr   = FSDev_Template_ListFreePtr;            /* Add to free pool.                                    */
    FSDev_Template_ListFreePtr = p_template_data;
    CPU_CRITICAL_EXIT();
}


/*
*********************************************************************************************************
*                                        FSDev_####_DataGet()
*
* Description : Allocate & initialize a #### data object.
*
* Argument(s) : none.
*
* Return(s)   : Pointer to a #### data object, if NO errors.
*               Pointer to NULL,               otherwise.
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  FS_DEV_FILEDISK_DATA  *FSDev_FileDisk_DataGet (void)
{
    LIB_ERR                alloc_err;
    CPU_SIZE_T             octets_reqd;
    FS_DEV_FILEDISK_DATA  *p_template_data;
    CPU_SR_ALLOC();


                                                                /* --------------------- ALLOC DATA ------------------- */
    CPU_CRITICAL_ENTER();
    if (FSDev_Template_ListFreePtr == (FS_DEV_FILEDISK_DATA *)0) {
        p_template_data = (FS_DEV_FILEDISK_DATA *)Mem_HeapAlloc( sizeof(FS_DEV_FILEDISK_DATA),
                                                                 sizeof(CPU_INT32U),
                                                                &octets_reqd,
                                                                &alloc_err);
        if (p_template_data == (FS_DEV_FILEDISK_DATA *)0) {
            CPU_CRITICAL_EXIT();
            FS_TRACE_DBG(("FSDev_Template_DataGet(): Could not alloc mem for #### data: %d octets required.\r\n", octets_reqd));
            return ((FS_DEV_FILEDISK_DATA *)0);
        }
        (void)alloc_err;

        FSDev_FileDisk_UnitCtr++;


    } else {
        p_template_data            = FSDev_Template_ListFreePtr;
        FSDev_Template_ListFreePtr = FSDev_Template_ListFreePtr->NextPtr;
    }
    CPU_CRITICAL_EXIT();


                                                                /* ---------------------- CLR DATA -------------------- */
#if (FS_CFG_CTR_STAT_EN    == DEF_ENABLED)                      /* Clr stat ctrs.                                       */
    p_template_data->StatRdCtr =  0u;
    p_template_data->StatWrCtr =  0u;
#endif

#if (FS_CFG_CTR_ERR_EN     == DEF_ENABLED)                      /* Clr err ctrs.                                        */
   /* $$$$ CLEAR ERROR COUNTERS */
#endif

    p_template_data->NextPtr   = (FS_DEV_FILEDISK_DATA *)0;

    return (p_template_data);
}
