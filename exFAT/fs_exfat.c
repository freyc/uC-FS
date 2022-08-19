#include "fs_exfat.h"

#ifdef   FS_EXFAT_MODULE_PRESENT

#include "../Source/fs_buf.h"
#include "../Source/fs_vol.h"
#include "fs.h"
#include "fs_dev.h"
#include "fs_type.h"
#include "fs_sys.h"

#define FS_EXFAT_MAX_LABEL_LENGTH   22

#define DIR_ENTRY_TYPE_LABEL    0x83

#define  MEM_VAL_GET_INT64U_LITTLE(addr)        ((CPU_INT64U)(((CPU_INT64U)(((CPU_INT64U)(*(((CPU_INT08U *)(addr)) + 0))) << (0u * DEF_OCTET_NBR_BITS))) + \
                                                              ((CPU_INT64U)(((CPU_INT64U)(*(((CPU_INT08U *)(addr)) + 1))) << (1u * DEF_OCTET_NBR_BITS))) + \
                                                              ((CPU_INT64U)(((CPU_INT64U)(*(((CPU_INT08U *)(addr)) + 2))) << (2u * DEF_OCTET_NBR_BITS))) + \
                                                              ((CPU_INT64U)(((CPU_INT64U)(*(((CPU_INT08U *)(addr)) + 3))) << (3u * DEF_OCTET_NBR_BITS))) + \
                                                              ((CPU_INT64U)(((CPU_INT64U)(*(((CPU_INT08U *)(addr)) + 4))) << (4u * DEF_OCTET_NBR_BITS))) + \
                                                              ((CPU_INT64U)(((CPU_INT64U)(*(((CPU_INT08U *)(addr)) + 5))) << (5u * DEF_OCTET_NBR_BITS))) + \
                                                              ((CPU_INT64U)(((CPU_INT64U)(*(((CPU_INT08U *)(addr)) + 6))) << (6u * DEF_OCTET_NBR_BITS))) + \
                                                              ((CPU_INT64U)(((CPU_INT64U)(*(((CPU_INT08U *)(addr)) + 7))) << (7u * DEF_OCTET_NBR_BITS)))))



#define FS_EXFAT_DIR_ENTRY_SIZE (32u)

static  MEM_POOL  FS_exFAT_DataPool;

static void  FS_exFAT_ChkBootSec (FS_EXFAT_DATA  *p_fat_data, CPU_INT08U   *p_temp_08, FS_ERR       *p_err);
static void  FS_exFAT_Query(FS_VOL *p_vol, FS_BUF *p_buf, FS_SYS_INFO *p_info, FS_ERR *p_err);
static void FS_exFAT_SearchDirEntry(FS_EXFAT_DATA* p_fat_data, FS_BUF *p_buf, CPU_INT08U type, FS_SEC_NBR *sec, FS_SEC_SIZE *sec_offset, FS_ERR *p_err);

static void FS_exFAT_SearchDirEntry(FS_EXFAT_DATA* p_fat_data, FS_BUF *p_buf, CPU_INT08U type, FS_SEC_NBR *sec, FS_SEC_SIZE *sec_offset, FS_ERR *p_err) 
{
    FS_SEC_NBR start_sec = p_fat_data->ClusterHeapOffset_sec + ((p_fat_data->FirstClusterOfRootDir - 2) << p_fat_data->SectorsPerClusterShift);
    FS_SEC_SIZE offset;
    CPU_INT08U dirEntryType;

    FSBuf_Set(p_buf, start_sec, FS_VOL_SEC_TYPE_MGMT, DEF_YES, p_err);

    offset = 0;

    dirEntryType = MEM_VAL_GET_INT08U_LITTLE(p_buf->DataPtr + offset);
    while(dirEntryType != type && dirEntryType != 0x00) {
        offset = offset + 32u;
        dirEntryType = MEM_VAL_GET_INT08U_LITTLE(p_buf->DataPtr + offset);
    }

    if(dirEntryType == type) {
        *sec = start_sec;
        *sec_offset = offset;
        *p_err = FS_ERR_NONE;
        return;
    }

    *p_err = FS_ERR_ENTRY_NOT_FOUND;
}

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
                                                FS_ERR            *p_err) 
{
    FS_BUF       *p_buf;
    FS_EXFAT_DATA  *p_fat_data;


    p_fat_data = (FS_EXFAT_DATA *)p_vol->DataPtr;

    if (p_fat_data == (FS_EXFAT_DATA *)0) {
       *p_err =   FS_ERR_VOL_INVALID_SYS;
        return;
    }

    p_buf = FSBuf_Get(p_vol);
    if (p_buf == (FS_BUF *)0) {
       *p_err =   FS_ERR_BUF_NONE_AVAIL;
        return;
    }


    FS_SEC_NBR sec;
    FS_SEC_SIZE sec_offset;
    FS_exFAT_SearchDirEntry(p_fat_data, p_buf, DIR_ENTRY_TYPE_LABEL, &sec, &sec_offset, p_err);

    if(*p_err != FS_ERR_NONE) {
        FSBuf_Free(p_buf);
        return;
    }

    FSBuf_Set(p_buf, sec, FS_VOL_SEC_TYPE_MGMT, DEF_YES, p_err);
    CPU_INT08U label_length = MEM_VAL_GET_INT08U_LITTLE(p_buf->DataPtr + sec_offset + 1);
    if(label_length > len_max || label_length > FS_EXFAT_MAX_LABEL_LENGTH) {
        *p_err = FS_ERR_INVALID_ARG;
        FSBuf_Free(p_buf);
        return;
    }

    Mem_Copy(label, p_buf->DataPtr + sec_offset + 2, DEF_MAX(FS_EXFAT_MAX_LABEL_LENGTH, len_max)  );

    FSBuf_Free(p_buf);

    *p_err = FS_ERR_NONE;
}

