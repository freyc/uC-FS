#include "fs_exfat.h"

typedef struct {
    CPU_INT32U reserved;
} FS_EXFAT_DATA;


static  MEM_POOL  FS_exFAT_DataPool;



void             FS_exFAT_ModuleInit           (FS_QTY             vol_cnt,     /* Initialize FAT system driver.        */
                                                FS_QTY             file_cnt,
                                                FS_QTY             dir_cnt,
                                                FS_ERR            *p_err) {

    CPU_SIZE_T  octets_reqd;
    LIB_ERR     pool_err;


                                                                /* ----------------- CREATE INFO POOL ----------------- */
    Mem_PoolCreate(&FS_exFAT_DataPool,
                    DEF_NULL,
                    0,
                    vol_cnt,
                    sizeof(FS_EXFAT_DATA),
                    sizeof(CPU_ALIGN),
                   &octets_reqd,
                   &pool_err);

    if (pool_err != LIB_MEM_ERR_NONE) {
       *p_err = FS_ERR_MEM_ALLOC;
        FS_TRACE_INFO(("FS_FAT_ModuleInit(): Could not alloc mem for FAT info: %d octets req'd.\r\n", octets_reqd));
        return;
    }



                                                                /* --------------- INIT FAT FILE MODULE --------------- */
    FS_exFAT_FileModuleInit(file_cnt,
                          p_err);

    if (*p_err != FS_ERR_NONE) {
        return;
    }



#ifdef  FS_DIR_MODULE_PRESENT                                   /* ---------------- INIT FAT DIR MODULE --------------- */
    if (dir_cnt != 0u) {
        FS_FAT_DirModuleInit(dir_cnt,
                             p_err);

        if (*p_err != FS_ERR_NONE) {
            return;
        }
    }
#else
    (void)dir_cnt;
#endif



#ifdef  FS_FAT_JOURNAL_MODULE_PRESENT                           /* -------------- INIT FAT JOURNAL MODULE ------------- */
    FS_FAT_JournalModuleInit(vol_cnt, p_err);

    if (*p_err != FS_ERR_NONE) {
        return;
    }
#endif

   *p_err = FS_ERR_NONE;
}

void             FS_exFAT_VolClose             (FS_VOL            *p_vol) {}      /* Close a volume.                      */

#if (FS_CFG_RD_ONLY_EN == DEF_DISABLED)
void             FS_exFAT_VolFmt               (FS_VOL            *p_vol,       /* Create a volume.                     */
                                                void              *p_sys_cfg,
                                                FS_SEC_SIZE        sec_size,
                                                FS_SEC_QTY         size,
                                                FS_ERR            *p_err) {}
#endif

void             FS_exFAT_VolLabelGet          (FS_VOL            *p_vol,       /* Get volume label.                    */
                                                CPU_CHAR          *label,
                                                CPU_SIZE_T         len_max,
                                                FS_ERR            *p_err) {}

#if (FS_CFG_RD_ONLY_EN == DEF_DISABLED)
void             FS_exFAT_VolLabelSet          (FS_VOL            *p_vol,       /* Set volume label.                    */
                                                CPU_CHAR          *label,
                                                FS_ERR            *p_err) {}
#endif

void             FS_exFAT_VolOpen              (FS_VOL            *p_vol,       /* Open a volume.                       */
                                                FS_ERR            *p_err) {}

void             FS_exFAT_VolQuery             (FS_VOL            *p_vol,       /* Get info about a volume.             */
                                                FS_SYS_INFO       *p_info,
                                                FS_ERR            *p_err) {}






void             FS_FAT_LowEntryFind           (FS_VOL            *p_vol,       /* Find entry (low-level).              */
                                                FS_FAT_FILE_DATA  *p_entry_data,
                                                CPU_CHAR          *name_entry,
                                                FS_FLAGS           mode,
                                                FS_ERR            *p_err) {}