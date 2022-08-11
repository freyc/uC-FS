if(NOT TARGET uC-OS3)
    message(FATAL_ERROR "uC-FS requested uC-OS3 as OS-backend, but target uC-OS3 does not exist")
endif()

target_link_libraries(uC-FS PUBLIC uC-OS3)
