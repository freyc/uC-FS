#define  FS_NAND_CTRLR_FILE_MODULE


#include  <fs_cfg.h>
#include  <fs_dev_nand_cfg.h>
#include  <Source/crc_util.h>
#include  "../../../Source/fs.h"
#include  "../../../Source/fs_dev.h"
#include  "../../../Source/fs_util.h"
#include  "../fs_dev_nand.h"
#include  "fs_dev_nand_ctrlr_file.h"

//TODO: this should be moved to the bsp-layer
#define PAGE_DATA_SIZE  (2048u)
#define PAGE_SPARE_SIZE (64u)
#define PAGE_SIZE       (PAGE_DATA_SIZE + PAGE_SPARE_SIZE)
#define PAGES_PER_BLOCK (64u)
#define BLOCKS_PER_DEVICE (1024u)

#define  FS_NAND_CTRLR_FILE_MAX_EXT_MOD                 1u       /* Max nbr of reg'd ext mods.                           */
#define  FS_NAND_CTRLR_FILE_MAX_SPARE_ZONES           255u       /* Max nbr of spare area zones.                         */


typedef  CPU_INT08U  FS_NAND_CTRLR_ADDR_FMT_FLAGS;

typedef  CPU_INT08U  FS_NAND_CTRLR_GEN_SPARE_ZONE_IX;


#define  FS_NAND_CTRLR_SPARE_ZONE_IX_MAX        255u


#define  FS_NAND_CTRLR_FILE_CFG_FIELD(type, name, dflt_val)  dflt_val,
const  FS_NAND_CTRLR_FILE_CFG  FS_NAND_CtrlrFile_DfltCfg = {FS_NAND_CTRLR_FILE_CFG_FIELDS};
#undef   FS_NAND_CTRLR_FILE_CFG_FIELD

       FS_NAND_CTRLR_FILE_DATA  *FS_NAND_CtrlrMmap_ListFreePtr;
const  FS_NAND_CTRLR_FILE_EXT   *FS_NAND_CtrlrMmap_ListExtMod[FS_NAND_CTRLR_FILE_MAX_EXT_MOD];


/*
*********************************************************************************************************
*                                      CTRLR GENERIC API PROTOTYPES
*********************************************************************************************************
*/

                                                                /* Init NAND controller.                                */
static  void                Init       (FS_ERR            *p_err);

                                                                /* Open NAND controller.                                */
static  void               *Open       (FS_NAND_PART_API  *p_part_api,
                                        void              *p_bsp_api_v,
                                        void              *p_ctrlr_cfg_v,
                                        void              *p_part_cfg,
                                        FS_ERR            *p_err);

                                                                /* Close NAND controller.                               */
static  void                Close      (void              *p_ctrlr_data_v);

                                                                /* Get pointer to part data structure.                  */
static  FS_NAND_PART_DATA  *PartDataGet(void              *p_ctrlr_data_v);

                                                                /* Setup controller.                                    */
static  FS_NAND_OOS_INFO    Setup      (void              *p_ctrlr_data_v,
                                        FS_NAND_PG_SIZE    sec_size,
                                        FS_ERR            *p_err);

                                                                /* --------------  NAND FLASH HIGH LVL OPS ------------ */
                                                                /* Read sector from NAND device.                        */
static  void                SecRd      (void              *p_ctrlr_data_v,
                                        void              *p_dest,
                                        void              *p_dest_oos,
                                        FS_SEC_NBR         sec_ix_phy,
                                        FS_ERR            *p_err);

                                                                /* Read out of sector data.                             */
static  void                OOSRdRaw   (void              *p_ctrlr_data_v,
                                        void              *p_dest_oos,
                                        FS_SEC_NBR         sec_ix_phy,
                                        FS_NAND_PG_SIZE    offset,
                                        FS_NAND_PG_SIZE    len,
                                        FS_ERR            *p_err);

                                                                /* Read raw pg data.                                    */
static  void                PgRdRaw    (void              *p_ctrlr_data_v,
                                        void              *p_dest,
                                        FS_SEC_NBR         pg_ix_phy,
                                        FS_NAND_PG_SIZE    offset,
                                        FS_NAND_PG_SIZE    len,
                                        FS_ERR            *p_err);

                                                                /* Read spare area data.                                */
static  void                SpareRdRaw (void              *p_ctrlr_data_v,
                                        void              *p_dest_spare,
                                        FS_SEC_NBR         pg_ix_phy,
                                        FS_NAND_PG_SIZE    offset,
                                        FS_NAND_PG_SIZE    len,
                                        FS_ERR            *p_err);

                                                                /* Write sector to NAND device.                         */
static  void                SecWr      (void              *p_ctrlr_data_v,
                                        void              *p_src,
                                        void              *p_src_oos,
                                        FS_SEC_NBR         sec_ix_phy,
                                        FS_ERR            *p_err);

                                                                /* Erase block on NAND device.                          */
static  void                BlkErase   (void              *p_ctrlr_data_v,
                                        CPU_INT32U         blk_ix_phy,
                                        FS_ERR            *p_err);

                                                                /* Perform device I/O control.                          */
static  void                IO_Ctrl    (void              *p_ctrlr_data_v,
                                        CPU_INT08U         cmd,
                                        void              *p_arg,
                                        FS_ERR            *p_err);


/*
*********************************************************************************************************
*                                       LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*/

static  void                     OOS_Setup   (FS_NAND_CTRLR_FILE_DATA        *p_ctrlr_data,
                                              CPU_INT08U                     n_sec_per_pg,
                                              FS_ERR                        *p_err);

static  CPU_INT08U               StatusRd    (FS_NAND_CTRLR_FILE_DATA        *p_ctrlr_data);

static  void                     AddrFmt     (FS_NAND_CTRLR_FILE_DATA        *p_ctrlr_data,
                                              FS_SEC_QTY                     pg_ix,
                                              FS_NAND_PG_SIZE                pg_offset,
                                              FS_NAND_CTRLR_ADDR_FMT_FLAGS   addr_fmt_flags,
                                              CPU_INT08U                    *p_addr);

static  void                     SparePack   (FS_NAND_CTRLR_FILE_DATA        *p_ctrlr_data,
                                              void                          *p_spare,
                                              CPU_INT08U                     spare_seg_ix);

static  void                     SpareUnpack (FS_NAND_CTRLR_FILE_DATA        *p_ctrlr_data,
                                              void                          *p_spare,
                                              CPU_INT08U                     spare_seg_ix);

static  void                     SpareSplit  (void                          *p_spare,
                                              FS_NAND_PG_SIZE                buf_len,
                                              FS_NAND_PG_SIZE                pos,
                                              FS_NAND_PG_SIZE                len);

static  void                     SpareJoin   (void                          *p_spare,
                                              FS_NAND_PG_SIZE                buf_len,
                                              FS_NAND_PG_SIZE                pos,
                                              FS_NAND_PG_SIZE                len);

static  void                     ParamPgRd   (void                              *p_ctrlr_data_v,
                                              FS_NAND_RD_PARAM_PG_IO_CTRL_DATA  *p_param_pg,
                                              FS_ERR                            *p_err);

static  void                     ExtReg      (const  FS_NAND_CTRLR_FILE_EXT  *p_ext,
                                              FS_ERR                        *p_err);

static  FS_NAND_CTRLR_FILE_DATA  *DataGet     (void);

static  void                     DataFree    (FS_NAND_CTRLR_FILE_DATA        *p_ctrlr_data);


/*
*********************************************************************************************************
*                                          INTERFACE STRUCTURE
*********************************************************************************************************
*/

const  FS_NAND_CTRLR_API  FS_NAND_CtrlrFile = {
    Init,                                                       /* Init NAND ctrlr.                                     */
    Open,                                                       /* Open NAND ctrlr.                                     */
    Close,                                                      /* Close NAND ctrlr.                                    */
    PartDataGet,                                                /* Get part data ptr.                                   */
    Setup,                                                      /* Setup NAND ctrlr.                                    */
    SecRd,                                                      /* Rd sec from NAND dev.                                */
    OOSRdRaw,                                                   /* Rd out of sec data for specified sec without ECC.    */
    SpareRdRaw,                                                 /* Rd pg spare data from NAND dev without ECC.          */
    SecWr,                                                      /* Wr sec on NAND dev.                                  */
    BlkErase,                                                   /* Erase blk on NAND dev.                               */
    IO_Ctrl                                                     /* Perform NAND dev I/O ctrl.                           */
};


/*
*********************************************************************************************************
*                                       LOCAL CONFIGURATION ERRORS
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*********************************************************************************************************
*                                           GLOBAL FUNCTIONS
*********************************************************************************************************
*********************************************************************************************************
*/



/*
*********************************************************************************************************
*********************************************************************************************************
*                                        NAND CTRLR API FUNCTIONS
*********************************************************************************************************
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                                Init()
*
* Description : Initialize a NAND generic controller implementation.
*
* Argument(s) : p_err   Pointer to variable that will receive the return error code from this function :
*
*                           FS_ERR_NONE     Device driver initialized successfully.
*
* Return(s)   : none.
*
* Note(s)     : none.
*
*********************************************************************************************************
*/

static  void  Init (FS_ERR  *p_err)
{
    CPU_INT08U  ext_ix;
    CPU_SR_ALLOC();


    (void)p_err;
    CPU_CRITICAL_ENTER();
    FS_NAND_CtrlrMmap_ListFreePtr = DEF_NULL;
    for (ext_ix = 0u; ext_ix < FS_NAND_CTRLR_FILE_MAX_EXT_MOD; ext_ix++) {
        FS_NAND_CtrlrMmap_ListExtMod[ext_ix] = DEF_NULL;
    }
    CPU_CRITICAL_EXIT();
}


