cmake_minimum_required (VERSION 3.16)

project (vfb2 VERSION 0.1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_VERBOSE_MAKEFILE off)

# Find kernel headers
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
find_package(KernelHeaders REQUIRED)
include_directories(${KERNELHEADERS_INCLUDE_DIRS})

set(module_sources vfb2.c)

add_compile_options(
    -include${KERNELHEADERS_DIR}/include/linux/kconfig.h
    -include${KERNELHEADERS_DIR}/include/linux/compiler_types.h
    -D__KERNEL__
    -DMODULE
    )
add_library(${PROJECT_NAME} STATIC vfb2.c)


# Generate Kbuild file in build directory
string(REPLACE ";" " " module_sources_string "${module_sources}")
configure_file(Kbuild.in Kbuild @ONLY)


set(DRIVER_FILE ${PROJECT_NAME}.ko)
set(KBUILD_CMD ${CMAKE_MAKE_PROGRAM} -C ${KERNELHEADERS_DIR} modules M=${CMAKE_CURRENT_BINARY_DIR} src=${CMAKE_CURRENT_SOURCE_DIR})

add_custom_command(OUTPUT ${DRIVER_FILE}
    COMMAND ${KBUILD_CMD}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    DEPENDS ${module_sources} ${CMAKE_CURRENT_BINARY_DIR}/Kbuild
    VERBATIM)

add_custom_target(driver ALL DEPENDS ${DRIVER_FILE})


#
# Installation of the module
#
set(PROGRAM_PERMISSIONS_DEFAULT
    OWNER_WRITE OWNER_READ
    GROUP_READ
    WORLD_READ)

set(MODULES_DIR /lib/modules/${KERNEL_RELEASE}/)

# Sign the kernel module, in case uefi secure boot is enabled:
install(CODE "execute_process(COMMAND ${MODULES_DIR}build/scripts/sign-file sha512 /var/lib/shim-signed/mok/MOK.priv /var/lib/shim-signed/mok/MOK.der ${CMAKE_CURRENT_BINARY_DIR}/${DRIVER_FILE})")

# Then copy it to current module directory
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${DRIVER_FILE}
        DESTINATION ${MODULES_DIR}
        PERMISSIONS ${PROGRAM_PERMISSIONS_DEFAULT})

# And tell modprobe it exists
install(CODE "execute_process(COMMAND depmod -a)")

