
#define  FS_DEV_NOR_MMAP_MODULE


#include  <Source/fs.h>

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include  "fs_dev_nor_mmap.h"


typedef  struct  fs_dev_nor_mmap_data  FS_DEV_NOR_MMAP_DATA;
struct  fs_dev_nor_mmap_data {
    CPU_SIZE_T             SecSize;
    CPU_SIZE_T             SecCnt;
    CPU_SIZE_T             Offset;
    void*                  Data;

#if (FS_CFG_CTR_STAT_EN == DEF_ENABLED)
    FS_CTR                 StatRdCtr;
    FS_CTR                 StatWrCtr;
#endif

#if (FS_CFG_CTR_ERR_EN == DEF_ENABLED)
   /* $$$$ ERROR COUNTERS */
#endif

    FS_DEV_NOR_MMAP_DATA  *NextPtr;
};

typedef struct {
    uint32_t version;
    uint32_t blockCount;
    uint32_t blockSize;
    uint32_t offset;
} nor_image_header;

typedef struct {
    uint16_t erase_count;
    uint16_t write_count;
} nor_block_stat;

static  FS_DEV_NOR_MMAP_DATA  *FSDev_NOR_ListFreePtr;

static void FSDev_NOR_DataFree (FS_DEV_NOR_MMAP_DATA  * p_data)
{
    CPU_SR_ALLOC();

    CPU_CRITICAL_ENTER();
    p_data->NextPtr = FSDev_NOR_ListFreePtr;
    FSDev_NOR_ListFreePtr = p_data;
    CPU_CRITICAL_EXIT();
}

static  FS_DEV_NOR_MMAP_DATA  *FSDev_NOR_DataGet (void)
{
    LIB_ERR                alloc_err;
    CPU_SIZE_T             octets_reqd;
    FS_DEV_NOR_MMAP_DATA  *p_data;
    CPU_SR_ALLOC();

    CPU_CRITICAL_ENTER();
    if (FSDev_NOR_ListFreePtr == (FS_DEV_NOR_MMAP_DATA *)0) {
        p_data = (FS_DEV_NOR_MMAP_DATA *)Mem_HeapAlloc( sizeof(FS_DEV_NOR_MMAP_DATA),
                                                                 sizeof(CPU_INT32U),
                                                                &octets_reqd,
                                                                &alloc_err);
        if (p_data == (FS_DEV_NOR_MMAP_DATA *)0) {
            CPU_CRITICAL_EXIT();
            return ((FS_DEV_NOR_MMAP_DATA *)0);
        }
        (void)alloc_err;

    } else {
        p_data            = FSDev_NOR_ListFreePtr;
        FSDev_NOR_ListFreePtr = FSDev_NOR_ListFreePtr->NextPtr;
    }
    CPU_CRITICAL_EXIT();


                                                                /* ---------------------- CLR DATA -------------------- */
#if (FS_CFG_CTR_STAT_EN    == DEF_ENABLED)                      /* Clr stat ctrs.                                       */
    p_data->StatRdCtr =  0u;
    p_data->StatWrCtr =  0u;
#endif

#if (FS_CFG_CTR_ERR_EN     == DEF_ENABLED)                      /* Clr err ctrs.                                        */
   /* $$$$ CLEAR ERROR COUNTERS */
#endif

    p_data->NextPtr   = (FS_DEV_NOR_MMAP_DATA *)0;

    return (p_data);
}


static  void  FSDev_NOR_PHY_Open     (FS_DEV_NOR_PHY_DATA  *p_phy_data,     /* Open NOR device.                         */
                                      FS_ERR               *p_err);

static  void  FSDev_NOR_PHY_Close    (FS_DEV_NOR_PHY_DATA  *p_phy_data);    /* Close NOR device.                        */

static  void  FSDev_NOR_PHY_Rd       (FS_DEV_NOR_PHY_DATA  *p_phy_data,     /* Read from NOR device.                    */
                                      void                 *p_dest,
                                      CPU_INT32U            start,
                                      CPU_INT32U            cnt,
                                      FS_ERR               *p_err);

