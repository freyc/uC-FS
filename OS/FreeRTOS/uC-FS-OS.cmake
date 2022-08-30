if(NOT TARGET freertos_kernel)
    message(FATAL_ERROR "uC-FS requested uC-OS3 as OS-backend, but target freertos_kernel does not exist")
endif()

target_link_libraries(uC-FS PUBLIC freertos_kernel uC-Clk)
