/*
 * FramebufferDevice.h
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

#ifndef FRAMEBUFFERDEVICE_H_
#define FRAMEBUFFERDEVICE_H_

#include <linux/fb.h>
#include "FramebufferViewSDL.h"

class FramebufferDevice
{
public:
    struct fb_fix_screeninfo mFbFix;
    struct fb_var_screeninfo mFbVar;

    static FramebufferDevice& Get();

    void SetVScreenInfo(const void *in_buf, size_t in_bufsz);

    FramebufferViewSDL& GetView() { return *mpFbView; }

    void run(const std::string aDevName);

protected:
    FramebufferViewSDL* mpFbView;
    void* mpFuseSession = nullptr;

    FramebufferDevice();
    FramebufferDevice(const FramebufferDevice&) = delete;
    virtual ~FramebufferDevice();

    void SessionLoop();
};

#endif /* FRAMEBUFFERDEVICE_H_ */
