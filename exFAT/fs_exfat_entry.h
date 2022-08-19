#include "fs_exfat.h"
#include "fs_err.h"

#if (FS_CFG_RD_ONLY_EN == DEF_DISABLED)
void      FS_exFAT_EntryAttribSet(FS_VOL         *p_vol,          /* Set file or directory's attributes.                  */
                                  CPU_CHAR       *name_entry,
                                  FS_FLAGS        attrib,
                                  FS_ERR         *p_err);

void      FS_exFAT_EntryCreate   (FS_VOL         *p_vol,          /* Create a file or directory.                          */
                                  CPU_CHAR       *name_entry,
                                  FS_FLAGS        entry_type,
                                  CPU_BOOLEAN     excl,
                                  FS_ERR         *p_err);

void      FS_exFAT_EntryDel      (FS_VOL         *p_vol,          /* Delete a file or directory.                          */
                                  CPU_CHAR       *name_entry,
                                  FS_FLAGS        entry_type,
                                  FS_ERR         *p_err);
#endif

void      FS_exFAT_EntryQuery    (FS_VOL         *p_vol,          /* Delete a file or directory.                          */
                                  CPU_CHAR       *name_entry,
                                  FS_ENTRY_INFO  *p_info,
                                  FS_ERR         *p_err);

#if (FS_CFG_RD_ONLY_EN == DEF_DISABLED)
void      FS_exFAT_EntryRename   (FS_VOL         *p_vol,          /* Rename a file or directory.                          */
                                  CPU_CHAR       *name_entry_old,
                                  CPU_CHAR       *name_entry_new,
                                  CPU_BOOLEAN     excl,
                                  FS_ERR         *p_err);

void      FS_exFAT_EntryTimeSet  (FS_VOL         *p_vol,          /* Set file or directory's date/time.                   */
                                  CPU_CHAR       *name_entry,
                                  CLK_DATE_TIME  *p_time,
                                  CPU_INT08U      time_type,
                                  FS_ERR         *p_err);
#endif
