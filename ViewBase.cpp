/*
 * ViewBase.cpp
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
#include <linux/kd.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <system_error>
#include <iostream>
#include "Epoll.h"
#include "ViewBase.h"
#include "log.h"

ViewBase::ViewBase(const std::string aFrameBufferName, const std::string aViewDeviceName)
{
    LOG("Emulating frame buffer in ", aFrameBufferName, " with view in ", aViewDeviceName);

    mViewFd = open(aViewDeviceName.c_str(), O_RDWR);
    if (mViewFd == -1) {
        throw std::system_error(errno, std::generic_category(), "Failed to open view device");
    }

    mFrameBufFd = open(aFrameBufferName.c_str(), O_RDONLY);
    if (mFrameBufFd == -1) {
        throw std::system_error(errno, std::generic_category(), "Failed to open frame buffer device");
    }

    int ret = ioctl(mFrameBufFd, FBIOGET_VSCREENINFO, &mFbVar);
    if (ret == -1) {
        throw std::system_error(errno, std::generic_category(), "Failed to get variable screen info");
    }
    ret = ioctl(mFrameBufFd, FBIOGET_FSCREENINFO, &mFbFix);
    if (ret == -1) {
        throw std::system_error(errno, std::generic_category(), "Failed to get variable screen info");
    }

    LOG("smem_start: ", mFbFix.smem_start, ", smem_len: ", mFbFix.smem_len, ", bpp: ", mFbVar.bits_per_pixel);

    // We only support 32-bit colors for now.
    if (mFbVar.bits_per_pixel != 32) {
        throw std::runtime_error("Frame buffer reports unsupported color depth.");
    }

//    void *p = mmap((void*)mFbFix.smem_start, mFbFix.smem_len, PROT_READ, MAP_SHARED, mFrameBufFd, 0);
    void *p = mmap(0, mFbFix.smem_len, PROT_READ, MAP_SHARED, mFrameBufFd, 0);
    if (p == reinterpret_cast<void*>(-1)) {
        throw std::system_error(errno, std::generic_category(), "Failed to mmap frame buffer");
    }

    mpBuffer = static_cast<uint32_t*>(p);
}

ViewBase::~ViewBase()
{
    if (mViewFd > 0) {
        close(mViewFd);
    }

    if (mFrameBufFd > 0) {
        close(mFrameBufFd);
    }
}

void ViewBase::run()
{
    const int cMAX_EVENTS = 1;
    struct epoll_event events[cMAX_EVENTS];

    Epoll ep;
    ep.Add(mViewFd, EPOLLIN);
    int counts = 1;

    while (PollEvents()) {
        if (counts > 0) {
            size_t bytes = read(mViewFd, &mFbVar, sizeof(mFbVar));
            if (bytes == -1) {
                throw std::system_error(errno, std::generic_category(), "Failed to read from fb_view");
            }
            LOG("Read: ", bytes, ", should be: ", sizeof(mFbVar), ", yoffset: ", mFbVar.yoffset);
            Resize(mFbVar.xres, mFbVar.yres);
            Render();
        }
        counts = ep.Wait(events, cMAX_EVENTS, 10);
    }
}