#if (FS_CFG_RD_ONLY_EN == DEF_DISABLED)
void             FS_exFAT_VolLabelSet          (FS_VOL            *p_vol,       /* Set volume label.                    */
                                                CPU_CHAR          *label,
                                                FS_ERR            *p_err) {}
#endif

void  FS_exFAT_VolOpen (FS_VOL  *p_vol,
                        FS_ERR  *p_err)
{
    CPU_INT16U      boot_sig;
    FS_BUF         *p_buf;
    FS_EXFAT_DATA  *p_fat_data;
    void           *p_temp;
    CPU_INT08U     *p_temp_08;
    FS_ERR          err_tmp;
    LIB_ERR         pool_err;


    p_buf = FSBuf_Get(p_vol);
    if (p_buf == (FS_BUF *)0) {
       *p_err = FS_ERR_BUF_NONE_AVAIL;
        return;
    }

    p_temp    = (void       *)p_buf->DataPtr;
    p_temp_08 = (CPU_INT08U *)p_temp;



                                                                /* ------------------- RD FIRST SEC ------------------- */
    FSVol_RdLockedEx(p_vol,
                     p_temp,
                     0u,
                     1u,
                     FS_VOL_SEC_TYPE_MGMT,
                     p_err);

    if (*p_err != FS_ERR_NONE) {
         FSBuf_Free(p_buf);
         return;
    }

                                                                /* Check signature in bytes 510:511 of sec.             */
    boot_sig = MEM_VAL_GET_INT16U_LITTLE((void *)(p_temp_08 + 510u));
    if (boot_sig != 0xAA55) {
        FSBuf_Free(p_buf);
        FS_TRACE_DBG(("FS_FAT_VolOpen(): Invalid boot sec sig: 0x%04X != 0x%04X\r\n", boot_sig, 0xAA55));
       *p_err = FS_ERR_VOL_INVALID_SYS;
        return;
    }
                                                                /* ------------------ ALLOC FAT DATA ------------------ */
    FS_OS_Lock(p_err);                                          /* Acquire FS lock (see Note #1).                       */
    if (*p_err != FS_ERR_NONE) {
        FSBuf_Free(p_buf);
        return;
    }

    p_fat_data = (FS_EXFAT_DATA *)Mem_PoolBlkGet(&FS_exFAT_DataPool,/* Alloc FAT data.                                      */
                                                 sizeof(FS_EXFAT_DATA),
                                                 &pool_err);
    (void)pool_err;                                            /* Err ignored. Ret val chk'd instead.                  */

    FS_OS_Unlock();


    if (p_fat_data == (FS_EXFAT_DATA *)0) {
        FSBuf_Free(p_buf);
        return;
    }

    Mem_Set(p_fat_data, 0x00, sizeof(FS_EXFAT_DATA));

                                                                /* --------- DETERMINE & APPLY FILE SYS PROPS --------- */
    FS_exFAT_ChkBootSec(p_fat_data,
                      p_temp_08,
                      p_err);

    FSBuf_Free(p_buf);

    if (*p_err != FS_ERR_NONE) {                                /* If fat param's NOT valid ...                         */
        FS_OS_Lock(&err_tmp);
        Mem_PoolBlkFree(        &FS_exFAT_DataPool,               /* ... free FAT data        ...                         */
                        (void *) p_fat_data,
                                &pool_err);
        FS_OS_Unlock();
        return;                                                 /* ... & rtn.                                           */
    }

                                                                /* ------------------ ALLOC FAT DATA ------------------ */
    p_vol->DataPtr = (void *)p_fat_data;                        /* Save FAT data in vol.                                */

#ifdef  FS_FAT_JOURNAL_MODULE_PRESENT
    FS_FAT_JournalInit(p_vol, p_err);                           /* Init journal info.                                   */

    if (*p_err != FS_ERR_NONE) {
        FS_OS_Lock(&err_tmp);
        Mem_PoolBlkFree(        &FS_FAT_DataPool,               /* ... & free FAT data.                                 */
                        (void *) p_fat_data,
                                &pool_err);
        FS_OS_Unlock();
        p_vol->DataPtr = (void *)0;
        return;
    }
#endif

                                                                /* ----------------- OUTPUT TRACE INFO ---------------- */
    FS_TRACE_INFO(("FS_exFAT_VolOpen(): File system found: Type   : exFAT \r\n"));
    FS_TRACE_INFO(("                                     Sec  size: %d B  \r\n",  p_fat_data->SectorSize));
    FS_TRACE_INFO(("                                     Clus size: %d sec\r\n",  p_fat_data->SectorsPerCluster));
    FS_TRACE_INFO(("                                     Vol  size: %d sec\r\n",  p_fat_data->VolumeLength));
    FS_TRACE_INFO(("                                     # Clus   : %d    \r\n", (p_fat_data->ClusterCount)));
    FS_TRACE_INFO(("                                     # FATs   : %d    \r\n",  p_fat_data->NbrOfFATs));

#if 1
    printf("FAT pos : 0x%08X\n", p_fat_data->FatOffset_sec << p_fat_data->BytesPerSectorShift);
    printf("Data pos: 0x%08X\n", p_fat_data->ClusterHeapOffset_sec << p_fat_data->BytesPerSectorShift);
    printf("Cluster size: 0x%08X\n", p_fat_data->SectorsPerCluster << p_fat_data->BytesPerSectorShift);
    printf("Rootdir pos: 0x%08X\n", 
        (((p_fat_data->FirstClusterOfRootDir - 2) << p_fat_data->SectorsPerClusterShift)  + p_fat_data->ClusterHeapOffset_sec) << p_fat_data->BytesPerSectorShift);
#endif

   *p_err = FS_ERR_NONE;
}

