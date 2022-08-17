set(UCFS_DIR ${CMAKE_CURRENT_SOURCE_DIR})

if(NOT UCFS_CFG_DIR)
    message(FATAL_ERROR "UCFS_CFG_DIR must be set")
endif()

if(NOT UCFS_OS_PORT)

    if(TARGET uC-OS3)
        message(STATUS "Found uC-OS3; using it as uC-FS OS-backend")
        set(UCFS_OS_PORT uCOS-III)
    elseif(TARGET uC-OS2)
        message(STATUS "Found uC-OS2; using it as uC-FS OS-backend")
        set(UCFS_OS_PORT uCOS-II)
    else()
        message(WARN "UCFS_OS_PORT is not set; defaulting to None")
        set(UCFS_OS_PORT None)
    endif()
endif()

option(UCFS_DEV_RAMDISK_SUPPORT "Include RAMDisk-driver" OFF)
option(UCFS_DEV_NOR_SUPPORT     "Include NOR-flash-driver" OFF)
option(UCFS_DEV_NAND_SUPPORT    "Include NAND-flash-driver" OFF)

add_library(uC-FS STATIC
    # uC-FS sources
    ${UCFS_DIR}/Source/fs_api.c
    ${UCFS_DIR}/Source/fs_api.h
    ${UCFS_DIR}/Source/fs_buf.c
    ${UCFS_DIR}/Source/fs_buf.h
    ${UCFS_DIR}/Source/fs.c
    ${UCFS_DIR}/Source/fs_cache.c
    ${UCFS_DIR}/Source/fs_cache.h
    ${UCFS_DIR}/Source/fs_cfg_fs.h
    ${UCFS_DIR}/Source/fs_ctr.h
    ${UCFS_DIR}/Source/fs_def.h
    ${UCFS_DIR}/Source/fs_dev.c
    ${UCFS_DIR}/Source/fs_dev.h
    ${UCFS_DIR}/Source/fs_dir.c
    ${UCFS_DIR}/Source/fs_dir.h
    ${UCFS_DIR}/Source/fs_entry.c
    ${UCFS_DIR}/Source/fs_entry.h
    ${UCFS_DIR}/Source/fs_err.h
    ${UCFS_DIR}/Source/fs_file.c
    ${UCFS_DIR}/Source/fs_file.h
    ${UCFS_DIR}/Source/fs.h
    ${UCFS_DIR}/Source/fs_partition.c
    ${UCFS_DIR}/Source/fs_partition.h
    ${UCFS_DIR}/Source/fs_sys.c
    ${UCFS_DIR}/Source/fs_sys.h
    ${UCFS_DIR}/Source/fs_type.h
    ${UCFS_DIR}/Source/fs_unicode.c
    ${UCFS_DIR}/Source/fs_unicode.h
    ${UCFS_DIR}/Source/fs_util.c
    ${UCFS_DIR}/Source/fs_util.h
    ${UCFS_DIR}/Source/fs_vol.c
    ${UCFS_DIR}/Source/fs_vol.h

    # FAT sources
    ${UCFS_DIR}/FAT/fs_fat.c
    ${UCFS_DIR}/FAT/fs_fat_dir.c
    ${UCFS_DIR}/FAT/fs_fat_dir.h
    ${UCFS_DIR}/FAT/fs_fat_entry.c
    ${UCFS_DIR}/FAT/fs_fat_entry.h
    ${UCFS_DIR}/FAT/fs_fat_fat12.c
    ${UCFS_DIR}/FAT/fs_fat_fat12.h
    ${UCFS_DIR}/FAT/fs_fat_fat16.c
    ${UCFS_DIR}/FAT/fs_fat_fat16.h
    ${UCFS_DIR}/FAT/fs_fat_fat32.c
    ${UCFS_DIR}/FAT/fs_fat_fat32.h
    ${UCFS_DIR}/FAT/fs_fat_file.c
    ${UCFS_DIR}/FAT/fs_fat_file.h
    ${UCFS_DIR}/FAT/fs_fat.h
    ${UCFS_DIR}/FAT/fs_fat_journal.c
    ${UCFS_DIR}/FAT/fs_fat_journal.h
    ${UCFS_DIR}/FAT/fs_fat_lfn.c
    ${UCFS_DIR}/FAT/fs_fat_lfn.h
    ${UCFS_DIR}/FAT/fs_fat_sfn.c
    ${UCFS_DIR}/FAT/fs_fat_sfn.h
    ${UCFS_DIR}/FAT/fs_fat_type.h

    # exFAT sources
    ${UCFS_DIR}/exFAT/fs_exfat.c
    ${UCFS_DIR}/exFAT/fs_exfat_file.c
    ${UCFS_DIR}/exFAT/fs_exfat_entry.c
)