/*
*********************************************************************************************************
*                                                Open()
*
* Description : Open a NAND controller instance & get NAND device information.
*
* Argument(s) : p_part_api      Pointer to part layer driver.
*               ----------      Argument validated by caller.
*
*               p_bsp_api_v     Pointer to BSP (board support package) layer driver.
*               -----------     Argument validated by caller.
*
*               p_ctrlr_cfg_v   Pointer to controller configuration.
*               -------------   Argument validated by caller.
*
*               p_err           Pointer to variable that will receive the return error code from this function :
*               -----           Argument validated by caller.
*
*                                   FS_ERR_DEV_INVALID_CFG          Received NULL part config pointer.
*                                   FS_ERR_DEV_INVALID_LOW_PARAMS   Parameters are not valid for the device.
*                                   FS_ERR_DEV_INVALID_OP           Device invalid operation.
*                                   FS_ERR_DEV_IO                   Device I/O error.
*                                   FS_ERR_DEV_TIMEOUT              Device timeout.
*                                   FS_ERR_MEM_ALLOC                Memory could not be allocated.
*                                   FS_ERR_NONE                     Device driver initialized successfully.
*
*                                   ---------------------RETURNED BY p_part_api->Open()---------------------
*                                   See p_part_api->Open() for additional return error codes.
*
*                                   -------------RETURNED BY p_ctrlr_data->CtrlrExtPtr->Open()--------------
*                                   See p_ctrlr_data->CtrlrExtPtr->Open() for additional return error codes.
*
*                                   --------------------------RETURNED BY ExtReg()--------------------------
*                                   See ExtReg() for additional return error codes.
*
* Return(s)   : Pointer to controller data, if NO errors;
*               NULL,                       otherwise.
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  *Open (FS_NAND_PART_API  *p_part_api,
                     void              *p_bsp_api_v,
                     void              *p_ctrlr_cfg_v,
                     void              *p_part_cfg,
                     FS_ERR            *p_err)
{
    FS_NAND_CTRLR_FILE_DATA           *p_ctrlr_data;
    FS_NAND_PART_DATA                *p_part_data;
    FS_NAND_CTRLR_FILE_CFG            *p_ctrlr_gen_cfg;
    FS_SEC_QTY                        nbr_of_pgs;
    FS_NAND_CTRLR_GEN_SPARE_ZONE_IX   ix;
    FS_NAND_PG_SIZE                   free_start;
    FS_NAND_PG_SIZE                   free_end;
    FS_NAND_PG_SIZE                   free_len;
    FS_NAND_PG_SIZE                   spare_size;
    CPU_INT08U                        cmd;
    CPU_INT08U                        pg_offset_size;
    CPU_INT08U                        pg_addr_size;
    CPU_INT08U                        id[2];
    CPU_INT08U                        id_manuf;
#if (FS_TRACE_LEVEL >= TRACE_LEVEL_INFO)
    CPU_INT08U                        id_dev;
#endif
    CPU_INT08U                        parity_manuf;
    CPU_INT08U                        addr;


                                                                /* -------------- GET ALLOC'D CTRLR DATA -------------- */
    p_ctrlr_data = (FS_NAND_CTRLR_FILE_DATA *)DataGet();
    if (p_ctrlr_data == DEF_NULL) {
       *p_err = FS_ERR_MEM_ALLOC;
        return (DEF_NULL);
    }

    p_ctrlr_data->BSP_Ptr = p_bsp_api_v;

                                                                /* Set Ctrlr cfg ptr.                                   */
    p_ctrlr_gen_cfg       = (FS_NAND_CTRLR_FILE_CFG *)p_ctrlr_cfg_v;


    if (p_part_cfg == DEF_NULL) {                               /* Validate part cfg ptr.                               */
        FS_TRACE_DBG(("(fs_nand_ctrlr_gen) Open(): NAND Part cfg NULL ptr.\r\n"));
       *p_err = FS_ERR_DEV_INVALID_CFG;
        return (DEF_NULL);
    }


    const char* filename = p_ctrlr_gen_cfg->FileName == DEF_NULL ? "nand.bin" : p_ctrlr_gen_cfg->FileName;

    void *bspData = p_ctrlr_data->BSP_Ptr->Open(filename, p_err);

    if(*p_err != FS_ERR_NONE) {
        return DEF_NULL;
    }

    p_ctrlr_data->BSPData = bspData;

    p_ctrlr_data->CtrlrExtPtr = p_ctrlr_gen_cfg->CtrlrExt;

    ExtReg(p_ctrlr_data->CtrlrExtPtr,                           /* Register extension.                                  */
           p_err);
    if (*p_err != FS_ERR_NONE) {
        FS_TRACE_DBG(("(fs_nand_ctrlr_gen) Open(): Error opening NAND generic controller extension.\r\n"));
        return (DEF_NULL);
    }

    if (p_ctrlr_data->CtrlrExtPtr != DEF_NULL && p_ctrlr_data->CtrlrExtPtr->Open != DEF_NULL) {
        p_ctrlr_data->CtrlrExtData = p_ctrlr_data->CtrlrExtPtr->Open(p_ctrlr_data,
                                                                     p_ctrlr_gen_cfg->CtrlrExtCfg,
                                                                     p_err);
        if (*p_err != FS_ERR_NONE) {
            FS_TRACE_DBG(("(fs_nand_ctrlr_gen) Open: Error opening ctrlr ext module.\r\n"));
            return (DEF_NULL);
        }
    }
                                                                /* -------------------- OPEN PART --------------------- */
    p_part_data = p_part_api->Open(&FS_NAND_CtrlrFile,
                                    p_ctrlr_data,
                                    p_part_cfg,
                                    p_err);

    if (*p_err != FS_ERR_NONE) {
        return (DEF_NULL);
    }
    p_ctrlr_data->PartPtr     = p_part_api;
    p_ctrlr_data->PartDataPtr = p_part_data;


                                                                /* ------------- VALIDATE PART PARAMETERS ------------- */
    free_end   = 0u;
    spare_size = 0u;
    ix         = 0u;

    while ((ix < FS_NAND_CTRLR_FILE_MAX_SPARE_ZONES) && (p_part_data->FreeSpareMap[ix].OctetOffset != FS_NAND_PG_IX_INVALID)) {

        free_start = p_part_data->FreeSpareMap[ix].OctetOffset;
        if (free_start < free_end) {                            /* Validate sections not overlapping or out of order.   */
            FS_TRACE_DBG(("(fs_nand_ctrlr_gen) Open: Invalid part config: FreeSpareMap should not contain overlapping sections.\r\n"));
           *p_err = FS_ERR_DEV_INVALID_LOW_PARAMS;
            return (DEF_NULL);
        }

        free_len   = p_part_data->FreeSpareMap[ix].OctetLen;
        if (free_len < 1) {                                     /* Validate len is positive.                            */
            FS_TRACE_DBG(("(fs_nand_ctrlr_gen) Open: Invalid part config: FreeSpareMap should contain section with positive and non-null length.\r\n"));
           *p_err = FS_ERR_DEV_INVALID_LOW_PARAMS;
            return (DEF_NULL);
        }

        free_end   = free_start + free_len;
        if (free_start >= free_end) {                           /* Validate sections do NOT overflow/wraparound.        */
            FS_TRACE_DBG(("(fs_nand_ctrlr_gen) Open: Invalid part config: FreeSpareMap should contain sections that wrap around.\r\n"));
           *p_err = FS_ERR_DEV_INVALID_LOW_PARAMS;
            return (DEF_NULL);
        }

                                                                /* Accumulate tot avail spare size.                     */
        spare_size += free_len;

        ix++;
    }

                                                                /* Validate that the available spare ...                */
                                                                /* ... is not greater than total spare.                 */
    if (spare_size > p_part_data->SpareSize) {
        FS_TRACE_DBG(("(fs_nand_ctrlr_gen) Open: Invalid part config: FreeSpareMap should contain at maximum the number of octets in the spare area.\r\n"));
       *p_err = FS_ERR_DEV_INVALID_LOW_PARAMS;
        return (DEF_NULL);
    }

    p_ctrlr_data->SpareTotalAvailSize  = spare_size;



#if (FS_CFG_CTR_STAT_EN == DEF_ENABLED)                         /* ------------------ INIT STAT CTRS ------------------ */
    p_ctrlr_data->Ctrs.StatRdCtr              = 0u;
    p_ctrlr_data->Ctrs.StatWrCtr              = 0u;
    p_ctrlr_data->Ctrs.StatEraseCtr           = 0u;
    p_ctrlr_data->Ctrs.StatSpareRdRawCtr      = 0u;
    p_ctrlr_data->Ctrs.StatOOSRdRawCtr        = 0u;
#endif

#if (FS_CFG_CTR_ERR_EN == DEF_ENABLED)                          /* ------------------ INIT ERR CTRS ------------------- */
    p_ctrlr_data->Ctrs.ErrCorrECC_Ctr         = 0u;
    p_ctrlr_data->Ctrs.ErrCriticalCorrECC_Ctr = 0u;
    p_ctrlr_data->Ctrs.ErrUncorrECC_Ctr       = 0u;
    p_ctrlr_data->Ctrs.ErrWrCtr               = 0u;
    p_ctrlr_data->Ctrs.ErrEraseCtr            = 0u;
#endif


   *p_err = FS_ERR_NONE;
    return ((void *)p_ctrlr_data);
}