void             FS_exFAT_VolQuery             (FS_VOL            *p_vol,       /* Get info about a volume.             */
                                                FS_SYS_INFO       *p_info,
                                                FS_ERR            *p_err) 
{
    FS_BUF       *p_buf;
    FS_EXFAT_DATA  *p_fat_data;


    p_fat_data = (FS_EXFAT_DATA *)p_vol->DataPtr;

    if (p_fat_data == (FS_EXFAT_DATA *)0) {
       *p_err =   FS_ERR_VOL_INVALID_SYS;
        return;
    }

    p_buf = FSBuf_Get(p_vol);
    if (p_buf == (FS_BUF *)0) {
       *p_err =   FS_ERR_BUF_NONE_AVAIL;
        return;
    }


    FS_exFAT_Query(p_vol,
                 p_buf,
                 p_info,
                 p_err);

    FSBuf_Free(p_buf);
}






void             FS_FAT_LowEntryFind           (FS_VOL            *p_vol,       /* Find entry (low-level).              */
                                                FS_FAT_FILE_DATA  *p_entry_data,
                                                CPU_CHAR          *name_entry,
                                                FS_FLAGS           mode,
                                                FS_ERR            *p_err) {}




















static FS_FAT_CLUS_NBR exFAT_ClusValRd(FS_VOL           *p_vol,
                                                 FS_BUF           *p_buf,
                                                 FS_FAT_CLUS_NBR   clus,
                                                 FS_ERR           *p_err) 
{
FS_SEC_SIZE       fat_offset;
    FS_FAT_SEC_NBR    fat_sec;
    FS_SEC_SIZE       fat_sec_offset;
    FS_FAT_SEC_NBR    fat_start_sec;
    FS_EXFAT_DATA    *p_fat_data;
    FS_FAT_CLUS_NBR   val;
    FS_FAT_CLUS_NBR   val_lo;
    FS_FAT_CLUS_NBR   val_hi;
    FS_FAT_CLUS_NBR   val_temp;


    p_fat_data     = (FS_EXFAT_DATA *)p_vol->DataPtr;

    fat_start_sec  =  p_fat_data->FatOffset_sec;
    fat_offset = clus * 4u;
    fat_sec = fat_start_sec + (fat_offset >> p_fat_data->BytesPerSectorShift);
    fat_sec_offset = fat_offset & (p_fat_data->SectorSize - 1);

    FSBuf_Set(p_buf,                                        /* Rd 1st FAT sec.                                      */
              fat_sec,
              FS_VOL_SEC_TYPE_MGMT,
              DEF_YES,
              p_err);
    if (*p_err != FS_ERR_NONE) {
        return (0u);
    }

    val  = MEM_VAL_GET_INT32U_LITTLE((void *)((CPU_INT08U *)p_buf->DataPtr + fat_sec_offset));

    return (val);
}





