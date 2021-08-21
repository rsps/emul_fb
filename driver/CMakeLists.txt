cmake_minimum_required (VERSION 3.16)

project (vfb2 VERSION 0.1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_VERBOSE_MAKEFILE on)

# Find kernel headers
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
find_package(KernelHeaders REQUIRED)
include_directories(${KERNELHEADERS_INCLUDE_DIRS})

#include_directories(/lib/modules/5.4.0-81-generic/build/include/ /lib/modules/5.4.0-81-generic/build/arch/x86/include/)

set(module_sources vfb2.c)

add_library(${PROJECT_NAME}-lib STATIC vfb2-test.c)
#get_target_property(module_sources mymodule-lib SOURCES)
#list(APPEND module_sources module.c)


# Generate Kbuild file in build directory
string(REPLACE ";" " " module_sources_string "${module_sources}")
configure_file(Kbuild.in Kbuild @ONLY)

# Copy sources to build directory
#foreach(src ${module_sources})
#    configure_file(${src} ${src} COPYONLY)
#endforeach()

set(DRIVER_FILE ${PROJECT_NAME}.ko)
set(KBUILD_CMD ${CMAKE_MAKE_PROGRAM} -C ${KERNELHEADERS_DIR} modules M=${CMAKE_CURRENT_BINARY_DIR} src=${CMAKE_CURRENT_SOURCE_DIR})
#set(KBUILD_CMD ${CMAKE_MAKE_PROGRAM} -C ${KERNELHEADERS_DIR} modules M=${CMAKE_CURRENT_BINARY_DIR})

add_custom_command(OUTPUT ${DRIVER_FILE}
    COMMAND ${KBUILD_CMD}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    DEPENDS ${module_sources} ${CMAKE_CURRENT_BINARY_DIR}/Kbuild
    VERBATIM)

add_custom_target(driver ALL DEPENDS ${DRIVER_FILE})



#set(module_cmd ${CMAKE_MAKE_PROGRAM} -C ${kerneldir} M=${CMAKE_CURRENT_BINARY_DIR})
#add_custom_command(OUTPUT vfb2.ko
#  COMMAND ${module_cmd} modules
#  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
#  DEPENDS ${module_sources} ${CMAKE_CURRENT_BINARY_DIR}/Kbuild
#  VERBATIM)
#add_custom_target(vfb2 DEPENDS vfb2.ko)
#add_custom_target(vfb2-clean COMMAND ${module_cmd} clean)