/*
*********************************************************************************************************
*                                                Close()
*
* Description : Close (uninitialize) a NAND controller instance.
*
* Argument(s) : p_ctrlr_data_v  Pointer to NAND controller data.
*               --------------  Argument validated by caller.
*
* Return(s)   : none.
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  Close (void  *p_ctrlr_data_v)
{

    FS_NAND_CTRLR_FILE_DATA  *p_ctrlr_data;


    p_ctrlr_data = (FS_NAND_CTRLR_FILE_DATA *)p_ctrlr_data_v;

    if (p_ctrlr_data->PartDataPtr != DEF_NULL) {
        p_ctrlr_data->PartPtr->Close(p_ctrlr_data->PartDataPtr);
    }


    if (p_ctrlr_data->CtrlrExtPtr != DEF_NULL) {
        if (p_ctrlr_data->CtrlrExtPtr->Close != DEF_NULL) {
            p_ctrlr_data->CtrlrExtPtr->Close(p_ctrlr_data->CtrlrExtData);
        }
    }

    if (p_ctrlr_data->BSP_Ptr != DEF_NULL) {
        if (p_ctrlr_data->BSP_Ptr->Close  != DEF_NULL) {
            p_ctrlr_data->BSP_Ptr->Close(p_ctrlr_data->BSPData);
        }
    }

    DataFree(p_ctrlr_data);
}


/*
*********************************************************************************************************
*                                             PartDataGet()
*
* Description : Retrieve part data pointer from controller instance.
*
* Argument(s) : p_ctrlr_data_v  Pointer to NAND controller data.
*               --------------  Argument validated by caller.
*
* Return(s)   : Pointer to part data.
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  FS_NAND_PART_DATA  *PartDataGet (void  *p_ctrlr_data_v)
{
    FS_NAND_CTRLR_FILE_DATA  *p_ctrlr_data;


    p_ctrlr_data = (FS_NAND_CTRLR_FILE_DATA *)p_ctrlr_data_v;

    return (p_ctrlr_data->PartDataPtr);
}


/*
*********************************************************************************************************
*                                                Setup()
*
* Description : Setup controller module according to chosen sector size.
*
* Argument(s) : p_ctrlr_data_v  Pointer to NAND controller data.
*               --------------  Argument validated by caller.
*
*               sec_size        Sector size.
*
*               p_err           Pointer to variable that will receive the return error code from this function
*               -----           Argument validated by caller.
*
*                                   FS_ERR_DEV_INVALID_LOW_PARAMS   Parameters are not valid for the device.
*                                                                   codes.
*
*                                   -------------RETURNED BY p_ctrlr_data->CtrlrExtPtr->Setup()-------------
*                                   See p_ctrlr_data->CtrlrExtPtr->Setup() for additional return error codes.
*
*                                   ------------------------RETURNED BY OOS_Setup()-------------------------
*                                   See OOS_Setup() for additional return error codes.
*
* Return(s)   : Structure with size in octets of out of sector (OOS) area per sector
*               and pointer to OOS buffer                                           , if NO errors;
*
*               Structure with size 0 and null pointer,                               otherwise.
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  FS_NAND_OOS_INFO  Setup (void             *p_ctrlr_data_v,
                                 FS_NAND_PG_SIZE   sec_size,
                                 FS_ERR           *p_err)
{
    FS_NAND_CTRLR_FILE_DATA  *p_ctrlr_data;
    FS_NAND_OOS_INFO         OOS_info;
    FS_NAND_PG_SIZE          rsvd_size;
    CPU_INT16S               OOS_size;
    CPU_INT08U               n_sec_per_pg;


    p_ctrlr_data             = (FS_NAND_CTRLR_FILE_DATA *)p_ctrlr_data_v;
    p_ctrlr_data->SecSize    =  sec_size;


    OOS_info.BufPtr = DEF_NULL;
    OOS_info.Size   = 0u;


    rsvd_size = 0u;                                             /* ------------------ EXT MOD SETUP ------------------- */
    if (p_ctrlr_data->CtrlrExtPtr != DEF_NULL && p_ctrlr_data->CtrlrExtPtr->Setup != DEF_NULL) {
        rsvd_size = p_ctrlr_data->CtrlrExtPtr->Setup(p_ctrlr_data,
                                                     p_ctrlr_data->CtrlrExtData,
                                                     p_err);
        if (*p_err != FS_ERR_NONE) {
            return (OOS_info);
        }
    }

                                                                /* -------------------- OOS SETUP --------------------- */
                                                                /* Calc avail oos size per sec.                         */
    n_sec_per_pg  = p_ctrlr_data->PartDataPtr->PgSize / p_ctrlr_data->SecSize;
    OOS_size      = p_ctrlr_data->SpareTotalAvailSize;
    OOS_size     /= n_sec_per_pg;
    OOS_size     -= rsvd_size;
    if (OOS_size  < 0) {
        FS_TRACE_DBG(("(fs_nand_ctrlr_gen) Setup(): Not enough spare area available.\r\n"));
       *p_err = FS_ERR_DEV_INVALID_LOW_PARAMS;
        return (OOS_info);
    }
    p_ctrlr_data->OOS_SizePerSec = (CPU_INT16U)OOS_size;


    OOS_Setup(p_ctrlr_data,                /* Setup spare segs.                                    */
              n_sec_per_pg,
              p_err);
    if (*p_err != FS_ERR_NONE) {
        return (OOS_info);
    }

    OOS_info.Size   = p_ctrlr_data->OOS_SizePerSec;
    OOS_info.BufPtr = p_ctrlr_data->SpareBufPtr;

    return (OOS_info);
}


/*
*********************************************************************************************************
*                                        NAND FLASH HIGH LVL OPS
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                                SecRd()
*
* Description : Read a physical sector from a NAND device & store data in buffer.
*
* Argument(s) : p_ctrlr_data_v  Pointer to NAND controller data.
*               --------------  Argument validated by caller.
*
*               p_dest          Pointer to destination buffer.
*               ------          Argument validated by caller.
*
*               p_dest_oos      Pointer to destination spare buffer.
*               ----------      Argument validated by caller.
*
*               sec_ix_phy      Index of sector to read from.
*
*               p_err           Pointer to variable that will receive the return error code from this function :
*               -----           Argument validated by caller.
*
*                                   FS_ERR_DEV_TIMEOUT          Device timeout.
*                                   FS_ERR_ECC_CORR             An ECC error has been successfully corrected.
*                                   FS_ERR_ECC_CRITICAL_CORR    A critical ECC error has been successfully
*                                                               corrected. Data should be refreshed.
*                                   FS_ERR_ECC_UNCORR           An uncorrectable ECC error occurred.
*                                   FS_ERR_NONE                 Page read successfully.
*
*                                   ----------RETURNED BY p_ctrlr_data->CtrlrExtPtr->RdStatusChk()-----------
*                                   See p_ctrlr_data->CtrlrExtPtr->RdStatusChk() for additional return error codes.
*
* Return(s)   : none.
*
* Note(s)     : (1) The address is a 4- or 5-octet value, depending on the number of address bits necessary
*                   to specify the highest address on the NAND device. 1st and 2nd octets specify the column
*                   address, which specifies the location (in octets) within a page. 3rd, 4th and 5th octets
*                   specify the row address, which specifies the location of the page within the device.
*
*                          ----------------------------------------------------------------------         ---
*                          | BUS CYCLE  | I/O7 | I/O6 | I/O5 | I/O4 | I/O3 | I/O2 | I/O1 | I/O0 |          |
*                          ----------------------------------------------------------------------    Column Address
*                          |    1st     |  A7  |  A6  |  A5  |  A4  |  A3  |  A2  |  A1  |  A0  | (octet within page)
*                          ----------------------------------------------------------------------          |
*                          |    2nd     |  --- |  --- |  --- |  --- |  A11 |  A10 |  A9  |  A8  |          |
*                          ----------------------------------------------------------------------         ---
*                          |    3rd     |  A19 |  A18 |  A17 |  A16 |  A15 |  A14 |  A13 |  A12 |          |
*                          ----------------------------------------------------------------------     Row Address
*                          |    4th     |  A27 |  A26 |  A25 |  A24 |  A23 |  A22 |  A21 |  A20 | (page within device)
*                          ----------------------------------------------------------------------          |
*                          |    5th     |  --- |  --- |  --- |  --- |  A31 |  A30 |  A29 |  A28 |          |
*                          ----------------------------------------------------------------------         ---
*
*                   The 5th byte is only necessary for devices with more than 65536 pages.
*********************************************************************************************************
*/

