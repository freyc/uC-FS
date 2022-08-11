if(NOT TARGET uC-OS2)
    message(FATAL_ERROR "uC-FS requested uC-OS2 as OS-backend, but target uC-OS2 does not exist")
endif()

target_link_libraries(uC-FS PUBLIC uC-OS2)
