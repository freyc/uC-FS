#include "bsp_fs_dev_nand_ctrlr_file.h"


#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

typedef struct {
    uint32_t version;
    uint32_t page_size;
    uint32_t page_spare_size;
    uint32_t pages_per_block;
    uint32_t blocks;
    uint32_t data_offset;
} nand_image_header;

typedef struct {
    void *header_data;
    void *file_data;
    nand_image_header header;

    uint32_t page_size_main;
    uint32_t page_size_spare;
    uint32_t pages_per_block;
    uint32_t blocks;

    FS_NAND_PART_DATA part_data;

    uint32_t *block_erase_count;
    uint8_t *page_pgm_counter;
} bsp_mmap_data;

static void * bsp_mmap_open(const char* filename, FS_ERR* err);

static void bsp_mmap_close(void *data);

static void  bsp_mmap_read  (void         *p_bsp_data,
                             void         *p_dest,
                             CPU_INT32U    row_address,
                             CPU_INT32U    col_address,
                             CPU_SIZE_T    cnt,
                             FS_ERR       *p_err);

static void bsp_mmap_write(void         *p_bsp_data,
                             const void   *p_src,
                             CPU_INT32U    row_address,
                             CPU_INT32U    col_address,
                             CPU_SIZE_T    cnt,
                             FS_ERR       *p_err);

static void  bsp_mmap_block_erase(void       *p_bsp_data, CPU_INT32U  blkIdx, FS_ERR     *p_err);


const FS_NAND_CTRLR_FILE_BSP_API FS_NAND_BSP_MMAP = {
    bsp_mmap_open,
    bsp_mmap_close,
    bsp_mmap_write,
    bsp_mmap_read,
    bsp_mmap_block_erase
};