static  void  SecRd (void        *p_ctrlr_data_v,
                     void        *p_dest,
                     void        *p_dest_oos,
                     FS_SEC_NBR   sec_ix_phy,
                     FS_ERR      *p_err)
{
    FS_NAND_CTRLR_FILE_DATA        *p_ctrlr_data;
    FS_NAND_PART_DATA             *p_part_data;
    FS_ERR                         err_rtn;
    FS_NAND_CTRLR_ADDR_FMT_FLAGS   addr_fmt_flags;
    FS_NAND_PG_SIZE                sec_size;
    CPU_INT32U                     nbr_sec_per_pg;
    CPU_INT32U                     sec_offset_pg;
    CPU_INT32U                     pg_size;
    CPU_INT32U                     row_addr;
    CPU_INT16U                     col_addr;


    p_ctrlr_data      = (FS_NAND_CTRLR_FILE_DATA *)p_ctrlr_data_v;
    p_part_data       =  p_ctrlr_data->PartDataPtr;
    sec_size          =  p_ctrlr_data->SecSize;
    pg_size           =  p_part_data->PgSize;
    err_rtn           =  FS_ERR_NONE;

    FS_CTR_STAT_INC(p_ctrlr_data->Ctrs.StatRdCtr);

                                                                /* -------------------- ADDR CALC --------------------- */
    nbr_sec_per_pg = pg_size       / sec_size;
    sec_offset_pg  = sec_ix_phy    % nbr_sec_per_pg;
    row_addr       = sec_ix_phy    / nbr_sec_per_pg;
    col_addr       = sec_offset_pg * sec_size;
#if 0
    //printf("nand: reading %d from %08X:%08X\n", sec_size, row_addr, col_addr);

    Mem_Copy(p_dest, p_ctrlr_data->Data + row_addr * PAGE_SIZE + col_addr, sec_size);
#else
    p_ctrlr_data->BSP_Ptr->DataRd(p_ctrlr_data->BSPData, p_dest, row_addr, col_addr, sec_size, p_err);
#endif

    int16_t len = p_ctrlr_data->OOS_InfoTbl[sec_offset_pg].Len;
    col_addr = p_ctrlr_data->OOS_InfoTbl[sec_offset_pg].PgOffset;

#if 0    
    Mem_Copy(
        p_dest_oos, 
        p_ctrlr_data->Data + row_addr * PAGE_SIZE + col_addr, 
        len);
#else
    p_ctrlr_data->BSP_Ptr->DataRd(p_ctrlr_data->BSPData, p_dest_oos, row_addr, col_addr, len, p_err);
#endif

    SpareUnpack(p_ctrlr_data, p_dest_oos, sec_offset_pg);


                                                                /* -------------------- CHECK ECC --------------------- */
    if (p_ctrlr_data->CtrlrExtPtr != DEF_NULL && p_ctrlr_data->CtrlrExtPtr->ECC_Verify != DEF_NULL) {
        p_ctrlr_data->CtrlrExtPtr->ECC_Verify(p_ctrlr_data->CtrlrExtData,
                                              p_dest,
                                              p_dest_oos,
                                              p_ctrlr_data->OOS_SizePerSec,
                                             &err_rtn);
    }

   *p_err = err_rtn;

#if (FS_CFG_CTR_ERR_EN == DEF_ENABLED)
    switch (*p_err) {
        case FS_ERR_ECC_UNCORR:
             FS_CTR_ERR_INC(p_ctrlr_data->Ctrs.ErrUncorrECC_Ctr);
             break;

        case FS_ERR_ECC_CRITICAL_CORR:
             FS_CTR_ERR_INC(p_ctrlr_data->Ctrs.ErrCriticalCorrECC_Ctr);
             break;

        case FS_ERR_ECC_CORR:
             FS_CTR_ERR_INC(p_ctrlr_data->Ctrs.ErrCorrECC_Ctr);
             break;

        default:
             break;
    }
#endif


    //*p_err = FS_ERR_NONE;
}


/*
*********************************************************************************************************
*                                              OOSRdRaw()
*
* Description : Read raw OOS without ECC check.
*
* Argument(s) : p_ctrlr_data_v  Pointer to NAND controller data.
*               --------------  Argument validated by caller.
*
*               p_dest_oos      Pointer to destination OOS (out of sector data) buffer.
*               ----------      Argument validated by caller.
*
*               sec_ix_phy      Sector index for which OOS will be read.
*
*               offset          Offset in the OOS area.
*
*               len             Number of octets to read.
*
*               p_err           Pointer to variable that will receive the return error code from this function :
*               -----           Argument validated by caller.
*
*                                   FS_ERR_DEV_TIMEOUT  Device timeout.
*
* Return(s)   : none.
*
* Note(s)     : none
*********************************************************************************************************
*/

static  void  OOSRdRaw (void             *p_ctrlr_data_v,
                        void             *p_dest_oos,
                        FS_SEC_NBR        sec_ix_phy,
                        FS_NAND_PG_SIZE   offset,
                        FS_NAND_PG_SIZE   len,
                        FS_ERR           *p_err)
{
    FS_NAND_CTRLR_FILE_DATA        *p_ctrlr_data;
    FS_NAND_PART_DATA             *p_part_data;
    CPU_INT08U                    *p_spare_08;
    FS_NAND_CTRLR_ADDR_FMT_FLAGS   addr_fmt_flags;
    CPU_INT32U                     row_addr;
    CPU_INT08U                     nbr_sec_per_pg;
    CPU_INT08U                     sec_offset_pg;
    CPU_INT08U                     cmd;


    p_ctrlr_data = (FS_NAND_CTRLR_FILE_DATA *)p_ctrlr_data_v;
    p_part_data  =  p_ctrlr_data->PartDataPtr;

    FS_CTR_STAT_INC(p_ctrlr_data->Ctrs.StatOOSRdRawCtr);

                                                                /* --------------- ADDRESS CALCULATION ---------------- */
    nbr_sec_per_pg = p_part_data->PgSize / p_ctrlr_data->SecSize;
    sec_offset_pg  = sec_ix_phy          % nbr_sec_per_pg;
    row_addr       = sec_ix_phy          / nbr_sec_per_pg;

    p_spare_08  = (CPU_INT08U *)p_ctrlr_data->SpareBufPtr;
    uint32_t spare_len = p_ctrlr_data->OOS_InfoTbl[sec_offset_pg].Len;
    int16_t col_addr = p_ctrlr_data->OOS_InfoTbl[sec_offset_pg].PgOffset;
#if 1
    p_ctrlr_data->BSP_Ptr->DataRd(p_ctrlr_data->BSPData, p_spare_08, row_addr, col_addr, spare_len, p_err);
#else
    Mem_Copy(p_spare_08, p_ctrlr_data->Data + row_addr * PAGE_SIZE + col_addr,
            spare_len);
#endif

    SpareUnpack(p_ctrlr_data,
                p_spare_08,
                sec_offset_pg);

    p_spare_08 += offset;

    Mem_Copy(p_dest_oos, p_spare_08, len);
}


/*
*********************************************************************************************************
*                                             SpareRdRaw()
*
* Description : Read raw spare data without ECC check.
*
* Argument(s) : p_ctrlr_data_v  Pointer to NAND controller data.
*               --------------  Argument validated by caller.
*
*               p_dest_spare    Pointer to destination spare area data  buffer.
*               ------------    Argument validated by caller.
*
*               pg_ix_phy       Page index for which spare data will be read.
*
*               offset          Offset in the spare area.
*
*               len             Number of octets to read.
*
*               p_err           Pointer to variable that will receive the return error code from this function :
*               -----           Argument validated by caller.
*
*                                   FS_ERR_DEV_TIMEOUT  Device timeout.
*
* Return(s)   : none.
*
* Note(s)     : none
*********************************************************************************************************
*/

static  void  SpareRdRaw (void             *p_ctrlr_data_v,
                          void             *p_dest_spare,
                          FS_SEC_NBR        pg_ix_phy,
                          FS_NAND_PG_SIZE   offset,
                          FS_NAND_PG_SIZE   len,
                          FS_ERR           *p_err)
{
    FS_NAND_CTRLR_FILE_DATA  *p_ctrlr_data;


    p_ctrlr_data = (FS_NAND_CTRLR_FILE_DATA *)p_ctrlr_data_v;

    PgRdRaw(p_ctrlr_data_v,
            p_dest_spare,
            pg_ix_phy,
            offset + p_ctrlr_data->PartDataPtr->PgSize,
            len,
            p_err);
}


/*
*********************************************************************************************************
*                                             PgRdRaw()
*
* Description : Read raw page data without ECC check.
*
* Argument(s) : p_ctrlr_data_v  Pointer to NAND controller data.
*               --------------  Argument validated by caller.
*
*               p_dest          Pointer to destination data  buffer.
*               ------------    Argument validated by caller.
*
*               pg_ix_phy       Page index for which page data will be read.
*
*               offset          Offset in the page.
*
*               len             Number of octets to read.
*
*               p_err           Pointer to variable that will receive the return error code from this function :
*               -----           Argument validated by caller.
*
*                                   FS_ERR_DEV_TIMEOUT  Device timeout.
*
* Return(s)   : none.
*
* Note(s)     : none
*********************************************************************************************************
*/

static  void  PgRdRaw (void             *p_ctrlr_data_v,
                       void             *p_dest,
                       FS_SEC_NBR        pg_ix_phy,
                       FS_NAND_PG_SIZE   offset,
                       FS_NAND_PG_SIZE   len,
                       FS_ERR           *p_err)
{
    FS_NAND_CTRLR_FILE_DATA        *p_ctrlr_data;
    FS_NAND_PART_DATA             *p_part_data;
    FS_NAND_CTRLR_ADDR_FMT_FLAGS   addr_fmt_flags;
    CPU_INT08U                     cmd;


    p_ctrlr_data = (FS_NAND_CTRLR_FILE_DATA *)p_ctrlr_data_v;
    p_part_data  =  p_ctrlr_data->PartDataPtr;

    FS_CTR_STAT_INC(p_ctrlr_data->Ctrs.StatSpareRdRawCtr);
    //printf("PgRdRaw unimplemented\n");
#if 0
    Mem_Copy(p_dest, p_ctrlr_data->Data + pg_ix_phy * PAGE_SIZE + offset, len);
#else
    p_ctrlr_data->BSP_Ptr->DataRd(p_ctrlr_data->BSPData, p_dest, pg_ix_phy, offset, len, p_err);
#endif
    *p_err = FS_ERR_NONE;
#if 0
                                                                /* --------------- ADDRESS CALCULATION ---------------- */
    addr_fmt_flags =  FS_NAND_CTRLR_ADDR_FMT_COL |
                      FS_NAND_CTRLR_ADDR_FMT_ROW |
                     (p_ctrlr_data->PartDataPtr->PgSize == 512u ? FS_NAND_CTRLR_ADDR_FMT_SMALL_PG : 0u);

    AddrFmt(p_ctrlr_data,
            pg_ix_phy,
            offset,
            addr_fmt_flags,
           &addr[0]);

    p_bsp_api->ChipSelEn();

    if (p_ctrlr_data->PartDataPtr->PgSize == 512u) {            /* Small pg dev.                                        */
        if (offset >= 512u) {
            cmd = FS_NAND_CMD_RD_SETUP_ZONE_C;
        } else if (offset >= 256u) {
            cmd = FS_NAND_CMD_RD_SETUP_ZONE_B;
        } else {
            cmd = FS_NAND_CMD_RD_SETUP;                         /* Zone A.                                              */
        }
    } else {                                                    /* Large pg dev.                                        */
        cmd = FS_NAND_CMD_RD_SETUP;
    }

    FS_ERR_CHK(p_bsp_api->CmdWr(&cmd, 1u, p_err),
               p_bsp_api->ChipSelDis());
    FS_ERR_CHK(p_bsp_api->AddrWr(&addr[0], p_ctrlr_data->AddrSize, p_err),
               p_bsp_api->ChipSelDis());
    if (p_ctrlr_data->PartDataPtr->PgSize != 512u) {
        cmd = FS_NAND_CMD_RD_CONFIRM;
        FS_ERR_CHK(p_bsp_api->CmdWr(&cmd, 1u, p_err),
                   p_bsp_api->ChipSelDis());
    }

    p_bsp_api->WaitWhileBusy(p_ctrlr_data,                      /* Wait until ready.                                    */
                             FS_NAND_CtrlrGen_PollFnct,
                             FS_NAND_MAX_RD_us,
                             p_err);
    if (*p_err != FS_ERR_NONE) {
        p_bsp_api->ChipSelDis();
        return;
    }

    cmd = FS_NAND_CMD_RD_SETUP;
    FS_ERR_CHK(p_bsp_api->CmdWr(&cmd, 1u, p_err),
               p_bsp_api->ChipSelDis());


    FS_ERR_CHK(p_bsp_api->DataRd(p_dest, len, p_part_data->BusWidth, p_err),
               p_bsp_api->ChipSelDis());

    p_bsp_api->ChipSelDis();
#endif
}


