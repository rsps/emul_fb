/*
 * ViewBase.cpp
 *
 *  Created on: 27. aug. 2021
 *      Author: steffen
 */
#include <fctnl.h>
#include "Epoll.h"
#include "ViewBase.h"

ViewBase::ViewBase(const std::string aFrameBufferName, const std::string aVievDeviceName)
{
    mViewFd = open(aVievDeviceName.c_str(), O_RDWR);
    if (mViewFd == -1) {
        throw std::system_error(errno, std::generic_category(), "Failed to open view device");
    }

    mFrameBufFd = open(aFrameBufferName.c_str(), O_RD);
    if (mFrameBufFd == -1) {
        throw std::system_error(errno, std::generic_category(), "Failed to open frame buffer device");
    }

    void *p = mmap(0, screensize * 2, PROT_READ | PROT_WRITE, MAP_SHARED, framebufferFile, 0);
    if (p == static_cast<void*>(-1)) {
        throw std::system_error(errno, std::generic_category(), "Failed to mmap frame buffer");
    }

    mpBuffer = static_cast<uint8_t*>Ø(p);



    mpFbView = new FramebufferViewSDL(480, 800, 480, 800);

    memset(&mFbFix, 0, sizeof(mFbFix));
    strcpy(mFbFix.id, "emul_fb");
    mFbFix.line_length = sizeof(uint32_t) * GetView().GetWidth();
    mFbFix.smem_start = 0;
    mFbFix.smem_len = 0;

    memset(&mFbVar, 0, sizeof(mFbVar));
    mFbVar.xres = GetView().GetWidth();
    mFbVar.yres = GetView().GetHeight();
    mFbVar.xres_virtual = GetView().GetWidth();
    mFbVar.yres_virtual = GetView().GetHeight();
    mFbVar.bits_per_pixel = sizeof(uint32_t) * 8;

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

void ViewBase::run(const std::string aDevName)
{
    const int cMAX_EVENTS = 1;
    struct epoll_event events[cMAX_EVENTS];

    Epoll ep;
    ep.Add(mViewFd, EPOLLIN);

    while (PollEvents()) {

        int counts = ep.Wait(events, cMAX_EVENTS, 10);

        if (counts > 0) {
            Render();
        }
    }
}
