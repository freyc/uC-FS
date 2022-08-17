#pragma once

#include  "../Source/fs_cfg_fs.h"
#include  "../Source/fs_ctr.h"
#include  "../Source/fs_type.h"

#include "../Source/fs_err.h"

typedef  struct  fs_fat_file_data  FS_FAT_FILE_DATA;

typedef  CPU_INT32U  FS_FAT_CLUS_NBR;                           /* Number of clusters/cluster index.                    */
typedef  CPU_INT32U  FS_FAT_SEC_NBR;                            /* Number of sectors/sector index.                      */
typedef  CPU_INT16U  FS_FAT_DATE;                               /* FAT date.                                            */
typedef  CPU_INT16U  FS_FAT_TIME;                               /* FAT time.                                            */
typedef  CPU_INT08U  FS_FAT_DIR_ENTRY_QTY;                      /* Quantity of directory entries.                       */
typedef  CPU_INT32U  FS_FAT_FILE_SIZE;                          /* Size of file, in octets.                             */

struct  fs_fat_file_data {
    FS_FAT_FILE_SIZE          FilePos;                          /* Current file pos.                                    */
    FS_FAT_FILE_SIZE          FileSize;                         /* Nbr octets in file.                                  */
    CPU_BOOLEAN               UpdateReqd;                       /* Dir sec update req'd.                                */
    FS_FLAGS                  Mode;                             /* Access mode.                                         */

    FS_FAT_SEC_NBR            DirFirstSec;                      /* First sec nbr of file's parent dir.                  */
    FS_FAT_SEC_NBR            DirStartSec;                      /* Sec  nbr of file's first dir entry.                  */
    FS_SEC_SIZE               DirStartSecPos;                   /* Pos of      file's first dir entry in sec.           */
    FS_FAT_SEC_NBR            DirEndSec;                        /* Sec  nbr of file's last  dir entry.                  */
    FS_SEC_SIZE               DirEndSecPos;                     /* Pos of      file's last  dir entry in sec.           */

    FS_FAT_CLUS_NBR           FileFirstClus;                    /* Clus nbr of first file clus.                         */
    FS_FAT_SEC_NBR            FileCurSec;                       /* Sec  nbr of cur   file sec.                          */
    FS_SEC_SIZE               FileCurSecPos;                    /* Pos      of cur   file pos in sec.                   */

    FS_FLAGS                  Attrib;                           /* File attrib.                                         */
    FS_FAT_DATE               DateCreate;                       /* File creation date.                                  */
    FS_FAT_TIME               TimeCreate;                       /* File creation time.                                  */
    FS_FAT_DATE               DateAccess;                       /* File last access date.                               */
    FS_FAT_DATE               DateWr;                           /* File last wr  date.                                  */
    FS_FAT_TIME               TimeWr;                           /* File last wr  time.                                  */
};


void          FS_exFAT_FileModuleInit(FS_QTY          file_cnt,   /* Init FAT dir module.                                 */
                                      FS_ERR         *p_err);


void          FS_exFAT_FileClose   (FS_FILE        *p_file,     /* Close a file.                                        */
                                    FS_ERR         *p_err);

void          FS_exFAT_FileOpen    (FS_FILE        *p_file,     /* Open a file.                                         */
                                    CPU_CHAR       *name_file,
                                    FS_ERR         *p_err);

void          FS_exFAT_FilePosSet  (FS_FILE        *p_file,     /* Set file position indicator.                         */
                                    FS_FILE_SIZE    pos_new,
                                    FS_ERR         *p_err);

void          FS_exFAT_FileQuery   (FS_FILE        *p_file,     /* Get info about file.                                 */
                                    FS_ENTRY_INFO  *p_info,
                                    FS_ERR         *p_err);

CPU_SIZE_T    FS_exFAT_FileRd      (FS_FILE        *p_file,     /* Read from a file.                                    */
                                    void           *p_dest,
                                    CPU_SIZE_T      size,
                                    FS_ERR         *p_err);

#if (FS_CFG_RD_ONLY_EN == DEF_DISABLED)
void          FS_exFAT_FileTruncate(FS_FILE        *p_file,     /* Truncate a file.                                     */
                                    FS_FILE_SIZE    size,
                                    FS_ERR         *p_err);
#endif

#if (FS_CFG_RD_ONLY_EN == DEF_DISABLED)
CPU_SIZE_T    FS_exFAT_FileWr      (FS_FILE        *p_file,     /* Write to a file.                                     */
                                    void           *p_src,
                                    CPU_SIZE_T      size,
                                    FS_ERR         *p_err);
#endif

