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

#include "FramebufferViewSDL.h"


int main(int argc, char **argv)
{
    std::cout << "Framebuffer Emulator ver. 0.3.0" << std::endl;

    try {
        FramebufferViewSDL view("/dev/fb1", "/dev/fb_view");

        view.run();
    }
    catch (const std::exception &e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }

    std::cout << "Done" << std::endl;
    return 0;
}
