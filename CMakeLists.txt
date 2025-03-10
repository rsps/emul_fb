cmake_minimum_required (VERSION 3.16)

project (emul_fb)

set(CMAKE_VERBOSE_MAKEFILE off)

add_subdirectory(driver)

include(FetchContent)
find_package(Git REQUIRED)

set (SDL2PP_WITH_IMAGE OFF CACHE INTERNAL "Turn off image support")
set (SDL2PP_WITH_MIXER OFF CACHE INTERNAL "Turn off audio mixer")
set (SDL2PP_WITH_TTF OFF CACHE INTERNAL "Turn off fonts")

FetchContent_Declare(
    libSDL2pp
    GIT_REPOSITORY https://github.com/libSDL2pp/libSDL2pp.git
    GIT_TAG        "0.18.1"
)

FetchContent_MakeAvailable(libSDL2pp)

set(CMAKE_CXX_STANDARD 20)

find_package(SDL2 REQUIRED)

INCLUDE_DIRECTORIES(${SDL2PP_INCLUDE_DIRS} ${SDL2_INCLUDE_DIRS})

add_executable(emul_fb emul_fb.cpp Epoll.cpp FramebufferViewSDL.cpp ViewBase.cpp)

target_link_libraries(emul_fb SDL2pp::SDL2pp ${SDL2_LIBRARIES})


include(GNUInstallDirs)

set(PROGRAM_PERMISSIONS_DEFAULT
    OWNER_WRITE OWNER_READ OWNER_EXECUTE
    GROUP_READ GROUP_EXECUTE
    WORLD_READ WORLD_EXECUTE)

install(TARGETS emul_fb
        DESTINATION ${CMAKE_INSTALL_BINDIR}
        PERMISSIONS ${PROGRAM_PERMISSIONS_DEFAULT}
)

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/emul_fb.desktop
        DESTINATION /usr/share/applications/
        PERMISSIONS OWNER_WRITE OWNER_READ GROUP_READ WORLD_READ
)

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/emul_fb.png
        DESTINATION /usr/share/icons/
        PERMISSIONS OWNER_WRITE OWNER_READ GROUP_READ WORLD_READ
)