static  void  FSDev_NOR_PHY_Wr       (FS_DEV_NOR_PHY_DATA  *p_phy_data,     /* Write to NOR device.                     */
                                      void                 *p_src,
                                      CPU_INT32U            start,
                                      CPU_INT32U            cnt,
                                      FS_ERR               *p_err);

static  void  FSDev_NOR_PHY_EraseBlk (FS_DEV_NOR_PHY_DATA  *p_phy_data,     /* Erase block on NOR device.               */
                                      CPU_INT32U            start,
                                      CPU_INT32U            size,
                                      FS_ERR               *p_err);

static  void  FSDev_NOR_PHY_IO_Ctrl  (FS_DEV_NOR_PHY_DATA  *p_phy_data,     /* Perform NOR device I/O control.          */
                                      CPU_INT08U            opt,
                                      void                 *p_data,
                                      FS_ERR               *p_err);


                                                                            /* -------------- LOCAL FNCTS ------------- */
static  void  FSDev_NOR_PHY_EraseChip(FS_DEV_NOR_PHY_DATA  *p_phy_data,     /* Erase NOR device.                        */
                                      FS_ERR               *p_err);

const  FS_DEV_NOR_PHY_API  FSDev_NOR_MMAP = {
    FSDev_NOR_PHY_Open,
    FSDev_NOR_PHY_Close,
    FSDev_NOR_PHY_Rd,
    FSDev_NOR_PHY_Wr,
    FSDev_NOR_PHY_EraseBlk,
    FSDev_NOR_PHY_IO_Ctrl
};


/*
*********************************************************************************************************
*                                        FSDev_NOR_PHY_Open()
*
* Description : Open (initialize) a NOR device instance & get NOR device information.
*
* Argument(s) : p_phy_data  Pointer to NOR phy data.
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               FS_ERR_NONE                    Device driver initialized successfully.
*                               FS_ERR_DEV_INVALID_CFG         Device configuration specified invalid.
*                               FS_ERR_DEV_INVALID_UNIT_NBR    Device unit number is invalid.
*                               FS_ERR_DEV_IO                  Device I/O error.
*                               FS_ERR_DEV_TIMEOUT             Device timeout.
*
* Return(s)   : none.
*
* Note(s)     : (1) Several members of 'p_phy_data' may need to be used/assigned :
*
*                   (a) 'BlkCnt' & 'BlkSize' MUST be assigned the block count & block size of the device,
*                       respectively.
*
*                   (b) 'RegionNbr' specifies the block region that will be used.  'AddrRegionStart' MUST
*                       be assigned the start address of this block region.
*
*                   (c) 'DataPtr' may store a pointer to any driver-specific data.
*
*                   (d) 'UnitNbr' is the unit number of the NOR device.
*
*                   (e) (1) 'MaxClkFreq' specifies the maximum SPI clock frequency.
*
*
*                       (2) 'BusWidth', 'BusWidthMax' & 'PhyDevCnt' specify the bus configuration.
*                           'AddrBase' specifies the base address of the NOR flash memory.
*********************************************************************************************************
*/

