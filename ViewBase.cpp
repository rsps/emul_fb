/*
 * ViewBase.cpp
 *
 *  Created on: 27. aug. 2021
 *      Author: steffen
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

ViewBase::ViewBase(const std::string aFrameBufferName, const std::string aVievDeviceName)
{
    mViewFd = open(aVievDeviceName.c_str(), O_RDWR);
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

    LOG("smem_start: ", mFbFix.smem_start, ", smem_len: ", mFbFix.smem_len);

    void *p = mmap((void*)mFbFix.smem_start, mFbFix.smem_len, PROT_READ, MAP_SHARED, mFrameBufFd, 0);
    if (p == reinterpret_cast<void*>(-1)) {
        throw std::system_error(errno, std::generic_category(), "Failed to mmap frame buffer");
    }

    mpBuffer = static_cast<uint8_t*>(p);
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

    while (PollEvents()) {

        int counts = ep.Wait(events, cMAX_EVENTS, 10);

        if (counts > 0) {
            size_t bytes = read(mViewFd, &mFbVar, sizeof(mFbVar));
            LOG("Read: ", bytes, ", should be: ", sizeof(mFbVar));
            LOG("Resize(", mFbVar.xres, ", ", mFbVar.yres, ")");
            Resize(mFbVar.xres, mFbVar.yres);
            LOG("Render()");
            Render();
        }
    }
}