void  FS_exFAT_Query (FS_VOL       *p_vol,
                      FS_BUF       *p_buf,
                      FS_SYS_INFO  *p_info,
                      FS_ERR       *p_err)
{
    FS_FAT_CLUS_NBR   bad_clus_cnt;
    FS_FAT_CLUS_NBR   clus;
    FS_FAT_CLUS_NBR   fat_entry;
    FS_FAT_CLUS_NBR   free_clus_cnt;
    FS_FAT_CLUS_NBR   used_clus_cnt;
    FS_EXFAT_DATA    *p_fat_data;
    FS_SEC_NBR sec;
    FS_SEC_SIZE sec_offset;


                                                                /* ----------------- ASSIGN DFLT VAL'S ---------------- */
    p_fat_data         = (FS_EXFAT_DATA *)p_vol->DataPtr;
    p_info->BadSecCnt  =  0u;
    p_info->FreeSecCnt =  0u;
    p_info->UsedSecCnt =  0u;
    p_info->TotSecCnt  =  0u;

    FS_exFAT_SearchDirEntry(p_fat_data, p_buf, 0x81, &sec, &sec_offset, p_err);

    if(*p_err != FS_ERR_NONE) {
        return;
    }

    FSBuf_Set(p_buf, sec, FS_VOL_SEC_TYPE_MGMT, DEF_YES, p_err);

    FS_FAT_CLUS_NBR first_cluster = MEM_VAL_GET_INT32U_LITTLE(p_buf->DataPtr + sec_offset + 20u);
    CPU_INT64U bitmap_size = MEM_VAL_GET_INT64U_LITTLE(p_buf->DataPtr + sec_offset + 24u);

    FSBuf_Set(p_buf, p_fat_data->ClusterHeapOffset_sec + ((first_cluster - 2) << p_fat_data->SectorsPerClusterShift), FS_VOL_SEC_TYPE_MGMT, DEF_YES, p_err);

    CPU_INT32U offset = 0;
    
    used_clus_cnt = 0;
    while(offset < p_buf->Size && offset < bitmap_size) {
        CPU_INT32U bits = MEM_VAL_GET_INT32U_LITTLE(p_buf->DataPtr + offset);
        offset = offset + 4;
        used_clus_cnt = used_clus_cnt + CPU_PopCnt32(bits);
    }

#if 0
                                                                /* ----------------- CHK IF INFO CACHED --------------- */
    if (p_fat_data->QueryInfoValid == DEF_YES) {
        p_info->BadSecCnt  = (FS_SEC_QTY)FS_UTIL_MULT_PWR2(p_fat_data->QueryBadClusCnt,  p_fat_data->ClusSizeLog2_sec);
        p_info->FreeSecCnt = (FS_SEC_QTY)FS_UTIL_MULT_PWR2(p_fat_data->QueryFreeClusCnt, p_fat_data->ClusSizeLog2_sec);

        if (p_fat_data->MaxClusNbr >= (FS_FAT_MIN_CLUS_NBR + p_fat_data->QueryBadClusCnt + p_fat_data->QueryFreeClusCnt)) {
            used_clus_cnt = ((p_fat_data->MaxClusNbr - FS_FAT_MIN_CLUS_NBR) - p_fat_data->QueryBadClusCnt) - p_fat_data->QueryFreeClusCnt;
        } else {
            used_clus_cnt = 0u;
        }

        p_info->UsedSecCnt = (FS_SEC_QTY)FS_UTIL_MULT_PWR2(used_clus_cnt, p_fat_data->ClusSizeLog2_sec);
        p_info->TotSecCnt  =  p_info->BadSecCnt + p_info->FreeSecCnt + p_info->UsedSecCnt;

       *p_err = FS_ERR_NONE;
        return;
    }
#endif

#if 1
                                                                /* ---------- CNT NBR OF BAD/FREE/USED CLUS'S --------- */
    free_clus_cnt = 0u;
    used_clus_cnt = 0u;
    bad_clus_cnt  = 0u;
    clus          = 2u;
    while (clus < p_fat_data->ClusterCount) {
#if 1
        fat_entry = exFAT_ClusValRd(p_vol,
                                    p_buf,
                                    clus,
                                    p_err);
        if (*p_err != FS_ERR_NONE) {
            return;
        }

        if (fat_entry == 0xFFFFFFF7) {
            bad_clus_cnt++;

        } else if ((fat_entry >= 2 && fat_entry <= (p_fat_data->ClusterCount + 1)) || fat_entry == 0xFFFFFFFFu) {
            used_clus_cnt++;
        } else {
            free_clus_cnt++;
        }
#endif
        clus++;
    }



                                                                /* ------------------ CALC SEC CNT'S ------------------ */
    p_fat_data->QueryInfoValid   =  DEF_YES;
    p_fat_data->QueryBadClusCnt  =  bad_clus_cnt;
    p_fat_data->QueryFreeClusCnt =  free_clus_cnt;

    p_info->BadSecCnt            = bad_clus_cnt << p_fat_data->SectorsPerClusterShift; // (FS_SEC_QTY)FS_UTIL_MULT_PWR2(bad_clus_cnt,  p_fat_data->ClusSizeLog2_sec);
    p_info->FreeSecCnt           = free_clus_cnt << p_fat_data->SectorsPerClusterShift; // (FS_SEC_QTY)FS_UTIL_MULT_PWR2(free_clus_cnt, p_fat_data->ClusSizeLog2_sec);
    p_info->UsedSecCnt           = used_clus_cnt << p_fat_data->SectorsPerClusterShift; // (FS_SEC_QTY)FS_UTIL_MULT_PWR2(used_clus_cnt, p_fat_data->ClusSizeLog2_sec);
    p_info->TotSecCnt            =  p_info->BadSecCnt + p_info->FreeSecCnt + p_info->UsedSecCnt;

#endif
   *p_err = FS_ERR_NONE;
}












