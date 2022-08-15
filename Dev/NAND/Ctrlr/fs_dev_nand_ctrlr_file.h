#ifndef  FS_NAND_CTRLR_FILE_MODULE_PRESENT
#define  FS_NAND_CTRLR_FILE_MODULE_PRESENT


#include  <Source/ecc.h>
#include  "../fs_dev_nand.h"



#ifdef   FS_NAND_CTRLR_FILE_MODULE
#define  FS_NAND_CTRLR_FILE_MOD_EXT
#else
#define  FS_NAND_CTRLR_FILE_MOD_EXT  extern
#endif


#define  FS_NAND_CTRLR_FILE_CTRS_TBL_SIZE  4u

typedef  struct {
    void  *(*Open)           (const char* FileName,
                            FS_ERR       *p_err);

    void  (*Close)          (void *BspData);

    void  (*DataWr)         (void         *p_bsp_data,
                             const void   *p_src,               /* Write data cycle(s).                                 */
                             CPU_INT32U    row_address,
                             CPU_INT32U    col_address,
                             CPU_SIZE_T    cnt,
                             FS_ERR       *p_err);

    void  (*DataRd)         (void         *p_bsp_data,
                             void         *p_dest,              /* Read data.                                           */
                             CPU_INT32U    row_address,
                             CPU_INT32U    col_address,
                             CPU_SIZE_T    cnt,
                             FS_ERR       *p_err);

    void  (*BlkErase)       (void       *p_bsp_data,
                            CPU_INT32U  blkIdx,
                            FS_ERR     *p_err);

} FS_NAND_CTRLR_FILE_BSP_API;

typedef  struct {
    FS_NAND_PG_SIZE  PgOffset;                                  /* Offset in pg of spare seg.                           */
    FS_NAND_PG_SIZE  Len;                                       /* Len in bytes of spare seg.                           */
} FS_NAND_CTRLR_FILE_SPARE_SEG_INFO;


#if ((FS_CFG_CTR_STAT_EN == DEF_ENABLED) || \
     (FS_CFG_CTR_ERR_EN  == DEF_ENABLED))
typedef  struct  fs_nand_ctrlr_file_ctrs {
#if (FS_CFG_CTR_STAT_EN == DEF_ENABLED)                         /* -------------------- STAT CTRS --------------------- */
    FS_CTR                             StatRdCtr;               /* Nbr of sec rd.                                       */
    FS_CTR                             StatWrCtr;               /* Nbr of sec wr.                                       */
    FS_CTR                             StatEraseCtr;            /* Nbr of blk erase.                                    */
    FS_CTR                             StatSpareRdRawCtr;       /* Nbr of raw spare rd.                                 */
    FS_CTR                             StatOOSRdRawCtr;         /* Nbr of raw OOS rd.                                   */
#endif

#if (FS_CFG_CTR_ERR_EN == DEF_ENABLED)                          /* --------------------- ERR CTRS --------------------- */
    FS_CTR                             ErrCorrECC_Ctr;          /* Nbr of correctable ECC rd errs.                      */
    FS_CTR                             ErrCriticalCorrECC_Ctr;  /* Nbr of critical correctable ECC rd errs.             */
    FS_CTR                             ErrUncorrECC_Ctr;        /* Nbr of uncorrectable ECC rd errs.                    */
    FS_CTR                             ErrWrCtr;                /* Nbr of wr failures.                                  */
    FS_CTR                             ErrEraseCtr;             /* Nbr of erase failures.                               */
#endif
} FS_NAND_CTRLR_FILE_CTRS;
#endif


typedef  struct  fs_nand_ctrlr_file_ext   FS_NAND_CTRLR_FILE_EXT;
typedef  struct  fs_nand_ctrlr_file_data  FS_NAND_CTRLR_FILE_DATA;

struct fs_nand_ctrlr_file_data {
    FS_NAND_PART_API                  *PartPtr;                 /* Ptr to part layer interface.                         */
    FS_NAND_PART_DATA                 *PartDataPtr;             /* Ptr to part layer data.                              */


    FS_NAND_CTRLR_FILE_BSP_API         *BSP_Ptr;                 /* Ptr to ctrlr BSP.                                    */
    void                               *BSPData;

    FS_NAND_PG_SIZE                    SecSize;                 /* Size in octets of sec.                               */
    FS_NAND_PG_SIZE                    SpareTotalAvailSize;     /* Nbr of avail spare bytes.                            */

