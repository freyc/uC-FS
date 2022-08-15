#include  <Dev/NOR/fs_dev_nor.h>

void *FSDev_NOR_File_BSP_Open (FS_DEV_NOR_PHY_DATA* p_phy_data);

void FSDev_NOR_File_BSP_Close(void* data);

void FSDev_NOR_File_BSP_Read(void* data, uint32_t offset, size_t len, void* p_dest);

void FSDev_NOR_File_BSP_Write(void* data, uint32_t offset, size_t len, const void* p_src);

void FSDev_NOR_File_BSP_Erase(void* data, uint32_t offset, size_t len);