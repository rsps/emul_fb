cmake_minimum_required (VERSION 3.16)

project (emul_fb)

#set (CMAKE_VERBOSE_MAKEFILE on)

include(ExternalProject)
find_package(Git REQUIRED)

ExternalProject_Add(
    libSDL2pp

    SOURCE_DIR "${CMAKE_SOURCE_DIR}/ext-libs/libSDL2pp"
    GIT_REPOSITORY https://github.com/libSDL2pp/libSDL2pp.git
    GIT_TAG "0.16.1"
    TIMEOUT 10

    UPDATE_COMMAND ${GIT_EXECUTABLE} pull
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
    LOG_DOWNLOAD ON
)
ExternalProject_Get_Property(libSDL2pp source_dir)
#set(LIBSDL2PP_INCLUDE_DIR ${source_dir}/libSDL2pp CACHE INTERNAL "Path to include folder for libSDL2pp")

SET(SDL2PP_WITH_IMAGE ON) # if you need SDL_image support
SET(SDL2PP_WITH_MIXER OFF) # if you need SDL_mixer support
SET(SDL2PP_WITH_TTF OFF) # if you need SDL_ttf support

ADD_SUBDIRECTORY(ext-libs/libSDL2pp)

set(LIBFUSE "fuse")

set (CMAKE_CXX_STANDARD 20)

find_package(SDL2 REQUIRED)

INCLUDE_DIRECTORIES(${SDL2PP_INCLUDE_DIRS} ${SDL2_INCLUDE_DIRS})

add_executable(emul_fb emul_fb.cpp FramebufferView.cpp FramebufferDevice.cpp)

target_link_libraries(emul_fb ${LIBFUSE} ${SDL2PP_LIBRARIES} ${SDL2_LIBRARIES})