/*
*********************************************************************************************************
*                                         FS_FAT_ChkBootSec()
*
* Description : Check boot sector & calculate FAT parameters.
*
* Argument(s) : p_fat_data  Pointer to FAT data.
*               ----------  Argument validated by caller.
*
*               p_temp_08   Pointer to buffer containing boot sector.
*               ----------  Argument validated by caller.
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*               ----------  Argument validated by caller.
*
*                               FS_ERR_NONE           File system information parsed.
*                               FS_ERR_INVALID_SYS    Invalid file system.
*
* Return(s)   : none.
*
* Note(s)     : (1) (a) The first 36 bytes of the FAT boot sector are identical for FAT12/16/32 file
*                       systems :
*
*                       ---------------------------------------------------------------------------------------------------------
*                       |    JUMP INSTRUCTION TO BOOT CODE*    |             OEM NAME (in ASCII): TYPICALLY "MSWINx.y"
*                       ---------------------------------------------------------------------------------------------------------
*                                                              |    BYTES PER SECTOR**   | SEC/CLUS***|    RSVD SEC CNT****     |
*                       ---------------------------------------------------------------------------------------------------------
*                       | NBR FATs^  |    ROOT ENTRY CNT^^     | TOTAL NBR of SECTORS^^^ | MEDIA^^^^  |    FAT SIZE (in SEC)#   |
*                       ---------------------------------------------------------------------------------------------------------
*                       |    SECTORS PER TRACK    |       NBR of HEADS      |       NBR SECTORS before START OF PARTITION       |
*                       ---------------------------------------------------------------------------------------------------------
*                       |             TOTAL NBR of SECTORS^^^               |
*                       -----------------------------------------------------
*
*                       *Legal forms are
*                           boot[0] = 0xEB, boot[1] = ????, boot[2] = 0x90
*                        &
*                           boot[0] = 0xE9, boot[1] = ????, boot[2] = ????.
*
*                      **Legal values are 512, 1024, 2048 & 4096.
*
*                     ***Legal values are 2, 4, 8, 16, 32, 64 & 128.  However, the number of bytes per
*                        cluster MUST be less than or equal to 32 * 1024.
*
*                    ****For FAT12 & FAT16, this SHOULD be 1.  In any case, this MUST NOT be 0.
*
*                       ^This SHOULD be 2.
*
*                      ^^For FAT12/16, this field contains the number of directory entries in the root
*                        directory, typically 512.  For FAT32, this field should be 0.
*
*                     ^^^Either the 16-bit count (bytes 19-20) or the 32-bit count (bytes 32-35) should
*                        be non-zero & contain the total number of sectors in the file system.  The
*                        other should be zero.
*
*                    ^^^^Media type.  Legal values are 0xF0, 0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE
*                        & 0xFF.  Typically, 0xF0 should be used for fixed disks & 0xF8 should be used
*                        for removable disks.
*
*                       #For FAT12/16, the number of sectors in each FAT.  For FAT32, this SHOULD be 0.
*
*                   (b) The bytes 36-61 of the FAT boot sector for FAT12/16 file systems contain the
*                       following :
*
*                                                                           -----------------------------------------------------
*                                                                           | DRIVE NBR  |    0x00    |    0x29    |
*                       ---------------------------------------------------------------------------------------------------------
*                          VOLUME SERIAL NBR                   |            VOLUME LABEL (in ASCII)
*                       ---------------------------------------------------------------------------------------------------------
*                                                                                                     |
*                       ---------------------------------------------------------------------------------------------------------
                                 FILE SYSTEM TYPE LABEL (in ASCII)                                    |
*                       -------------------------------------------------------------------------------
*
*                   (c) The bytes 36-89 of the FAT boot sector for FAT32 file systems contain the
*                       following :
*
*                                                                           -----------------------------------------------------
*                                                                           |               FAT SIZE (in SECTORS)               |
*                       ---------------------------------------------------------------------------------------------------------
*                       |         FLAGS*          |     VERSION NUMBER**    |     CLUSTER NBR of FIRST CLUSTER OF ROOT DIR      |
*                       ---------------------------------------------------------------------------------------------------------
*                       |   FSINFO SECTOR NBR***  |  BACKUP BOOT RECORD**** |                       0x0000
*                       ---------------------------------------------------------------------------------------------------------
*                                              0x0000                                               0x0000                      |
*                       ---------------------------------------------------------------------------------------------------------
*                       | DRIVE NBR  |    0x00    |    0x29    |                 VOLUME SERIAL NBR                 |
*                       ---------------------------------------------------------------------------------------------------------
*                                                             VOLUME LABEL (in ASCII)
*                       ---------------------------------------------------------------------------------------------------------
*                                                 |             FILE SYSTEM TYPE LABEL (in ASCII): TYPICALLY "FAT32    "
*                       ---------------------------------------------------------------------------------------------------------
*                                                 |
*                       ---------------------------
*
*                       *If bit 7 of this is 1, then only one FAT is active; the number of this FAT is
*                        specified in bits 0-3.  Otherwise, the FAT is mirrored at runtime into all FATs.
*
*                      **File systems conforming to Microsoft's FAT documentation should contain 0x0000.
*
*                     ***Typically 1.
*
*                    ****Typically 6.
*
*                   (d) For FAT12/16/32, bytes 510 & 511 of the FAT boot sector are ALWAYS 0xAA55.
*
*               (2) (a) The total number of sectors in the file system MAY be smaller (perhaps
*                       considerably smaller) than the number of sectors in the disk.  However, the
*                       total number of sectors in the file system SHOULD NEVER be larger than the
*                       number of sectors in the disk.
*
*                   (b) For FAT32, the number of sectors in the root directory is ALWAYS 0, since the
*                       root directory lies inside the data area.
*
*                   (c) (1) For FAT12/16, the data area begins after the predetermined root directory
*                           (which immediately follows the FAT sectors).
*
*                       (2) For FAT32,    the data area begins immediately after the FAT sectors (&
*                           includes the root directory).
*
*                       (3) (a) The total number of clusters in the volume is equal to the total number
*                               of data sectors divided by the number of clusters per sector :
*
*                                                             Number of Data Sectors
*                                   Number of Clusters = -------------------------------
*                                                         Number of Clusters per Sector
*
*                           (b) Since clusters 0 & 1 do not exist, the highest cluster number is
*                               'Number of Clusters' + 1.
*
*                   (d) (1) Four areas of a FAT12/16 file system exist :
*
*                           (a) The reserved area.            ----------------------------------------------------------------
*                           (b) The FAT      area.            | Rsvd |   FAT 1   |   FAT 2   | Root |         Data           |
*                           (c) The root     directory.       | Area |   Area    |   Area    | Dir  |         Area           |
*                           (d) The data     area.            ----------------------------------------------------------------
*                                                                    ^           ^           ^      ^
*                                                                    |           |           |      |
*                                                                    |           |           |      |
*                                                 'FS_FAT_DATA.FAT1_Start'       |           |      |
*                                                                                |           |      |
*                                                             'FS_FAT_DATA.FAT2_Start'       |   'FS_FAT_DATA.DataStart'
*                                                                                            |          @ Cluster #2
*                                                                     'FS_FAT_DATA.RootDirStart'
*
*                       (2) Three areas of a FAT32 file system exist :
*
*                           (a) The reserved area.            ----------------------------------------------------------------
*                           (b) The FAT      area.            | Rsvd |   FAT 1   |   FAT 2   |            Data               |
*                           (c) The data     area.            | Area |   Area    |   Area    |            Area               |
*                                                             ----------------------------------------------------------------
*                                                                    ^           ^           ^
*                                                                    |           |           |
*                                                                    |           |           |
*                                                 'FS_FAT_DATA.FAT1_Start'       |        'FS_FAT_DATA.DataStart'
*                                                                                |               @ Cluster #2
*                                                             'FS_FAT_DATA.FAT2_Start'
*
*                           Unlike FAT12/16, the root directory is in cluster(s) within the data area.
*
*                       (3) Up to three sectors of the reserved area may be used :
*
*                           (1) THE BOOT SECTOR.  This sector, the sector 0 of the volume, contains
*                               information about the format, size & layout of the file system.
*
*                           (2) THE BACKUP BOOT SECTOR.  This sector, typically sector 1 of the volume,
*                               contains a backup copy of the boot sector.  The backup boot sector is
*                               NOT used on FAT12/16 volumes.
*
*                           (3) THE FSINFO SECTOR.  This sector, typically sector 6 of the volume, may
*                               be used to help the file system suite more quickly access the volume.
*                               The FSINFO sector is NOT used on FAT12/16 volumes.
*
*               (3) The 'ClusSize_octet' value is stored temporarily in a 32-bit variable to protect
*                   against 16-bit overflow.  However, according to Microsoft's FAT specification, all
*                   legitimate values fit within the range of 16-bit unsigned integers.
********************************************************************************************************
*/

