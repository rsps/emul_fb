/*
 * Viewer.h
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
#ifndef VIEWBASE_H_
#define VIEWBASE_H_

#include <cstdint>
#include <string>
#include <linux/fb.h>

class ViewBase
{
public:
    ViewBase(const std::string aFrameBufferName, const std::string aVievDeviceName);
    virtual ~ViewBase();

    void run();

    virtual bool PollEvents() = 0; // Return false to terminate
    virtual void Render() = 0;
    virtual void Resize(int aWidth, int aHeight) = 0;

protected:
    int mViewFd;
    int mFrameBufFd;

    uint8_t* mpBuffer = nullptr;

    struct fb_fix_screeninfo mFbFix;
    struct fb_var_screeninfo mFbVar;
};

#endif /* VIEWBASE_H_ */