/*
*********************************************************************************************************
*                                                SecWr()
*
* Description : Write data to NAND device sector from a buffer.
*
* Argument(s) : p_ctrlr_data_v  Pointer to NAND controller data.
*               --------------  Argument validated by caller.
*
*               p_src           Pointer to source data buffer.
*               -----           Argument validated by caller.
*
*               p_src_oos       Pointer to source OOS data buffer.
*               ---------       Argument validated by caller.
*
*               sec_ix_phy      Sector index which will be written.
*
*               p_err           Pointer to variable that will receive the return error code from this function :
*               -----           Argument validated by caller.
*
*                                   FS_ERR_DEV_IO       Device I/O error.
*                                   FS_ERR_DEV_TIMEOUT  Device timeout.
*                                   FS_ERR_NONE         Data read successfully.
*
*                                   -RETURNED BY p_ctrlr_data->CtrlrExtPtr->ECC_Calc()-
*                                   See p_ctrlr_data->CtrlrExtPtr->ECC_Calc() for additional return error codes.
*
* Return(s)   : none.
*
* Note(s)     : none
*********************************************************************************************************
*/

static  void  SecWr (void        *p_ctrlr_data_v,
                     void        *p_src,
                     void        *p_src_oos,
                     FS_SEC_NBR   sec_ix_phy,
                     FS_ERR      *p_err)
{

    FS_NAND_CTRLR_FILE_DATA        *p_ctrlr_data;
    FS_NAND_PART_DATA             *p_part_data;
    FS_NAND_PG_SIZE                ix;
    FS_NAND_CTRLR_ADDR_FMT_FLAGS   addr_fmt_flags;
    FS_NAND_PG_SIZE                col_addr;
    FS_NAND_PG_SIZE                sec_size;
    CPU_INT32U                     nbr_sec_per_pg;
    CPU_INT32U                     sec_offset_pg;
    CPU_INT32U                     pg_size;
    CPU_INT32U                     row_addr;
    CPU_INT08U                     cmd1;
    CPU_INT08U                     cmd2;
    CPU_INT08U                     sr;
    CPU_INT08U                     val;
    CPU_BOOLEAN                    sr_fail;


    p_ctrlr_data      = (FS_NAND_CTRLR_FILE_DATA *)p_ctrlr_data_v;
    p_part_data       =  p_ctrlr_data->PartDataPtr;
    sec_size          =  p_ctrlr_data->SecSize;
    pg_size           =  p_part_data->PgSize;

    FS_CTR_STAT_INC(p_ctrlr_data->Ctrs.StatWrCtr);
                                                                /* --------------- ADDRESS CALCULATION ---------------- */
    nbr_sec_per_pg = pg_size       / sec_size;
    sec_offset_pg  = sec_ix_phy    % nbr_sec_per_pg;
    row_addr       = sec_ix_phy    / nbr_sec_per_pg;
    col_addr       = sec_offset_pg * sec_size;


#if 0
    Mem_Copy(p_ctrlr_data->Data + row_addr * PAGE_SIZE + col_addr , p_src, sec_size);
#else
    p_ctrlr_data->BSP_Ptr->DataWr(p_ctrlr_data->BSPData, p_src, row_addr, col_addr, sec_size, p_err);
#endif

    if (p_ctrlr_data->CtrlrExtPtr != DEF_NULL && p_ctrlr_data->CtrlrExtPtr->ECC_Calc != DEF_NULL) {
        p_ctrlr_data->CtrlrExtPtr->ECC_Calc(p_ctrlr_data->CtrlrExtData,
                                            p_src,
                                            p_src_oos,
                                            p_ctrlr_data->OOS_SizePerSec,
                                            p_err);
        if (*p_err != FS_ERR_NONE) {
            //p_bsp_api->ChipSelDis();
            return;
        }
    }

                                                                /* ----------- PACK AND WR OOS IN SPARE REG ----------- */
    SparePack(p_ctrlr_data,
              p_src_oos,
              sec_offset_pg);

    int16_t len = p_ctrlr_data->OOS_InfoTbl[sec_offset_pg].Len;
    col_addr = p_ctrlr_data->OOS_InfoTbl[sec_offset_pg].PgOffset;

#if 0
    Mem_Copy(p_ctrlr_data->Data + row_addr * PAGE_SIZE + col_addr, p_src_oos, len);
#else
    p_ctrlr_data->BSP_Ptr->DataWr(p_ctrlr_data->BSPData, p_src_oos, row_addr, col_addr, len, p_err);
#endif
}


/*
*********************************************************************************************************
*                                              BlkErase()
*
* Description : Erase block of NAND device.
*
* Argument(s) : p_ctrlr_data_v  Pointer to NAND controller data.
*               --------------  Argument validated by caller.
*
*               blk_ix_phy      Block index that will be erased.
*
*               p_err           Pointer to variable that will receive the return error code from this function :
*               -----           Argument validated by caller.
*
*                                   FS_ERR_OP_FAILED    Blk erase failed.
*                                   FS_ERR_DEV_TIMEOUT  Device timeout.
*                                   FS_ERR_NONE         Block erased successfully.
*
* Return(s)   : none.
*
* Note(s)     : (1) Can be called directly by user through I/O Ctrl. Care must be taken NOT to erase bad
*                   blocks because the factory defect mark will be lost.
*********************************************************************************************************
*/

static  void  BlkErase (void        *p_ctrlr_data_v,
                        CPU_INT32U   blk_ix_phy,
                        FS_ERR      *p_err)
{

    FS_NAND_CTRLR_FILE_DATA        *p_ctrlr_data;
    FS_SEC_QTY                     nbr_sec_per_blk;
    FS_SEC_QTY                     sec_ix_phy;
    FS_NAND_CTRLR_ADDR_FMT_FLAGS   addr_fmt_flags;
    CPU_INT32U                     row_addr;
    CPU_INT08U                     cmd1;
    CPU_INT08U                     cmd2;
    CPU_INT08U                     nbr_sec_per_pg;
    CPU_INT08U                     sr;
    CPU_BOOLEAN                    sr_fail;


    p_ctrlr_data = (FS_NAND_CTRLR_FILE_DATA *)p_ctrlr_data_v;

    FS_CTR_STAT_INC(p_ctrlr_data->Ctrs.StatEraseCtr);
#if 0
    Mem_Set(p_ctrlr_data->Data + blk_ix_phy * PAGES_PER_BLOCK * PAGE_SIZE, 0xFF, PAGES_PER_BLOCK * PAGE_SIZE);
#else
    p_ctrlr_data->BSP_Ptr->BlkErase(p_ctrlr_data->BSPData, blk_ix_phy, p_err);
#endif
}


/*
*********************************************************************************************************
*                                               IO_Ctrl()
*
* Description : Perform NAND device I/O control operation.
*
* Argument(s) : p_ctrlr_data_v  Pointer to NAND controller data.
*               --------------  Argument validated by caller.
*
*               cmd             Control command.
*
*               p_arg           Pointer to command argument(s).
*               -----           Argument validated by caller.
*
*               p_err           Pointer to variable that will receive the return the error code from this function :
*               -----           Argument validated by caller.
*
*                                   FS_ERR_DEV_INVALID_IO_CTRL  I/O control operation unknown to driver.
*
*                                   ----------------------RETURNED BY ParamPgRd()-----------------------
*                                   See ParamPgRd() for additional return error codes.
*
* Return(s)   : none.
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  IO_Ctrl (void        *p_ctrlr_data_v,
                       CPU_INT08U   cmd,
                       void        *p_arg,
                       FS_ERR      *p_err)
{
    FS_NAND_CTRLR_FILE_DATA  *p_ctrlr_data;
    FS_NAND_IO_CTRL_DATA    *p_op_data;

    p_ctrlr_data = p_ctrlr_data_v;

    switch(cmd) {
    case FS_DEV_IO_CTRL_PHY_RD_PAGE:
        p_op_data = p_arg;
#if 0
        Mem_Copy(p_op_data->DataPtr, p_ctrlr_data->Data + p_op_data->IxPhy * PAGE_SIZE, p_ctrlr_data->PartDataPtr->PgSize);
#else
        p_ctrlr_data->BSP_Ptr->DataRd(p_ctrlr_data->BSPData, p_op_data->DataPtr, p_op_data->IxPhy, 0, p_ctrlr_data->PartDataPtr->PgSize, p_err);
#endif
        break;
    break;
    default:
        printf("ioctl unimplemented\n");
        *p_err = FS_ERR_DEV_INVALID_IO_CTRL;
        break;
    }
}


/*
*********************************************************************************************************
*********************************************************************************************************
*                                            LOCAL FUNCTIONS
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                              OOS_Setup()
*
* Description : Setup out of sector area. Creates OOS to spare mapping and allocates buffer to use for
*               spare area access.
*
* Argument(s) : p_ctrlr_data    Pointer to NAND generic controller data.
*               ------------    Argument validated by caller.
*
*               n_sec_per_pg    Number of sectors per NAND page.
*
*               p_err           Pointer to variable that will receive the return the error code from this function :
*               -----           Argument validated by caller.
*
*                                   FS_ERR_DEV_INVALID_LOW_PARAMS   Invalid spare segment start offset.
*                                   FS_ERR_MEM_ALLOC                Unable to allocate required heap space.
*
* Return(s)   : none.
*
* Note(s)     : none.
*
*********************************************************************************************************
*/