static  void  FS_exFAT_ChkBootSec (FS_EXFAT_DATA  *p_fat_data,
                                   CPU_INT08U     *p_temp_08,
                                   FS_ERR         *p_err)
{
    FS_SEC_SIZE      clus_size_octet;
    FS_FAT_CLUS_NBR  clus_cnt;
    FS_FAT_CLUS_NBR  clus_cnt_max_fat;

    FS_FAT_SEC_NBR   fat_size;
    CPU_INT16U       root_cnt;
    CPU_INT32U       vol_size;
#if (FS_FAT_CFG_FAT32_EN == DEF_ENABLED)
    FS_FAT_CLUS_NBR  clus_nbr;
    CPU_INT16U       sec_nbr;
    CPU_BOOLEAN      valid;
#endif

    p_fat_data->BytesPerSectorShift = MEM_VAL_GET_INT08U_LITTLE(p_temp_08 + 108);
    p_fat_data->SectorsPerClusterShift = MEM_VAL_GET_INT08U_LITTLE(p_temp_08 + 109);

    p_fat_data->VolumeLength = MEM_VAL_GET_INT64U_LITTLE(p_temp_08 + 72);
    p_fat_data->PartitionOffset = MEM_VAL_GET_INT64U_LITTLE(p_temp_08 + 64);
    p_fat_data->ClusterCount = MEM_VAL_GET_INT32U_LITTLE(p_temp_08 + 92);
    p_fat_data->FirstClusterOfRootDir = MEM_VAL_GET_INT32U_LITTLE(p_temp_08 + 96);

    if(p_fat_data->BytesPerSectorShift < 9u || p_fat_data->BytesPerSectorShift > 12u) {
        FS_TRACE_DBG(("FS_exFAT_ChkBootSec(): Invalid bytes/sec\r\n"));
        *p_err = FS_ERR_VOL_INVALID_SYS;
        return;
    }

    if(p_fat_data->SectorsPerClusterShift > 25 - p_fat_data->BytesPerSectorShift) {
        FS_TRACE_DBG(("FS_exFAT_ChkBootSec(): Invalid sec/clus\r\n"));
        *p_err = FS_ERR_VOL_INVALID_SYS;
        return;
    }

    p_fat_data->SectorSize = (1u << p_fat_data->BytesPerSectorShift);
    p_fat_data->SectorsPerCluster = (1u << p_fat_data->SectorsPerClusterShift);

    p_fat_data->FatOffset_sec = MEM_VAL_GET_INT32U_LITTLE(p_temp_08 + 80);
    p_fat_data->FatLength_sec = MEM_VAL_GET_INT32U_LITTLE(p_temp_08 + 84);
    p_fat_data->ClusterHeapOffset_sec = MEM_VAL_GET_INT32U_LITTLE(p_temp_08 + 88);
    p_fat_data->ClusterCount = MEM_VAL_GET_INT32U_LITTLE(p_temp_08 + 92);
    p_fat_data->NbrOfFATs = MEM_VAL_GET_INT08U_LITTLE(p_temp_08 + 110u);

#if 0
                                                                /* ---------------- RD BOOT SET PARAMS ---------------- */
    p_fat_data->SecSize = (FS_SEC_SIZE)MEM_VAL_GET_INT16U_LITTLE((void *)(p_temp_08 + FS_FAT_BPB_OFF_BYTSPERSEC));
    switch (p_fat_data->SecSize) {
        case 512u:
        case 1024u:
        case 2048u:
        case 4096u:
             p_fat_data->SecSizeLog2 = FSUtil_Log2(p_fat_data->SecSize);
             break;

        default:
             FS_TRACE_DBG(("FS_FAT_ChkBootSec(): Invalid bytes/sec: %d\r\n", p_fat_data->SecSize));
            *p_err = FS_ERR_VOL_INVALID_SYS;
             return;
    }

    p_fat_data->ClusSize_sec = (FS_FAT_SEC_NBR)MEM_VAL_GET_INT08U_LITTLE((void *)(p_temp_08 + FS_FAT_BPB_OFF_SECPERCLUS));
    switch (p_fat_data->ClusSize_sec) {
        case 1u:
        case 2u:
        case 4u:
        case 8u:
        case 16u:
        case 32u:
        case 64u:
        case 128u:
             p_fat_data->ClusSizeLog2_sec = FSUtil_Log2((CPU_INT32U)p_fat_data->ClusSize_sec);
             break;

        default:
             FS_TRACE_DBG(("FS_FAT_ChkBootSec(): Invalid secs/clus: %d\n", p_fat_data->ClusSize_sec));
            *p_err = FS_ERR_VOL_INVALID_SYS;
             return;
    }

                                                                /* See Note #3.                                         */
    clus_size_octet = p_fat_data->SecSize << p_fat_data->ClusSizeLog2_sec;
    switch (clus_size_octet) {
        case 512u:
        case 1024u:
        case 2048u:
        case 4096u:
        case 8192u:
        case 16384u:
        case 32768u:
        case 65536u:
             p_fat_data->ClusSize_octet     = clus_size_octet;
             p_fat_data->ClusSizeLog2_octet = FSUtil_Log2(clus_size_octet);
             break;

        default:
             FS_TRACE_DBG(("FS_FAT_ChkBootSec(): Invalid bytes/clus: %d\n", clus_size_octet));
            *p_err = FS_ERR_VOL_INVALID_SYS;
             return;
    }

    p_fat_data->NbrFATs = MEM_VAL_GET_INT08U_LITTLE((void *)(p_temp_08 + FS_FAT_BPB_OFF_NUMFATS));
    switch (p_fat_data->NbrFATs) {
        case 1:
        case 2:
             break;

        default:
             FS_TRACE_DBG(("FS_FAT_ChkBootSec(): Invalid nbr FATs: %d\n", p_fat_data->NbrFATs));
            *p_err = FS_ERR_VOL_INVALID_SYS;
             return;
    }
                                                                /* Size of reserved area.                               */
    p_fat_data->RsvdSize    = (FS_FAT_SEC_NBR)MEM_VAL_GET_INT16U_LITTLE((void *)(p_temp_08 + FS_FAT_BPB_OFF_RSVDSECCNT));
    p_fat_data->FAT1_Start  =  p_fat_data->RsvdSize;            /* Sec nbr of 1st FAT.                                  */

                                                                /* Size of each FAT.                                    */
    fat_size                = (FS_FAT_SEC_NBR)MEM_VAL_GET_INT16U_LITTLE((void *)(p_temp_08 + FS_FAT_BPB_OFF_FATSZ16));
    if (fat_size           ==  0u) {
        fat_size            = (FS_FAT_SEC_NBR)MEM_VAL_GET_INT32U_LITTLE((void *)(p_temp_08 + FS_FAT_BPB_FAT32_OFF_FATSZ32));
    }
    p_fat_data->FAT_Size    =  fat_size;

                                                                /* Size of root dir (see Notes #2b).                    */
    root_cnt                =  MEM_VAL_GET_INT16U_LITTLE((void *)(p_temp_08 + FS_FAT_BPB_OFF_ROOTENTCNT));
    p_fat_data->RootDirSize =  FS_UTIL_DIV_PWR2((((FS_FAT_SEC_NBR)root_cnt * 32u) + ((FS_FAT_SEC_NBR)p_fat_data->SecSize - 1u)), p_fat_data->SecSizeLog2);

                                                                /* Sec nbr's of 2nd FAT, root dir, data.                */
    if (p_fat_data->NbrFATs      == 2u) {
        p_fat_data->FAT2_Start   =  p_fat_data->RsvdSize +  p_fat_data->FAT_Size;
        p_fat_data->RootDirStart =  p_fat_data->RsvdSize + (p_fat_data->FAT_Size * 2u);
        p_fat_data->DataStart    =  p_fat_data->RsvdSize + (p_fat_data->FAT_Size * 2u) + p_fat_data->RootDirSize;
    } else {
        p_fat_data->FAT2_Start   =  0u;
        p_fat_data->RootDirStart =  p_fat_data->RsvdSize + (p_fat_data->FAT_Size * 1u);
        p_fat_data->DataStart    =  p_fat_data->RsvdSize + (p_fat_data->FAT_Size * 1u) + p_fat_data->RootDirSize;
    }

                                                                /* Size of vol (see Notes #2a).                         */
    vol_size                = (CPU_INT32U)MEM_VAL_GET_INT16U_LITTLE((void *)(p_temp_08 + FS_FAT_BPB_OFF_TOTSEC16));
    if (vol_size           == 0u) {
        vol_size            = MEM_VAL_GET_INT32U_LITTLE((void *)(p_temp_08 + FS_FAT_BPB_OFF_TOTSEC32));
    }
    p_fat_data->VolSize     = vol_size;

                                                                /* Size of data area (see Notes #2c).                   */
    p_fat_data->DataSize    = (p_fat_data->VolSize - (p_fat_data->RsvdSize + ((FS_FAT_SEC_NBR)p_fat_data->NbrFATs * p_fat_data->FAT_Size))) - p_fat_data->RootDirSize;
    p_fat_data->NextClusNbr =  2u;                              /* Next clus to alloc.                                  */
                                                                /* See Notes #2c3.                                      */
    clus_cnt                =  (FS_FAT_CLUS_NBR)FS_UTIL_DIV_PWR2(p_fat_data->DataSize, p_fat_data->ClusSizeLog2_sec);



                                                                /* --------------------- CHK FAT12 -------------------- */
    if (clus_cnt                   <=  FS_FAT_MAX_NBR_CLUS_FAT12) {
#if (FS_FAT_CFG_FAT12_EN == DEF_ENABLED)
        p_fat_data->FAT_Type        =  FS_FAT_FAT_TYPE_FAT12;
        p_fat_data->FAT_TypeAPI_Ptr = &FS_FAT_FAT12_API;
        clus_cnt_max_fat            = ((FS_FAT_CLUS_NBR)p_fat_data->FAT_Size * (FS_FAT_CLUS_NBR)p_fat_data->SecSize * 2u) / 3u;
        p_fat_data->MaxClusNbr      =  DEF_MIN(clus_cnt + FS_FAT_MIN_CLUS_NBR, clus_cnt_max_fat + FS_FAT_MIN_CLUS_NBR);
#else
        FS_TRACE_DBG(("FS_FAT_ChkBootSec(): Invalid file sys: FAT12\n"));
       *p_err = FS_ERR_VOL_INVALID_SYS;
        return;
#endif


                                                                /* --------------------- CHK FAT16 -------------------- */
    } else if (clus_cnt            <=  FS_FAT_MAX_NBR_CLUS_FAT16) {
#if (FS_FAT_CFG_FAT16_EN == DEF_ENABLED)
        p_fat_data->FAT_Type        =  FS_FAT_FAT_TYPE_FAT16;
        p_fat_data->FAT_TypeAPI_Ptr = &FS_FAT_FAT16_API;
        clus_cnt_max_fat            = (FS_FAT_CLUS_NBR)p_fat_data->FAT_Size * ((FS_FAT_CLUS_NBR)p_fat_data->SecSize / FS_FAT_FAT16_ENTRY_NBR_OCTETS);
        p_fat_data->MaxClusNbr      =  DEF_MIN(clus_cnt + FS_FAT_MIN_CLUS_NBR, clus_cnt_max_fat + FS_FAT_MIN_CLUS_NBR);
#else
        FS_TRACE_DBG(("FS_FAT_ChkBootSec(): Invalid file sys: FAT16\n"));
       *p_err = FS_ERR_VOL_INVALID_SYS;
        return;
#endif


                                                                /* --------------------- CHK FAT32 -------------------- */
    } else {
#if (FS_FAT_CFG_FAT32_EN == DEF_ENABLED)
        p_fat_data->FAT_Type        =  FS_FAT_FAT_TYPE_FAT32;
        p_fat_data->FAT_TypeAPI_Ptr = &FS_FAT_FAT32_API;
        clus_cnt_max_fat            = (FS_FAT_CLUS_NBR)p_fat_data->FAT_Size * ((FS_FAT_CLUS_NBR)p_fat_data->SecSize / FS_FAT_FAT32_ENTRY_NBR_OCTETS);
        p_fat_data->MaxClusNbr      = DEF_MIN(clus_cnt + FS_FAT_MIN_CLUS_NBR, clus_cnt_max_fat + FS_FAT_MIN_CLUS_NBR);

                                                                /* Get root dir clus nbr.                               */
        clus_nbr                    = MEM_VAL_GET_INT32U_LITTLE((void *)(p_temp_08 + FS_FAT_BPB_FAT32_OFF_ROOTCLUS));
        valid                       = FS_FAT_IS_VALID_CLUS(p_fat_data, clus_nbr);
        if (valid == DEF_NO) {
            FS_TRACE_DBG(("FS_FAT_ChkBootSec(): Invalid FAT32 root dir clus: %d\n", clus_nbr));
           *p_err = FS_ERR_VOL_INVALID_SYS;
            return;
        }
        p_fat_data->RootDirStart     = FS_FAT_CLUS_TO_SEC(p_fat_data, clus_nbr);
                                                                /* Get FSINFO sec nbr.                                  */
        sec_nbr                      = MEM_VAL_GET_INT16U_LITTLE((void *)(p_temp_08 + FS_FAT_BPB_FAT32_OFF_FSINFO));
        if (sec_nbr != 0u) {
            p_fat_data->FS_InfoStart = (FS_FAT_SEC_NBR)sec_nbr;
        }
                                                                /* Get BKBOOT sec nbr.                                  */
        sec_nbr                      = MEM_VAL_GET_INT16U_LITTLE((void *)(p_temp_08 + FS_FAT_BPB_FAT32_OFF_BKBOOTSEC));
        if (sec_nbr != 0u) {
            p_fat_data->BPB_BkStart  = (FS_FAT_SEC_NBR)sec_nbr;
        }
#else
        FS_TRACE_DBG(("FS_FAT_ChkBootSec(): Invalid file sys: FAT32\n"));
       *p_err = FS_ERR_VOL_INVALID_SYS;
        return;
#endif
    }
#endif
   *p_err = FS_ERR_NONE;
}

#endif