if(UCFS_DEV_FILEDISK_SUPPORT)
    target_sources(uC-FS PRIVATE 
        ${UCFS_DIR}/Dev/FileDisk/fs_dev_filedisk.c
        ${UCFS_DIR}/Dev/FileDisk/fs_dev_filedisk.h)
    target_compile_definitions(uC-FS PUBLIC -DUCFS_HAVE_FILEDISK_SUPPORT=1)
endif()

if(UCFS_DEV_RAMDISK_SUPPORT)
    target_sources(uC-FS PRIVATE 
        ${UCFS_DIR}/Dev/RAMDisk/fs_dev_ramdisk.c
        ${UCFS_DIR}/Dev/RAMDisk/fs_dev_ramdisk.h)
    target_compile_definitions(uC-FS PUBLIC -DUCFS_HAVE_RAMDISK_SUPPORT=1)
endif()

if(UCFS_DEV_NOR_SUPPORT)
    target_sources(uC-FS PRIVATE 
        ${UCFS_DIR}/Dev/NOR/fs_dev_nor.c
        ${UCFS_DIR}/Dev/NOR/fs_dev_nor.h
        ${UCFS_DIR}/Dev/NOR/PHY/fs_dev_nor_mmap.c
        ${UCFS_DIR}/Dev/NOR/PHY/fs_dev_nor_mmap.h
        ${UCFS_DIR}/Dev/NOR/BSP/bsp_fs_dev_nor_file.c
        )
    target_compile_definitions(uC-FS PUBLIC -DUCFS_HAVE_NOR_SUPPORT=1)
endif()

if(UCFS_DEV_NAND_SUPPORT)
    target_sources(uC-FS PRIVATE 
        ${UCFS_DIR}/Dev/NAND/fs_dev_nand.c
        ${UCFS_DIR}/Dev/NAND/fs_dev_nand.h
        

        #${UCFS_DIR}/Dev/NAND/Ctrlr/fs_dev_nand_ctrlr_gen.c
        #${UCFS_DIR}/Dev/NAND/Ctrlr/fs_dev_nand_ctrlr_gen.h

        ${UCFS_DIR}/Dev/NAND/Ctrlr/fs_dev_nand_ctrlr_file.c
        ${UCFS_DIR}/Dev/NAND/Ctrlr/fs_dev_nand_ctrlr_file.h

        ${UCFS_DIR}/Dev/NAND/Ctrlr/FileExt/fs_dev_nand_ctrlr_file_soft_ecc.c

        ${UCFS_DIR}/Dev/NAND/BSP/bsp_fs_dev_nand_ctrlr_file.c

        ${UCFS_DIR}/Dev/NAND/Part/fs_dev_nand_part_static.c)
    target_compile_definitions(uC-FS PUBLIC -DUCFS_HAVE_NAND_SUPPORT=1)
    target_link_libraries(uC-FS PUBLIC uC-CRC)
endif()

target_include_directories(uC-FS PUBLIC ${UCFS_CFG_DIR})
target_include_directories(uC-FS PRIVATE ${UCFS_DIR})

target_include_directories(uC-FS PUBLIC ${UCFS_DIR}/Source)
target_include_directories(uC-FS PUBLIC ${UCFS_DIR}/Dev)

target_compile_definitions(uC-FS PUBLIC -DHAVE_UCFS=1)

include(OS/${UCFS_OS_PORT}/uC-FS-OS.cmake)

target_sources(uC-FS PRIVATE 
    ${UCFS_DIR}/OS/${UCFS_OS_PORT}/fs_os.c
    ${UCFS_DIR}/OS/${UCFS_OS_PORT}/fs_os.h)

target_compile_options(uC-FS PRIVATE -Wno-format)
