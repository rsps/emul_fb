/*
 * emul_fb.cpp
 *
 * Copyright (C) 2021 RSP Systems <software@rspsystems.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <iostream>
#include <exception>
#include <string>
#include <filesystem>
#include "FramebufferViewSDL.h"

namespace fs = std::filesystem;

static std::string locateFramebufferDevice();


int main(int argc, char **argv)
{
    std::clog << "Framebuffer Emulator ver. 0.4.0 " << argc << std::endl;

    try {
        std::string fb = locateFramebufferDevice();
        if (argc > 1) {
            fb = argv[1];
        }

        FramebufferViewSDL view(fb, "/dev/fb_view");

        view.run();
    }
    catch (const std::exception &e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }

    std::clog << "Done" << std::endl;
    return 0;
}

static std::string locateFramebufferDevice()
{
    fs::path p = "/sys/devices/platform/vfb2.0/graphics/";

    if (fs::exists(p)) {
        for(auto const& dir_entry: fs::directory_iterator{p})
            return "/dev/" + dir_entry.path().stem().string();
    }
    throw std::runtime_error("Could not locate vfb2 device. Please make sure the vfb2 driver is loaded.");
}
