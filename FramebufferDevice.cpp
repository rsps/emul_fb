/*
 * FramebufferDevice.cpp
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
#define FUSE_USE_VERSION 30
#define _FILE_OFFSET_BITS 64
#include <fuse/cuse_lowlevel.h>
#include <fuse/fuse_opt.h>

#include <linux/fb.h>

#include <string>
#include <iostream>
#include <cstring>
#include <system_error>

#include "FramebufferDevice.h"

static void open(fuse_req_t req, struct fuse_file_info *fi)
{
    LOG("open");
    fuse_reply_open(req, fi);
}

static void read(fuse_req_t req, size_t size, off_t off, struct fuse_file_info *fi)
{
    LOG("read");
    fuse_reply_buf(req, "Hello", size > 5 ? 5 : size);
}

static void write(fuse_req_t req, const char *buf, size_t size, off_t off, struct fuse_file_info *fi)
{
    LOG("write (%zu bytes)", size);
    fuse_reply_write(req, size);
}

static void ioctl(fuse_req_t req, int cmd, void *arg, struct fuse_file_info *fi, unsigned flags, const void *in_buf, size_t in_bufsz, size_t out_bufsz)
{

    LOG("ioctl %d: insize: %zu outsize: %zu", cmd, in_bufsz, out_bufsz);
    switch (cmd) {
        case FBIOGET_FSCREENINFO:
            if (in_bufsz == 0) {
                struct iovec iov = { arg, sizeof(FramebufferDevice::Get().mFbFix) };
                fuse_reply_ioctl_retry(req, &iov, 1, &iov, 1);

            }
            else {
                fuse_reply_ioctl(req, 0, &FramebufferDevice::Get().mFbFix, sizeof(FramebufferDevice::Get().mFbFix));
            }
            break;

        case FBIOGET_VSCREENINFO:
            if (in_bufsz == 0) {
                struct iovec iov = { arg, sizeof(FramebufferDevice::Get().mFbVar) };
                fuse_reply_ioctl_retry(req, &iov, 1, &iov, 1);
            }
            else {
                fuse_reply_ioctl(req, 0, &FramebufferDevice::Get().mFbVar, sizeof(FramebufferDevice::Get().mFbVar));
            }
            break;

        case 23:
            if (in_bufsz == 0) {
                struct iovec iov = { arg, sizeof(int) };
                fuse_reply_ioctl_retry(req, &iov, 1, NULL, 0);
            }
            else {
                LOG("  got value: %d", *((int*)in_buf));
                fuse_reply_ioctl(req, 0, NULL, 0);
            }
            break;

        case 42:
            if (out_bufsz == 0) {
                struct iovec iov = { arg, sizeof(int) };
                fuse_reply_ioctl_retry(req, NULL, 0, &iov, 1);
            }
            else {
                LOG("  write back value");
                int v = 42;
                fuse_reply_ioctl(req, 0, &v, sizeof(int));
            }
            break;
    }
}

static const struct cuse_lowlevel_ops cuse_clop = {
    .open = open,
    .read = read,
    .write = write,
    .ioctl = ioctl
};


FramebufferDevice& FramebufferDevice::Get()
{
    FramebufferDevice *instance = nullptr;

    if (!instance) {
        instance = new FramebufferDevice();
    }

    return *instance;
}

FramebufferDevice::FramebufferDevice()
    : mFbView(480, 800)
{
    memset(&mFbFix, 0, sizeof(mFbFix));
    strcpy(mFbFix.id, "emul_fb");
    mFbFix.line_length = sizeof(uint32_t) * mFbView.GetWidth();

    memset(&mFbVar, 0, sizeof(mFbVar));
    mFbVar.xres_virtual = mFbView.GetWidth();
    mFbVar.yres_virtual = mFbView.GetHeight();
    mFbVar.bits_per_pixel = sizeof(uint32_t);
}

FramebufferDevice::~FramebufferDevice()
{
}

void FramebufferDevice::run(const std::string aDevName)
{
    const char *cusearg[] = { "test", "-f", "-d" }; // -d for debug -s for single thread
    std::string dev_name = "DEVNAME=" + aDevName;
    const char *devarg[] = { dev_name.data() };
    struct cuse_info ci;

    memset(&ci, 0x00, sizeof(ci));
    ci.flags = CUSE_UNRESTRICTED_IOCTL;
    ci.dev_info_argc = 1;
    ci.dev_info_argv = devarg;

    mFbView.Render();

    if (cuse_lowlevel_main(3, (char**)&cusearg, &ci, &cuse_clop, NULL) != 0) {
        throw std::system_error(errno, std::generic_category(), "cuse failed");
    }
}


