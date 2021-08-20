cmake_minimum_required (VERSION 3.16)

project (vfb2)

set(CMAKE_VERBOSE_MAKEFILE on)

set(kerneldir "" CACHE STRING "Path to the kernel build directory")
if("${kerneldir}" STREQUAL "")
  execute_process(COMMAND uname -r OUTPUT_VARIABLE uname_r
                  OUTPUT_STRIP_TRAILING_WHITESPACE)
  set(kerneldir "/lib/modules/${uname_r}/build")
endif()
find_file(kernel_makefile NAMES Makefile
                          PATHS ${kerneldir} NO_DEFAULT_PATH)
if(NOT kernel_makefile)
  message(FATAL_ERROR "There is no Makefile in kerneldir!")
endif()


add_library(vfb2-lib STATIC source1.c source2.c)
get_target_property(module_sources vfb2-lib SOURCES)
list(APPEND module_sources vfb2.c)


string(REPLACE ";" " " module_sources_string "${module_sources}")
configure_file(Kbuild.in Kbuild @ONLY)


foreach(src ${module_sources})
  configure_file(${src} ${src} COPYONLY)
endforeach()


set(module_cmd ${CMAKE_MAKE_PROGRAM} -C ${kerneldir} M=${CMAKE_CURRENT_BINARY_DIR})
add_custom_command(OUTPUT vfb2.ko
  COMMAND ${module_cmd} modules
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  DEPENDS ${module_sources} ${CMAKE_CURRENT_BINARY_DIR}/Kbuild
  VERBATIM)
add_custom_target(module DEPENDS vfb2.ko)
add_custom_target(module-clean COMMAND ${module_cmd} clean)