static  void OOS_Setup(FS_NAND_CTRLR_FILE_DATA  *p_ctrlr_data,
                       CPU_INT08U               n_sec_per_pg,
                       FS_ERR                  *p_err)
{
    CPU_SIZE_T                octets_reqd;
    CPU_INT08U                OOS_ix;
    FS_NAND_PG_SIZE           cur_pg_offset;
    FS_NAND_PG_SIZE           spare_seg_max_len;
    FS_NAND_FREE_SPARE_DATA  *free_spare_map;
    FS_NAND_PG_SIZE           nxt_spare_pos;
    FS_NAND_PG_SIZE           spare_size_per_sec;
    CPU_INT08U                spare_zone_ix;
    FS_NAND_PG_SIZE           spare_zone_free_space_rem;
    FS_NAND_PG_SIZE           OOS_size_rem;
    LIB_ERR                   err_lib;


                                                                /* ----------- ALLOC OOS SEGMENTS INFO TBL ------------ */
    p_ctrlr_data->OOS_InfoTbl = (FS_NAND_CTRLR_FILE_SPARE_SEG_INFO *)Mem_HeapAlloc(n_sec_per_pg * sizeof(FS_NAND_CTRLR_FILE_SPARE_SEG_INFO),
                                                                                  sizeof(CPU_DATA),
                                                                                 &octets_reqd,
                                                                                 &err_lib);
    if (err_lib != LIB_MEM_ERR_NONE) {
        FS_TRACE_DBG(("OOS_Setup(): Could not alloc mem for OOS Segment Info tbl: %d octets required.\r\n",
                      octets_reqd));
       *p_err = FS_ERR_MEM_ALLOC;
        return;
    }

                                                                /* -------------- CALC OOS SEGMENTS INFO -------------- */
    spare_size_per_sec = p_ctrlr_data->SpareTotalAvailSize / n_sec_per_pg;

    free_spare_map = p_ctrlr_data->PartDataPtr->FreeSpareMap;

    spare_zone_ix = 0u;
    spare_zone_free_space_rem = free_spare_map[0].OctetLen;
    spare_seg_max_len = 0u;

    cur_pg_offset  = p_ctrlr_data->PartDataPtr->PgSize;         /* First OOS begins at start of spare area.             */
    cur_pg_offset += free_spare_map[0].OctetOffset;
    for (OOS_ix = 0u; OOS_ix < n_sec_per_pg; OOS_ix++) {
        p_ctrlr_data->OOS_InfoTbl[OOS_ix].PgOffset = cur_pg_offset;
        if ((p_ctrlr_data->PartDataPtr->BusWidth == 16u) &&     /* Make sure seg doesn't start at invalid ix.           */
            (DEF_BIT_IS_SET(cur_pg_offset, DEF_BIT_00) == DEF_YES)) {
            FS_TRACE_DBG(("OOS_Setup(): OOS segment %d starts at odd index %d: need to fix FreeSpareMap.\r\n",
                          OOS_ix,
                          cur_pg_offset));
           *p_err = FS_ERR_DEV_INVALID_LOW_PARAMS;
            return;
        }

        p_ctrlr_data->OOS_InfoTbl[OOS_ix].Len   = 0u;

        OOS_size_rem = spare_size_per_sec;
        while (OOS_size_rem != 0u){
            if (spare_zone_free_space_rem >= OOS_size_rem) {    /* Cur spare zone has enough free space.                */
                spare_zone_free_space_rem -= OOS_size_rem;
                p_ctrlr_data->OOS_InfoTbl[OOS_ix].Len += OOS_size_rem;
                cur_pg_offset += OOS_size_rem;
                OOS_size_rem = 0u;
            } else {                                            /* Cur spare zone does not have enough free space.      */
                                                                /* Use remainder of spare zone.                         */
                p_ctrlr_data->OOS_InfoTbl[OOS_ix].Len += spare_zone_free_space_rem;
                OOS_size_rem  -= spare_zone_free_space_rem;
                cur_pg_offset += spare_zone_free_space_rem;
                spare_zone_free_space_rem = 0u;
            }

            if (spare_zone_free_space_rem == 0u) {              /* Switch to nxt spare zone.                            */
                nxt_spare_pos = free_spare_map[spare_zone_ix].OctetOffset + free_spare_map[spare_zone_ix].OctetLen;
                spare_zone_ix++;
                spare_zone_free_space_rem = free_spare_map[spare_zone_ix].OctetLen;
                cur_pg_offset += free_spare_map[spare_zone_ix].OctetOffset - nxt_spare_pos;

                if (OOS_size_rem != 0u) {
                    p_ctrlr_data->OOS_InfoTbl[OOS_ix].Len += free_spare_map[spare_zone_ix].OctetOffset - nxt_spare_pos;
                }
            }
        }

        if (p_ctrlr_data->OOS_InfoTbl[OOS_ix].Len > spare_seg_max_len) {
            spare_seg_max_len = p_ctrlr_data->OOS_InfoTbl[OOS_ix].Len;
        }

    }

                                                                /* ----------------- ALLOC SPARE BUF ------------------ */
    p_ctrlr_data->SpareBufPtr = Mem_HeapAlloc(spare_seg_max_len,
                                              FS_CFG_BUF_ALIGN_OCTETS,
                                             &octets_reqd,
                                             &err_lib);
    if (err_lib != LIB_MEM_ERR_NONE) {
        FS_TRACE_DBG(("OOS_Setup(): Could not alloc mem for OOS data buffer: %d octets required.\r\n",
                      octets_reqd));
       *p_err = FS_ERR_MEM_ALLOC;
        return;
    }

}


/*
*********************************************************************************************************
*                                              StatusRd()
*
* Description : Read NAND status register.
*
* Argument(s) : p_ctrlr_data    Pointer to NAND ctrlr data.
*               ------------    Argument validated by caller.
*
* Return(s)   : Status register value.
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  CPU_INT08U  StatusRd (FS_NAND_CTRLR_FILE_DATA  *p_ctrlr_data)
{
    CPU_INT08U  cmd;
    CPU_INT08U  sr;
    FS_ERR      err;

#if 0
    cmd = FS_NAND_CMD_RDSTATUS;
    p_ctrlr_data->BSP_Ptr->CmdWr(&cmd, 1u, &err);
    if (err != FS_ERR_NONE) {
        return (FS_NAND_SR_FAIL);
    }
    p_ctrlr_data->BSP_Ptr->DataRd((void *)&sr, 1u, 8u, &err);
    if (err != FS_ERR_NONE) {
        return (FS_NAND_SR_FAIL);
    }
#endif
    return (sr);
}


/*
*********************************************************************************************************
*                                               AddrFmt()
*
* Description : Format address according to specified flags.
*
* Argument(s) : p_ctrlr_data    Pointer to NAND ctrlr data.
*               ------------    Argument validated by caller.
*
*               pg_ix           Page index. Use first page index of block for block-related commands or 0
*                               when the row address is not required.
*
*               pg_offset       Page offset in octets. Use 0 if column address is not required.
*
*               addr_fmt_flags  Flags that control how address is generated:
*
*                                   FS_NAND_CTRLR_ADDR_FMT_COL          Include column address.
*                                   FS_NAND_CTRLR_ADDR_FMT_ROW          Include row address.
*                                   FS_NAND_CTRLR_ADDR_FMT_SMALL_PG     Use small-page device addressing.
*
*               p_addr          Pointer to array that will receive the formatted address.
*
* Return(s)   : none.
*
* Note(s)     : none.
*********************************************************************************************************
*/
#if 0
static  void  AddrFmt (FS_NAND_CTRLR_FILE_DATA        *p_ctrlr_data,
                       FS_SEC_QTY                     pg_ix,
                       FS_NAND_PG_SIZE                pg_offset,
                       FS_NAND_CTRLR_ADDR_FMT_FLAGS   addr_fmt_flags,
                       CPU_INT08U                    *p_addr)
{
    CPU_INT08U       addr_ix;
    CPU_INT08U       ix;
    CPU_INT08U       bus_width;
    FS_NAND_PG_SIZE  col_addr;


    bus_width = p_ctrlr_data->PartDataPtr->BusWidth;
    col_addr  = pg_offset;

    addr_ix = 0u;
                                                                /* --------------------- COL ADDR --------------------- */
    if (DEF_BIT_IS_SET(addr_fmt_flags, FS_NAND_CTRLR_ADDR_FMT_COL) == DEF_YES) {
        if (DEF_BIT_IS_SET(addr_fmt_flags, FS_NAND_CTRLR_ADDR_FMT_SMALL_PG) == DEF_YES) {
                                                                /* Small pg dev.                                        */
            if (col_addr >= 512u) {                             /* Zone C.                                              */
                col_addr -= 512u;
            } else {
                if ((col_addr >= 256u) &&                       /* Zone B.                                              */
                    (bus_width != 16u)) {
                    col_addr -= 256u;
                }
            }
        }

        if (bus_width == 16u) {
            col_addr >>= 1u;
        }

        for (ix = 0u; ix < p_ctrlr_data->ColAddrSize; ix++) {
            p_addr[addr_ix] = (col_addr >> (DEF_OCTET_NBR_BITS * ix)) & DEF_OCTET_MASK;
            addr_ix++;
        }
    }


                                                                /* --------------------- ROW ADDR --------------------- */
    if (DEF_BIT_IS_SET(addr_fmt_flags, FS_NAND_CTRLR_ADDR_FMT_ROW) == DEF_YES) {
        for (ix = 0u; ix < p_ctrlr_data->RowAddrSize; ix++) {
            p_addr[addr_ix] = (pg_ix >> (DEF_OCTET_NBR_BITS * ix)) & DEF_OCTET_MASK;
            addr_ix++;
        }
    }
}
#endif

