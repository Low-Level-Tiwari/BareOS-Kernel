add_library(bareOS-Kernel-core INTERFACE)
target_sources(bareOS-Kernel-core INTERFACE
	${BAREOS_KERNEL_PATH}/task.c
        )
	target_include_directories(bareOS-Kernel-core INTERFACE ${BAREOS_KERNEL_PATH}/include)


add_library(bareOS-Kernel INTERFACE)
target_sources(bareOS-Kernel INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/port.c
)

target_include_directories(bareOS-Kernel INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/include
)

target_link_libraries(bareOS-Kernel INTERFACE
        bareOS-Kernel-core
        pico_base_headers
        hardware_clocks
        hardware_exception
        pico_multicore
)

