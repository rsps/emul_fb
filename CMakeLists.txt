cmake_minimum_required (VERSION 3.16)

project (emul_fb)

set(CMAKE_VERBOSE_MAKEFILE off)

add_subdirectory(driver)

include(FetchContent)
find_package(Git REQUIRED)

FetchContent_Declare(
    libSDL2pp
    GIT_REPOSITORY https://github.com/libSDL2pp/libSDL2pp.git
    GIT_TAG        "0.16.1"
)

FetchContent_GetProperties(libSDL2pp)
if(NOT libSDL2pp_POPULATED)
    FetchContent_Populate(libSDL2pp)

    SET(SDL2PP_WITH_IMAGE OFF) # if you need SDL_image support
    SET(SDL2PP_WITH_MIXER OFF) # if you need SDL_mixer support
    SET(SDL2PP_WITH_TTF OFF) # if you need SDL_ttf support
    add_subdirectory(${libsdl2pp_SOURCE_DIR} ${libsdl2pp_BINARY_DIR})
endif()


set(CMAKE_CXX_STANDARD 20)

find_package(SDL2 REQUIRED)

INCLUDE_DIRECTORIES(${SDL2PP_INCLUDE_DIRS} ${SDL2_INCLUDE_DIRS} /usr/include/c++/9/)
unset(CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES)

add_executable(emul_fb emul_fb.cpp Epoll.cpp FramebufferViewSDL.cpp ViewBase.cpp)

target_link_libraries(emul_fb ${SDL2PP_LIBRARIES} ${SDL2_LIBRARIES})


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