static void * bsp_mmap_open(const char* filename, FS_ERR* p_err) {

    int fd = open(filename, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);

    if(fd < 0) {
        *p_err = FS_ERR_DEV_INVALID_NAME;
        return (DEF_NULL);
    }

    nand_image_header hdr;

    FILE* file = fdopen(fd, "r+");
    if(fread(&hdr, sizeof(nand_image_header), 1, file) != 1) {
        hdr.version = 1u;
        hdr.page_size = 2048u;
        hdr.page_spare_size = 64u;
        hdr.pages_per_block = 64u;
        hdr.blocks = 1024u;
        hdr.data_offset = (sizeof(nand_image_header) + 1023) & ~1023u;
        fseek(file, 0, SEEK_SET);
        fwrite(&hdr, sizeof(nand_image_header), 1, file);
    }
    fd = dup(fd);
    fclose(file);
    
    struct stat statbuf;
    int err = fstat(fd, &statbuf);

    if (err < 0) {
        close(fd);
        *p_err = FS_ERR_DEV_INVALID_NAME;
        return (DEF_NULL);
    }

    uint32_t page_size_main = hdr.page_size;
    uint32_t page_size_spare = hdr.page_spare_size;
    uint32_t pages_per_block = hdr.pages_per_block;
    uint32_t blocks = hdr.blocks;
    uint32_t page_size = page_size_main + page_size_spare;

    int clearmem = 0;
    if (hdr.data_offset + (blocks * pages_per_block * page_size) > (statbuf.st_size)) {
        ftruncate(fd, hdr.data_offset + (blocks * pages_per_block * page_size));
        clearmem = 1;
    }

    

    void* data = mmap(NULL, hdr.data_offset + blocks * pages_per_block * page_size , PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    close(fd);

    if(data == MAP_FAILED) {
        *p_err = FS_ERR_DEV_IO;
        return (DEF_NULL);
    }

    if(clearmem && data) {
        Mem_Set(data + hdr.data_offset, 0xFF, blocks * pages_per_block * page_size);
    }

    bsp_mmap_data* bsp_data = (bsp_mmap_data*)malloc(sizeof(bsp_mmap_data));

    bsp_data->header_data = data;
    bsp_data->file_data = data + hdr.data_offset;
    bsp_data->header = hdr;

    bsp_data->page_size_main = hdr.page_size;
    bsp_data->page_size_spare = hdr.page_spare_size;
    bsp_data->pages_per_block = hdr.pages_per_block;
    bsp_data->blocks = hdr.blocks;

    bsp_data->part_data.BlkCnt = blocks;
    bsp_data->part_data.BusWidth = 8;
    bsp_data->part_data.DefectMarkType = 0;
    bsp_data->part_data.ECC_CodewordSize = 528;
    bsp_data->part_data.ECC_NbrCorrBits = 1;
    bsp_data->part_data.PgSize = page_size_main;
    bsp_data->part_data.SpareSize = page_size_spare;
    bsp_data->part_data.NbrPgmPerPg = 4;
    bsp_data->part_data.PgPerBlk = hdr.pages_per_block;

    size_t page_count = bsp_data->pages_per_block * bsp_data->blocks;
    bsp_data->page_pgm_counter = malloc(sizeof(uint8_t) * page_count);
    Mem_Set(bsp_data->page_pgm_counter, 0x00, sizeof(uint8_t) * page_count);

    bsp_data->block_erase_count = malloc(sizeof(uint32_t) * bsp_data->blocks);
    Mem_Set(bsp_data->block_erase_count, 0x00, sizeof(uint32_t) * bsp_data->blocks);

    return bsp_data;

}

static void bsp_mmap_close(void *data) {
    bsp_mmap_data* bsp_data = data;
    munmap(bsp_data->file_data, bsp_data->header.data_offset + bsp_data->blocks * bsp_data->pages_per_block * (bsp_data->page_size_main + bsp_data->page_size_spare));
    free(bsp_data);
}

static void  bsp_mmap_read  (void         *p_bsp_data,
                             void         *p_dest,
                             CPU_INT32U    row_address,
                             CPU_INT32U    col_address,
                             CPU_SIZE_T    cnt,
                             FS_ERR       *p_err) {
    bsp_mmap_data* bsp_data = p_bsp_data;
    Mem_Copy(
        p_dest, 
        bsp_data->file_data + row_address * (bsp_data->page_size_main + bsp_data->page_size_spare) + col_address,
        cnt);
    *p_err = FS_ERR_NONE;
}

static void bsp_mmap_write(void         *p_bsp_data,
                             const void   *p_src,
                             CPU_INT32U    row_address,
                             CPU_INT32U    col_address,
                             CPU_SIZE_T    cnt,
                             FS_ERR       *p_err) {
    bsp_mmap_data* bsp_data = p_bsp_data;
    Mem_Copy(
        bsp_data->file_data + row_address * (bsp_data->page_size_main + bsp_data->page_size_spare) + col_address, 
        p_src, 
        cnt);
    *p_err = FS_ERR_NONE;

    bsp_data->page_pgm_counter[row_address]++;
    if(bsp_data->page_pgm_counter[row_address] > bsp_data->part_data.NbrPgmPerPg) {
        printf("!!! more page-program-commands (%d) than allowed (%d) !!!\n",
        bsp_data->page_pgm_counter[row_address], bsp_data->part_data.NbrPgmPerPg);
    }
}

static void  bsp_mmap_block_erase(void       *p_bsp_data, CPU_INT32U  blkIdx, FS_ERR     *p_err) {
    bsp_mmap_data* bsp_data = p_bsp_data;
    Mem_Set(
        bsp_data->file_data + blkIdx * (bsp_data->page_size_main + bsp_data->page_size_spare) * bsp_data->pages_per_block, 
        0xFF, 
        (bsp_data->page_size_main + bsp_data->page_size_spare) * bsp_data->pages_per_block);
    
    bsp_data->block_erase_count[blkIdx]++;
    if(bsp_data->block_erase_count[blkIdx] > bsp_data->part_data.MaxBlkErase) {
        printf("!!! block-erase-count for block %d exceeded (%d) !!!\n", blkIdx, bsp_data->block_erase_count[blkIdx]);
    }

    uint32_t row_address = blkIdx * bsp_data->pages_per_block;
    Mem_Set(&bsp_data->page_pgm_counter[row_address], 0x00, sizeof(uint8_t) * bsp_data->pages_per_block);
}




static FS_NAND_PART_DATA  *file_part_open(const  FS_NAND_CTRLR_API  *p_ctrlr_api,   /* Get NAND part-specific data.             */
                                         void               *p_ctrlr_data_v,
                                         void               *p_part_cfg_v,
                                         FS_ERR             *p_err) 
{
    FS_NAND_CTRLR_FILE_DATA* p_ctrl_data = p_ctrlr_data_v;
    bsp_mmap_data* bsp_data = p_ctrl_data->BSPData;
    FS_NAND_PART_FILE_CFG* p_part_cfg = p_part_cfg_v;

    bsp_data->part_data.FreeSpareMap = p_part_cfg->FreeSpareMap;

    return &bsp_data->part_data;
}

static void file_part_close(FS_NAND_PART_DATA  *p_part_data) {}

const FS_NAND_PART_API FS_NAND_PartFile = {
    file_part_open,
    file_part_close,
};