/*
*********************************************************************************************************
*                                              SparePack()
*
* Description : Pack spare data for specified segment according to spare free map.
*
* Argument(s) : p_ctrlr_data    Pointer to a buffer containing the spare data.
*               ------------    Argument validated by caller.
*
*               p_spare         Pointer to spare buffer.
*
*               spare_seg_ix    Index of spare segment.
*
* Return(s)   : none.
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  SparePack (FS_NAND_CTRLR_FILE_DATA  *p_ctrlr_data,
                         void                    *p_spare,
                         CPU_INT08U               spare_seg_ix)
{
    FS_NAND_FREE_SPARE_DATA           *free_spare_map;
    FS_NAND_CTRLR_FILE_SPARE_SEG_INFO   spare_seg_info;
    FS_NAND_PG_SIZE                    spare_seg_start;
    FS_NAND_PG_SIZE                    spare_seg_end;
    FS_NAND_PG_SIZE                    spare_pos;
    FS_NAND_PG_SIZE                    spare_notch_pos;
    FS_NAND_PG_SIZE                    spare_notch_len;
    CPU_INT08U                         spare_zone_ix;


    free_spare_map = p_ctrlr_data->PartDataPtr->FreeSpareMap;


    spare_seg_info  = p_ctrlr_data->OOS_InfoTbl[spare_seg_ix];
    spare_seg_start = spare_seg_info.PgOffset - p_ctrlr_data->PartDataPtr->PgSize;
    spare_seg_end   = spare_seg_start + spare_seg_info.Len;
    spare_pos       = spare_seg_start;
    spare_zone_ix   = 0u;

    spare_notch_len = free_spare_map[0].OctetOffset;
    spare_notch_pos = 0u;

    while (spare_pos < spare_seg_end) {
        if (spare_notch_pos >= spare_pos) {                     /* Notch after cur pos ...                              */
            if (spare_notch_pos < spare_seg_end) {              /* ... and in cur seg.                                  */
                spare_pos  += spare_notch_pos - spare_pos;
                SpareSplit(p_spare,
                           spare_seg_info.Len,
                           spare_pos - spare_seg_start,
                           spare_notch_len);
                spare_pos += spare_notch_len;
            } else {                                            /* ... and after cur seg.                               */
                spare_pos = spare_seg_end;                      /* Done.                                                */
            }
        }

                                                                /* Continue with nxt notch.                             */
        spare_notch_pos = free_spare_map[spare_zone_ix].OctetOffset +
                          free_spare_map[spare_zone_ix].OctetLen;
        spare_zone_ix++;
        if (free_spare_map[spare_zone_ix].OctetOffset == FS_NAND_PG_IX_INVALID) {
            spare_pos = spare_seg_end;                          /* No more spare notches... done.                       */
        } else {
            spare_notch_len = free_spare_map[spare_zone_ix].OctetOffset - spare_notch_pos;
        }

    }
}


/*
*********************************************************************************************************
*                                             SpareUnpack()
*
* Description : Unpack spare data for specified segment according to spare free map.
*
* Argument(s) : p_ctrlr_data    Pointer to a buffer containing the spare data.
*               ------------    Argument validated by caller.
*
*               p_spare         Pointer to spare buffer.
*
*               spare_seg_ix    Index of spare segment.
*
* Return(s)   : none.
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  SpareUnpack (FS_NAND_CTRLR_FILE_DATA  *p_ctrlr_data,
                           void                    *p_spare,
                           CPU_INT08U               spare_seg_ix)
{
    FS_NAND_FREE_SPARE_DATA           *free_spare_map;
    FS_NAND_CTRLR_FILE_SPARE_SEG_INFO   spare_seg_info;
    FS_NAND_PG_SIZE                    spare_seg_rstart;
    CPU_INT32S                         spare_seg_rend;
    CPU_INT32S                         spare_pos;
    FS_NAND_PG_SIZE                    spare_notch_rpos;
    FS_NAND_PG_SIZE                    spare_notch_len;
    CPU_INT08U                         spare_zone_ix;


    free_spare_map = p_ctrlr_data->PartDataPtr->FreeSpareMap;


                                                                /* Find last spare zone.                                */
    spare_zone_ix = 0u;
    while (free_spare_map[spare_zone_ix].OctetOffset != FS_NAND_PG_IX_INVALID) {
        spare_zone_ix++;
    }
    spare_zone_ix--;


    spare_seg_info   = p_ctrlr_data->OOS_InfoTbl[spare_seg_ix];
    spare_seg_rend   = spare_seg_info.PgOffset           -
                       p_ctrlr_data->PartDataPtr->PgSize -
                       1u;
    spare_seg_rstart = spare_seg_rend + spare_seg_info.Len;
    spare_pos        = spare_seg_rstart;

    spare_notch_rpos = p_ctrlr_data->PartDataPtr->SpareSize - 1u;
    spare_notch_len  =  p_ctrlr_data->PartDataPtr->SpareSize -
                       (free_spare_map[spare_zone_ix].OctetOffset +
                        free_spare_map[spare_zone_ix].OctetLen);

    while (spare_pos > spare_seg_rend) {
        if (spare_notch_rpos <= spare_pos) {                    /* Notch before cur pos ...                             */
            if (spare_notch_rpos > spare_seg_rend) {            /* ... and in cur seg.                                  */
                spare_pos = spare_notch_rpos - spare_notch_len + 1u;
                SpareJoin(p_spare,
                          spare_seg_info.Len,
                          spare_pos - (spare_seg_rend + 1u),
                          spare_notch_len);
            } else {                                            /* ... and after cur seg.                               */
                spare_pos = spare_seg_rend;                     /* Done.                                                */
            }
        }

        spare_notch_rpos = free_spare_map[spare_zone_ix].OctetOffset - 1u;
        if (spare_zone_ix >= 1u) {
            spare_zone_ix--;
            spare_notch_len  =  spare_notch_rpos -
                               (free_spare_map[spare_zone_ix].OctetOffset +
                                free_spare_map[spare_zone_ix].OctetLen) + 1u;
        } else {
            spare_pos = spare_seg_rend;                         /* No more notches... done.                             */
        }

    }
}


