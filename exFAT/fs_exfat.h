
#include  "../Source/fs_cfg_fs.h"
#include  "../Source/fs_ctr.h"
#include  "../Source/fs_type.h"

#ifdef   FS_EXFAT_MODULE_PRESENT

#include  <Source/clk.h>
#include  <cpu.h>
#include  "../Source/fs_err.h"
#include  "../Source/fs_util.h"

#include "fs_exfat_file.h"


void             FS_exFAT_ModuleInit           (FS_QTY             vol_cnt,     /* Initialize FAT system driver.        */
                                                FS_QTY             file_cnt,
                                                FS_QTY             dir_cnt,
                                                FS_ERR            *p_err);

void             FS_exFAT_VolClose             (FS_VOL            *p_vol);      /* Close a volume.                      */

#if (FS_CFG_RD_ONLY_EN == DEF_DISABLED)
void             FS_exFAT_VolFmt               (FS_VOL            *p_vol,       /* Create a volume.                     */
                                                void              *p_sys_cfg,
                                                FS_SEC_SIZE        sec_size,
                                                FS_SEC_QTY         size,
                                                FS_ERR            *p_err);
#endif

void             FS_exFAT_VolLabelGet          (FS_VOL            *p_vol,       /* Get volume label.                    */
                                                CPU_CHAR          *label,
                                                CPU_SIZE_T         len_max,
                                                FS_ERR            *p_err);

#if (FS_CFG_RD_ONLY_EN == DEF_DISABLED)
void             FS_exFAT_VolLabelSet          (FS_VOL            *p_vol,       /* Set volume label.                    */
                                                CPU_CHAR          *label,
                                                FS_ERR            *p_err);
#endif

void             FS_exFAT_VolOpen              (FS_VOL            *p_vol,       /* Open a volume.                       */
                                                FS_ERR            *p_err);

void             FS_exFAT_VolQuery             (FS_VOL            *p_vol,       /* Get info about a volume.             */
                                                FS_SYS_INFO       *p_info,
                                                FS_ERR            *p_err);





/* TODO: just here to fix compilation errors */
void             FS_FAT_LowEntryFind           (FS_VOL            *p_vol,       /* Find entry (low-level).              */
                                                FS_FAT_FILE_DATA  *p_entry_data,
                                                CPU_CHAR          *name_entry,
                                                FS_FLAGS           mode,
                                                FS_ERR            *p_err);

#define  FS_FAT_MODE_NONE                        FS_FILE_ACCESS_MODE_NONE
#define  FS_FAT_MODE_RD                          FS_FILE_ACCESS_MODE_RD
#define  FS_FAT_MODE_WR                          FS_FILE_ACCESS_MODE_WR
#define  FS_FAT_MODE_CREATE                      FS_FILE_ACCESS_MODE_CREATE
#define  FS_FAT_MODE_TRUNCATE                    FS_FILE_ACCESS_MODE_TRUNCATE
#define  FS_FAT_MODE_APPEND                      FS_FILE_ACCESS_MODE_APPEND
#define  FS_FAT_MODE_MUST_CREATE                 FS_FILE_ACCESS_MODE_EXCL
#define  FS_FAT_MODE_CACHED                      FS_FILE_ACCESS_MODE_CACHED
#define  FS_FAT_MODE_DEL                         DEF_BIT_07
#define  FS_FAT_MODE_DIR                         DEF_BIT_08
#define  FS_FAT_MODE_FILE                        DEF_BIT_09




#endif