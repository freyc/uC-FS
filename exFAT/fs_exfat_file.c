#include "fs_exfat_file.h"


void          FS_exFAT_FileClose   (FS_FILE        *p_file,     /* Close a file.                                        */
                                    FS_ERR         *p_err) {}

void          FS_exFAT_FileOpen    (FS_FILE        *p_file,     /* Open a file.                                         */
                                    CPU_CHAR       *name_file,
                                    FS_ERR         *p_err) {}

void          FS_exFAT_FilePosSet  (FS_FILE        *p_file,     /* Set file position indicator.                         */
                                    FS_FILE_SIZE    pos_new,
                                    FS_ERR         *p_err) {}

void          FS_exFAT_FileQuery   (FS_FILE        *p_file,     /* Get info about file.                                 */
                                    FS_ENTRY_INFO  *p_info,
                                    FS_ERR         *p_err) {}

CPU_SIZE_T    FS_exFAT_FileRd      (FS_FILE        *p_file,     /* Read from a file.                                    */
                                    void           *p_dest,
                                    CPU_SIZE_T      size,
                                    FS_ERR         *p_err) { return 0; }

#if (FS_CFG_RD_ONLY_EN == DEF_DISABLED)
void          FS_exFAT_FileTruncate(FS_FILE        *p_file,     /* Truncate a file.                                     */
                                    FS_FILE_SIZE    size,
                                    FS_ERR         *p_err) {}
#endif

#if (FS_CFG_RD_ONLY_EN == DEF_DISABLED)
CPU_SIZE_T    FS_exFAT_FileWr      (FS_FILE        *p_file,     /* Write to a file.                                     */
                                    void           *p_src,
                                    CPU_SIZE_T      size,
                                    FS_ERR         *p_err) {return 0;}
#endif