/*
*********************************************************************************************************
*                                             SpareSplit()
*
* Description : Split spare buffer at specified spare notch.
*
* Argument(s) : p_spare     Pointer to a buffer containing the spare data.
*               -------     Argument validated by caller.
*
*               buf_len     Length on which to apply the operation.
*
*               pos         Start position of the spare notch to create.
*
*               len         Length of the spare notch to create.
*
* Return(s)   : none.
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  SpareSplit (void             *p_spare,
                          FS_NAND_PG_SIZE   buf_len,
                          FS_NAND_PG_SIZE   pos,
                          FS_NAND_PG_SIZE   len)
{
    CPU_INT08U       *p_spare_08;
    FS_NAND_PG_SIZE   i;

    p_spare_08 = (CPU_INT08U *)p_spare;

    for (i = buf_len - len - 1u; i != pos; i--) {
        p_spare_08[i + len] = p_spare_08[i];
    }
    p_spare_08[pos + len] = p_spare_08[pos];

    for (i = pos; i < pos + len; i++) {
        p_spare_08[i] = 0xFFu;
    }
}


/*
*********************************************************************************************************
*                                              SpareJoin()
*
* Description : Join spare buffer at specified spare notch.
*
* Argument(s) : p_spare     Pointer to a buffer containing the spare data.
*               -------     Argument validated by caller.
*
*               buf_len     Length on which to apply the operation.
*
*               pos         Start position of the spare notch to eliminate.
*
*               len         Length of the spare notch to eliminate.
*
* Return(s)   : none.
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  SpareJoin (void             *p_spare,
                         FS_NAND_PG_SIZE   buf_len,
                         FS_NAND_PG_SIZE   pos,
                         FS_NAND_PG_SIZE   len)
{
    CPU_INT08U       *p_spare_08;
    FS_NAND_PG_SIZE   i;


    p_spare_08 = (CPU_INT08U *)p_spare;

    for (i = pos; i < buf_len - len; i++) {
        p_spare_08[i] = p_spare_08[i + len];
    }
    for (i = buf_len - len; i < buf_len; i++) {
        p_spare_08[i] = 0xFFu;
    }
}


/*
*********************************************************************************************************
*                                               ParamPgRd()
*
* Description : Read parameter page of ONFI devices.
*
* Argument(s) : p_ctrlr_data_v  Pointer to a buffer containing the controller data.
*               --------------  Argument validated by caller.
*
*               p_param_pg      Pointer to the parameter page structure.
*
*               p_err           Pointer to variable that will receive the return the error code :
*               -----           Argument validated by caller.
*
*                                   FS_ERR_DEV_TIMEOUT  Timeout occurred waiting for device to become ready.
*
* Return(s)   : none.
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  ParamPgRd (void                              *p_ctrlr_data_v,
                         FS_NAND_RD_PARAM_PG_IO_CTRL_DATA  *p_param_pg,
                         FS_ERR                            *p_err)
{
#if 0
    FS_NAND_CTRLR_FILE_DATA     *p_ctrlr_data;
    //FS_NAND_CTRLR_GEN_BSP_API  *p_bsp_api;
    CPU_INT08U                  addr[FS_NAND_CTRLR_GEN_ADDR_MAX_LEN];
    CPU_INT08U                  cmd1;
    CPU_INT08U                  cmd2;
    CPU_DATA                    ix;


    p_ctrlr_data = (FS_NAND_CTRLR_GEN_DATA *)p_ctrlr_data_v;
    p_bsp_api    =  p_ctrlr_data->BSP_Ptr;

    cmd1 = FS_NAND_CMD_RD_PARAM_PG;

    addr[0] = 0;

    p_bsp_api->ChipSelEn();

    FS_ERR_CHK(p_bsp_api->CmdWr (&cmd1,    1u, p_err),
               p_bsp_api->ChipSelDis());
    FS_ERR_CHK(p_bsp_api->AddrWr(&addr[0], 1u, p_err),          /* Sec addr.                                            */
               p_bsp_api->ChipSelDis());

    p_bsp_api->WaitWhileBusy(p_ctrlr_data_v,                    /* Wait until rdy.                                      */
                             FS_NAND_CtrlrGen_PollFnct,
                             FS_NAND_MAX_RD_us,
                             p_err);
    if (*p_err != FS_ERR_NONE) {
        p_bsp_api->ChipSelDis();
        FS_TRACE_DBG(("fs_dev_nand_ctrlr_gen.c : ParamPgRd(): Timeout occurred when sending command.\r\n"));
        return;
    }

                                                                /* Calculate column.                                    */
    if (p_param_pg->RelAddr != 0) {
        for (ix = 0u; ix < FS_NAND_CTRLR_GEN_ROW_ADDR_MAX_LEN; ix++) {
            addr[ix] = (p_param_pg->RelAddr >> (DEF_OCTET_NBR_BITS * ix)) & DEF_OCTET_MASK;
        }

        cmd1 = FS_NAND_CMD_CHNGRDCOL_SETUP;
        cmd2 = FS_NAND_CMD_CHNGRDCOL_CONFIRM;

        FS_ERR_CHK(p_bsp_api->CmdWr(&cmd1, 1u, p_err),
                   p_bsp_api->ChipSelDis());
        FS_ERR_CHK(p_bsp_api->AddrWr(&addr[0], FS_NAND_CTRLR_GEN_ROW_ADDR_MAX_LEN, p_err),
                   p_bsp_api->ChipSelDis());
        FS_ERR_CHK(p_bsp_api->CmdWr(&cmd2, 1u, p_err),
                   p_bsp_api->ChipSelDis());
    }

    cmd1 = FS_NAND_CMD_RD_SETUP;                                /* Switch back to rd mode.                              */
    FS_ERR_CHK(p_bsp_api->CmdWr(&cmd1, 1u, p_err),
               p_bsp_api->ChipSelDis());
                                                                /* Rd data from parameter page.                         */
    p_bsp_api->DataRd(p_param_pg->DataPtr, FS_NAND_PART_ONFI_PARAM_PAGE_LEN, 8u, p_err);

    p_bsp_api->ChipSelDis();
#endif
}


/*
*********************************************************************************************************
*                                               ExtReg()
*
* Description : Register and initialize generic controller extension module if needed.
*
* Argument(s) : p_ext   Pointer to NAND generic controller extension module.
*
*               p_err   Pointer to variable that will receive the return the error code from this function :
*               -----   Argument validated by caller.
*
*                           FS_ERR_DEV_INVALID_CFG  Maximum number of registered extension modules
*                                                   exceeded.
*
*                           ----------------------RETURNED BY p_ext->Init()-----------------------
*                           See p_ext->Init() for additional return error codes.
*
* Return(s)   : none.
*
* Note(s)     : none.
*
*********************************************************************************************************
*/

static  void ExtReg (const FS_NAND_CTRLR_FILE_EXT  *p_ext,
                           FS_ERR                 *p_err)
{
    CPU_BOOLEAN  ext_mod_init;
    CPU_INT08U   ext_mod_ix;
    CPU_INT08U   ext_mod_free_slot_ix;
    CPU_SR_ALLOC();


    ext_mod_init         = DEF_NO;
    ext_mod_free_slot_ix = (CPU_INT08U)-1;
    CPU_CRITICAL_ENTER();
    for (ext_mod_ix = 0u; ext_mod_ix < FS_NAND_CTRLR_FILE_MAX_EXT_MOD; ext_mod_ix++) {
        if (FS_NAND_CtrlrMmap_ListExtMod[ext_mod_ix] == p_ext) {
            ext_mod_init = DEF_YES;
        } else {
            if (FS_NAND_CtrlrMmap_ListExtMod[ext_mod_ix] == DEF_NULL) {
                ext_mod_free_slot_ix = ext_mod_ix;
            }
        }
    }

    if (ext_mod_init == DEF_NO) {                               /* Need to init ctrlr ext -- add to list.               */
        if (ext_mod_free_slot_ix != (CPU_INT08U)-1) {
            FS_NAND_CtrlrMmap_ListExtMod[ext_mod_free_slot_ix] = p_ext;
            if (ext_mod_init == DEF_NO) {
                p_ext->Init(p_err);
                if (*p_err != FS_ERR_NONE) {
                    CPU_CRITICAL_EXIT();
                    return;
                }
            }
        } else {
            CPU_CRITICAL_EXIT();
            FS_NAND_TRACE_DBG(("(fs_nand_ctrlr_gen) ExtReg(): Maximum number of NAND generic controller"
                               " extension modules exceeded.\r\n"));
           *p_err = FS_ERR_DEV_INVALID_CFG;
            return;
        }
    }
    CPU_CRITICAL_EXIT();
}


/*
*********************************************************************************************************
*                                              DataFree()
*
* Description : Free a NAND ctrlr generic data object.
*
* Argument(s) : p_ctrlr_data    Pointer to a nand ctrlr generic data object.
*               ------------    Argument validated by caller.
*
* Return(s)   : none.
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  DataFree (FS_NAND_CTRLR_FILE_DATA  *p_ctrlr_data)
{
    CPU_SR_ALLOC();


    CPU_CRITICAL_ENTER();
                                                                /* Add to free pool.                                    */
    p_ctrlr_data->NextPtr          = FS_NAND_CtrlrMmap_ListFreePtr;
    FS_NAND_CtrlrMmap_ListFreePtr   = p_ctrlr_data;
    CPU_CRITICAL_EXIT();
}


/*
*********************************************************************************************************
*                                               DataGet()
*
* Description : Allocate & initialize a NAND generic controller data object.
*
* Argument(s) : none.
*
* Return(s)   : Pointer to a NAND generic controller data object, if NO errors.
*               DEF_NULL,                                         otherwise.
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  FS_NAND_CTRLR_FILE_DATA  *DataGet (void)
{
    FS_NAND_CTRLR_FILE_DATA  *p_nand_ctrlr_gen_data;
    CPU_SIZE_T               octets_reqd;
    LIB_ERR                  alloc_err;
    CPU_SR_ALLOC();


                                                                /* --------------------- ALLOC DATA ------------------- */
    CPU_CRITICAL_ENTER();
    if (FS_NAND_CtrlrMmap_ListFreePtr == (FS_NAND_CTRLR_FILE_DATA *)0) {
        p_nand_ctrlr_gen_data = (FS_NAND_CTRLR_FILE_DATA *)Mem_HeapAlloc(sizeof(FS_NAND_CTRLR_FILE_DATA),
                                                                        sizeof(CPU_INT32U),
                                                                       &octets_reqd,
                                                                       &alloc_err);
        if (p_nand_ctrlr_gen_data == DEF_NULL) {
            CPU_CRITICAL_EXIT();
            FS_TRACE_DBG(("(fs_nand_ctrlr_gen) DataGet(): Could not alloc mem for NAND Generic Ctrlr data: %d octets required.\r\n", octets_reqd));
            return (DEF_NULL);
        }
        (void)alloc_err;

#if ((FS_CFG_CTR_STAT_EN == DEF_ENABLED) || \
     (FS_CFG_CTR_ERR_EN  == DEF_ENABLED))
        FS_NAND_CtrlrFile_CtrsTbl[FS_NAND_CtrlrFile_UnitCtr] = &p_nand_ctrlr_gen_data->Ctrs;
#endif

        FS_NAND_CtrlrFile_UnitCtr++;



    } else {
        p_nand_ctrlr_gen_data        = FS_NAND_CtrlrMmap_ListFreePtr;
        FS_NAND_CtrlrMmap_ListFreePtr = FS_NAND_CtrlrMmap_ListFreePtr->NextPtr;
    }
    CPU_CRITICAL_EXIT();


                                                                /* ---------------------- CLR DATA -------------------- */
    p_nand_ctrlr_gen_data->CtrlrExtPtr  = DEF_NULL;
    p_nand_ctrlr_gen_data->CtrlrExtData = DEF_NULL;
    p_nand_ctrlr_gen_data->PartDataPtr  = DEF_NULL;
    p_nand_ctrlr_gen_data->NextPtr      = DEF_NULL;


    return (p_nand_ctrlr_gen_data);
}

