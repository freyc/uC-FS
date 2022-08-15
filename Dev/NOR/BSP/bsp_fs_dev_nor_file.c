#include "bsp_fs_dev_nor_file.h"

#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

typedef struct {
    uint32_t version;
    uint32_t blockCount;
    uint32_t blockSize;
    uint32_t offset;
} nor_image_header;

typedef struct {
    uint16_t erase_count;
    uint16_t write_count;
} nor_block_stat;

typedef struct {
    void* data;
    size_t len;

    nor_image_header hdr;    
} bsp_mmap_data;


void *FSDev_NOR_File_BSP_Open (FS_DEV_NOR_PHY_DATA* p_phy_data)
{
    char filename[16];
    sprintf(filename, "nor%d.bin", p_phy_data->UnitNbr);

    int fd = open(filename, O_RDWR | O_CREAT,
                  S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);

    if(fd < 0) {
        return 0;
    }

    struct stat statbuf;
    int err = fstat(fd, &statbuf);

    if (err < 0) {
        close(fd);
        return 0;
    }

    nor_image_header hdr;

    if(read(fd, &hdr, sizeof(hdr)) != sizeof(hdr)) {
        hdr.version = 1;
        hdr.blockCount = 32;
        hdr.blockSize = 65536;
        hdr.offset = 1024;
        lseek(fd, 0, SEEK_SET);
        write(fd, &hdr, sizeof(hdr));
    }

    FS_SEC_SIZE flashSize = hdr.blockCount * hdr.blockSize;
    size_t map_len = flashSize + hdr.offset;

    if (map_len > statbuf.st_size) {
        ftruncate(fd, map_len);
    }

    void* data = mmap(NULL, map_len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    close(fd);

    if(data == MAP_FAILED) {
        return 0;
    }

    bsp_mmap_data *bsp_data = (bsp_mmap_data*)malloc(sizeof(bsp_mmap_data));
    
    if (bsp_data == DEF_NULL) {
        munmap(data, map_len);
        free(bsp_data);
        return 0;
    }

    bsp_data->data = data;
    bsp_data->len = map_len;
    bsp_data->hdr = hdr;
#if 0
    dev_data->Data = data;
    dev_data->Offset = hdr.offset;
    dev_data->SecCnt = hdr.blockCount;
    dev_data->SecSize = hdr.blockSize;
#endif                                                           /* ------------------- SAVE PHY INFO ------------------ */

    p_phy_data->BlkCnt          =  hdr.blockCount;              /* $$$$ Assign blk cnt  of device.                   */
    p_phy_data->BlkSize         =  hdr.blockSize;               /* $$$$ Assign blk size of device.                   */
    //p_phy_data->DataPtr         =  bsp_data;                    /* $$$$ Save any driver-specific data, if necessary. */
    p_phy_data->AddrRegionStart =  p_phy_data->AddrBase;        /* $$$$ Assign start of addr region.                  */
    


    return bsp_data;
}

void FSDev_NOR_File_BSP_Close(void* data) {
    bsp_mmap_data *bsp_data = data;

    munmap(bsp_data->data, bsp_data->len);

    free(data);
}

void FSDev_NOR_File_BSP_Read(void* data, uint32_t offset, size_t len, void* p_dest) {
    bsp_mmap_data *bsp_data = data;
    Mem_Copy(p_dest, bsp_data->data + bsp_data->hdr.offset + offset, len);
}

void FSDev_NOR_File_BSP_Write(void* data, uint32_t offset, size_t len, const void* p_src) {
    bsp_mmap_data *bsp_data = data;
    Mem_Copy(bsp_data->data + bsp_data->hdr.offset + offset, p_src, len);
}

void FSDev_NOR_File_BSP_Erase(void* data, uint32_t offset, size_t len) {
    bsp_mmap_data *bsp_data = data;
    Mem_Set(bsp_data->data + bsp_data->hdr.offset + offset, 0xFF, len);
}