static  void  FSDev_NOR_PHY_Open (FS_DEV_NOR_PHY_DATA  *p_phy_data,
                                  FS_ERR               *p_err)
{
    char filename[16];
    sprintf(filename, "nor%d.bin", p_phy_data->UnitNbr);

    int fd = open(filename, O_RDWR | O_CREAT,
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

    nor_image_header hdr;

    if(read(fd, &hdr, sizeof(hdr)) != sizeof(hdr)) {
        hdr.version = 1;
        hdr.blockCount = 32;
        hdr.blockSize = 65536;
        hdr.offset = 1024;
        lseek(fd, 0, SEEK_SET);
        write(fd, &hdr, sizeof(hdr));
    }

    FS_SEC_SIZE flashSize = hdr.blockCount * hdr.blockSize;

    if ((flashSize + hdr.offset) > (statbuf.st_size)) {
        ftruncate(fd, flashSize + hdr.offset);
    }

    void* data = mmap(NULL, flashSize + hdr.offset, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    close(fd);

    if(data == MAP_FAILED) {
        *p_err = FS_ERR_DEV_IO;
        return;
    }

    FS_DEV_NOR_MMAP_DATA* dev_data = FSDev_NOR_DataGet();
    
    if (dev_data == DEF_NULL) {
        munmap(data, flashSize);
       *p_err = FS_ERR_MEM_ALLOC;
        return;
    }

    dev_data->Data = data;
    dev_data->Offset = hdr.offset;
    dev_data->SecCnt = hdr.blockCount;
    dev_data->SecSize = hdr.blockSize;
                                                                /* ------------------- SAVE PHY INFO ------------------ */
    p_phy_data->BlkCnt          =  hdr.blockCount;              /* $$$$ Assign blk cnt  of device.                   */
    p_phy_data->BlkSize         =  hdr.blockSize;               /* $$$$ Assign blk size of device.                   */
    p_phy_data->DataPtr         =  dev_data;                    /* $$$$ Save any driver-specific data, if necessary. */
    p_phy_data->AddrRegionStart =  p_phy_data->AddrBase;        /* $$$$ Assign start of addr region.                  */

   *p_err = FS_ERR_NONE;
}


/*
*********************************************************************************************************
*                                        FSDev_NOR_PHY_Close()
*
* Description : Close (uninitialize) a NOR device instance.
*
* Argument(s) : p_phy_data  Pointer to NOR phy data.
*
* Return(s)   : none.
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  FSDev_NOR_PHY_Close (FS_DEV_NOR_PHY_DATA  *p_phy_data)
{
    FS_DEV_NOR_MMAP_DATA* p_data = (FS_DEV_NOR_MMAP_DATA*)p_phy_data->DataPtr;

    munmap(p_data->Data, p_data->SecCnt * p_data->SecSize + p_data->Offset);

    FSDev_NOR_DataFree(p_data);

    p_phy_data->DataPtr = 0;
}


/*
*********************************************************************************************************
*                                         FSDev_NOR_PHY_Rd()
*
* Description : Read from a NOR device & store data in buffer.
*
* Argument(s) : p_phy_data  Pointer to NOR phy data.
*
*               p_dest      Pointer to destination buffer.
*
*               start       Start address of read (relative to start of device).
*
*               cnt         Number of octets to read.
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               FS_ERR_NONE           Octets read successfully.
*                               FS_ERR_DEV_IO         Device I/O error.
*                               FS_ERR_DEV_TIMEOUT    Device timeout.
*
* Return(s)   : none.
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  FSDev_NOR_PHY_Rd (FS_DEV_NOR_PHY_DATA  *p_phy_data,
                                void                 *p_dest,
                                CPU_INT32U            start,
                                CPU_INT32U            cnt,
                                FS_ERR               *p_err)
{
    FS_DEV_NOR_MMAP_DATA* nor_data = (FS_DEV_NOR_MMAP_DATA*)p_phy_data->DataPtr;
    Mem_Copy(p_dest, nor_data->Data + nor_data->Offset + start, cnt);
    *p_err = FS_ERR_NONE;
}


/*
*********************************************************************************************************
*                                         FSDev_NOR_PHY_Wr()
*
* Description : Write data to a NOR device from a buffer.
*
* Argument(s) : p_phy_data  Pointer to NOR phy data.
*
*               p_src       Pointer to source buffer.
*
*               start       Start address of write (relative to start of device).
*
*               cnt         Number of octets to write.
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               FS_ERR_NONE           Octets written successfully.
*                               FS_ERR_DEV_IO         Device I/O error.
*                               FS_ERR_DEV_TIMEOUT    Device timeout.
*
* Return(s)   : none.
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  FSDev_NOR_PHY_Wr (FS_DEV_NOR_PHY_DATA  *p_phy_data,
                                void                 *p_src,
                                CPU_INT32U            start,
                                CPU_INT32U            cnt,
                                FS_ERR               *p_err)
{
    FS_DEV_NOR_MMAP_DATA* nor_data = (FS_DEV_NOR_MMAP_DATA*)p_phy_data->DataPtr;
    Mem_Copy(nor_data->Data + nor_data->Offset + start, p_src, cnt);
    *p_err = FS_ERR_NONE;
}


/*
*********************************************************************************************************
*                                      FSDev_NOR_PHY_EraseBlk()
*
* Description : Erase block of NOR device.
*
* Argument(s) : p_phy_data  Pointer to NOR phy data.
*
*               start       Start address of block (relative to start of device).
*
*               size        Size of block, in octets.
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               FS_ERR_NONE              Block erased successfully.
*                               FS_ERR_DEV_INVALID_OP    Invalid operation for device.
*                               FS_ERR_DEV_IO            Device I/O error.
*                               FS_ERR_DEV_TIMEOUT       Device timeout.
*
* Return(s)   : none.
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  FSDev_NOR_PHY_EraseBlk (FS_DEV_NOR_PHY_DATA  *p_phy_data,
                                      CPU_INT32U            start,
                                      CPU_INT32U            size,
                                      FS_ERR               *p_err)
{
    FS_DEV_NOR_MMAP_DATA* nor_data = (FS_DEV_NOR_MMAP_DATA*)p_phy_data->DataPtr;
    Mem_Set(nor_data->Data + nor_data->Offset + start, 0xFF, size);
    *p_err = FS_ERR_NONE;
}


/*
*********************************************************************************************************
*                                       FSDev_NOR_PHY_IO_Ctrl()
*
* Description : Perform NOR device I/O control operation.
*
* Argument(s) : p_phy_data  Pointer to NOR phy data.
*
*               opt         Control command.
*
*               p_data      Buffer which holds data to be used for operation.
*                              OR
*                           Buffer in which data will be stored as a result of operation.
*
*               p_err       Pointer to variable that will receive the return the error code from this function :
*
*                               FS_ERR_NONE                   Control operation performed successfully.
*                               FS_ERR_DEV_INVALID_IO_CTRL    I/O control operation unknown to driver.
*                               FS_ERR_DEV_INVALID_OP         Invalid operation for device.
*                               FS_ERR_DEV_IO                 Device I/O error.
*                               FS_ERR_DEV_TIMEOUT            Device timeout.
*
* Return(s)   : none.
*
* Note(s)     : (1) Defined I/O control operations are :
*
*                   (a) FS_DEV_IO_CTRL_PHY_ERASE_CHIP    Erase entire chip.
*********************************************************************************************************
*/

static  void  FSDev_NOR_PHY_IO_Ctrl (FS_DEV_NOR_PHY_DATA  *p_phy_data,
                                     CPU_INT08U            opt,
                                     void                 *p_data,
                                     FS_ERR               *p_err)
{
    switch (opt) {
        case FS_DEV_IO_CTRL_PHY_ERASE_CHIP:                     /* -------------------- ERASE CHIP -------------------- */
             FSDev_NOR_PHY_EraseChip(p_phy_data, p_err);
             break;

        default:                                                /* --------------- UNSUPPORTED I/O CTRL --------------- */
            *p_err = FS_ERR_DEV_INVALID_IO_CTRL;
             break;
    }
}

/*
*********************************************************************************************************
*                                     FSDev_NOR_PHY_EraseChip()
*
* Description : Erase NOR device.
*
* Argument(s) : p_phy_data  Pointer to NOR phy data.
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               FS_ERR_NONE              Block erased successfully.
*                               FS_ERR_DEV_INVALID_OP    Invalid operation for device.
*                               FS_ERR_DEV_IO            Device I/O error.
*                               FS_ERR_DEV_TIMEOUT       Device timeout.
*
* Return(s)   : none.
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  FSDev_NOR_PHY_EraseChip (FS_DEV_NOR_PHY_DATA  *p_phy_data,
                                       FS_ERR               *p_err)
{
    FS_DEV_NOR_MMAP_DATA* nor_data = (FS_DEV_NOR_MMAP_DATA*)p_phy_data->DataPtr;
    Mem_Set(nor_data->Data + nor_data->Offset, 0xFF, nor_data->SecSize * nor_data->SecCnt);
    *p_err = FS_ERR_NONE;
}
