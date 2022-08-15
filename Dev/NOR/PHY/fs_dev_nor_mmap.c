
#define  FS_DEV_NOR_MMAP_MODULE


#include  <Source/fs.h>



#include  "fs_dev_nor_mmap.h"

#include "../BSP/bsp_fs_dev_nor_file.h"


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
    p_phy_data->DataPtr = FSDev_NOR_File_BSP_Open(p_phy_data);
   *p_err = p_phy_data->DataPtr != DEF_NULL ? FS_ERR_NONE : FS_ERR_DEV_INVALID;
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
    FSDev_NOR_File_BSP_Close(p_phy_data->DataPtr);
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
    FSDev_NOR_File_BSP_Read(p_phy_data->DataPtr, start, cnt, p_dest);
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
FSDev_NOR_File_BSP_Write(p_phy_data->DataPtr, start, cnt, p_src);
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
    FSDev_NOR_File_BSP_Erase(p_phy_data->DataPtr, start, size);
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
#if 0
    FS_DEV_NOR_MMAP_DATA* nor_data = (FS_DEV_NOR_MMAP_DATA*)p_phy_data->DataPtr;
    Mem_Set(nor_data->Data + nor_data->Offset, 0xFF, nor_data->SecSize * nor_data->SecCnt);
#else
#endif
    *p_err = FS_ERR_NONE;
}