    FS_NAND_PG_SIZE                    OOS_SizePerSec;          /* Size in octets of OOS area per sec.                  */

    FS_NAND_CTRLR_FILE_SPARE_SEG_INFO  *OOS_InfoTbl;             /* OOS segments info tbl.                               */

    void                              *SpareBufPtr;             /* Ptr to OOS buf.                                      */

    void                              *CtrlrExtData;            /* Pointer to ctrlr ext data.                           */
    const  FS_NAND_CTRLR_FILE_EXT      *CtrlrExtPtr;             /* Pointer to ctrlr ext.                                */

#if ((FS_CFG_CTR_STAT_EN == DEF_ENABLED) || \
     (FS_CFG_CTR_ERR_EN == DEF_ENABLED))
    FS_NAND_CTRLR_FILE_CTRS             Ctrs;
#endif

    FS_NAND_CTRLR_FILE_DATA            *NextPtr;
};

struct fs_nand_ctrlr_file_ext {
    void              (*Init)        (FS_ERR                  *p_err);

    void             *(*Open)        (FS_NAND_CTRLR_FILE_DATA  *p_ctrlr_data,
                                      void                    *p_ext_cfg,
                                      FS_ERR                  *p_err);

    void              (*Close)       (void                    *p_ext_data);

    FS_NAND_PG_SIZE   (*Setup)       (FS_NAND_CTRLR_FILE_DATA  *p_ctrlr_data,
                                      void                    *p_ext_data,
                                      FS_ERR                  *p_err);

    void              (*RdStatusChk) (void                    *p_ext_data,
                                      FS_ERR                  *p_err);

    void              (*ECC_Calc)    (void                    *p_ext_data,
                                      void                    *p_sec_buf,
                                      void                    *p_oos_buf,
                                      FS_NAND_PG_SIZE          oos_size,
                                      FS_ERR                  *p_err);

    void              (*ECC_Verify)  (void                    *p_ext_data,
                                      void                    *p_sec_buf,
                                      void                    *p_oos_buf,
                                      FS_NAND_PG_SIZE          oos_size,
                                      FS_ERR                  *p_err);
};

#define  FS_NAND_CTRLR_FILE_CFG_FIELDS                                           \
    FS_NAND_CTRLR_FILE_CFG_FIELD(FS_NAND_CTRLR_FILE_EXT, *CtrlrExt   , DEF_NULL) \
    FS_NAND_CTRLR_FILE_CFG_FIELD(void                 , *CtrlrExtCfg, DEF_NULL)  \
    FS_NAND_CTRLR_FILE_CFG_FIELD(const char,            *FileName, DEF_NULL)


#define FS_NAND_CTRLR_FILE_CFG_FIELD(type, name, dftl_val) type name;
typedef  struct  fs_nand_ctrlr_file_cfg {
    FS_NAND_CTRLR_FILE_CFG_FIELDS
} FS_NAND_CTRLR_FILE_CFG;
#undef  FS_NAND_CTRLR_FILE_CFG_FIELD

extern  const  FS_NAND_CTRLR_FILE_CFG  FS_NAND_CtrlrFile_DfltCfg;



extern  const              FS_NAND_CTRLR_API        FS_NAND_CtrlrFile;
FS_NAND_CTRLR_FILE_MOD_EXT  FS_CTR                   FS_NAND_CtrlrFile_UnitCtr;
#if ((FS_CFG_CTR_STAT_EN == DEF_ENABLED) || \
     (FS_CFG_CTR_ERR_EN  == DEF_ENABLED))
FS_NAND_CTRLR_FILE_MOD_EXT  FS_NAND_CTRLR_FILE_CTRS  *FS_NAND_CtrlrFile_CtrsTbl[FS_NAND_CTRLR_FILE_CTRS_TBL_SIZE];
#endif


#define  FS_NAND_PART_FILE_CFG_FIELDS                                                                                                                                          \
    FS_NAND_PART_FILE_CFG_FIELD(FS_NAND_FREE_SPARE_DATA , *FreeSpareMap    , DEF_NULL                        /* Pointer to the map of available bytes in spare area. */)

#define  FS_NAND_PART_FILE_CFG_FIELD(type, name, dftl_val)  type name;
typedef struct fs_nand_part_file_cfg {
    FS_NAND_PART_FILE_CFG_FIELDS
} FS_NAND_PART_FILE_CFG;
#undef   FS_NAND_PART_FILE_CFG_FIELD


#endif  /* FS_NAND_CTRLR_GEN_MODULE_PRESENT */

