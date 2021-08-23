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
#include <fuse3/cuse_lowlevel.h>
#include <fuse3/fuse_opt.h>

#include <linux/fb.h>
#include <sys/stat.h>
#include <unistd.h>
#include <grp.h>

#include <string>
#include <iostream>
#include <cstring>
#include <system_error>
#include <stdexcept>

#include "FramebufferDevice.h"
#include "Epoll.h"
#include "log.h"

static size_t write_offset = 0;

static void drop_privileges()
{
    gid_t newgid = getgid();
    uid_t newuid = getuid();

    LOG("Dropping privileges. GID:", newgid, ", UID:", newuid);

  /* If root privileges are to be dropped, be sure to pare down the ancillary
   * groups for the process before doing anything else because the setgroups(  )
   * system call requires root privileges.  Drop ancillary groups regardless of
   * whether privileges are being dropped temporarily or permanently.
   */
    setgroups(1, &newgid);

    int res = setregid(newgid, newgid);
    if (res == -1) {
        throw std::system_error(errno, std::generic_category(), "setregid failed");
    }

    res = setreuid(newuid, newuid);
    if (res == -1) {
        throw std::system_error(errno, std::generic_category(), "setreuid failed");
    }
}

static void open(fuse_req_t req, struct fuse_file_info *fi)
{
    LOG("open");
    write_offset = 0;
    fuse_reply_open(req, fi);
}

static void read(fuse_req_t req, size_t size, off_t off, struct fuse_file_info *fi)
{
    LOG("read");
    fuse_reply_buf(req, "Hello", size > 5 ? 5 : size);
}

static void write(fuse_req_t req, const char *buf, size_t size, off_t off, struct fuse_file_info *fi)
{
    LOG("write ", size, " bytes @ offset ", static_cast<uint32_t>(off));

    uint32_t written = FramebufferDevice::Get().GetView().Write(
        reinterpret_cast<const uint32_t*>(buf),
        static_cast<uint32_t>(size) / sizeof(uint32_t),
        (static_cast<uint32_t>(off) + write_offset) / sizeof(uint32_t),
        0);

    written *= sizeof(uint32_t);
    write_offset += written;

    fuse_reply_write(req, written);
}

static void ioctl(fuse_req_t req, int cmd, void *arg, struct fuse_file_info *fi, unsigned flags, const void *in_buf, size_t in_bufsz, size_t out_bufsz)
{

    LOG("ioctl ", cmd, ": insize: ", in_bufsz, " outsize: ", out_bufsz);
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

        case FBIOPUT_VSCREENINFO:
            if (in_bufsz != sizeof(FramebufferDevice::Get().mFbVar)) {
                struct iovec iov = { arg, sizeof(FramebufferDevice::Get().mFbVar) };
                fuse_reply_ioctl_retry(req, &iov, 1, &iov, 1);
            }
            else {
                FramebufferDevice::Get().SetVScreenInfo(in_buf, in_bufsz);
                fuse_reply_ioctl(req, 0, nullptr, 0);
            }
            break;

        case FBIOPAN_DISPLAY:
            break;

        case 23:
            if (in_bufsz == 0) {
                struct iovec iov = { arg, sizeof(int) };
                fuse_reply_ioctl_retry(req, &iov, 1, NULL, 0);
            }
            else {
                LOG("  got value: ", *(reinterpret_cast<const int*>(in_buf)));
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

        default:
            break;
    }
}

static const struct cuse_lowlevel_ops cuse_clop = {
    .open = open,
    .read = read,
    .write = write,
    .ioctl = ioctl,
};


FramebufferDevice& FramebufferDevice::Get()
{
    static FramebufferDevice *instance = nullptr;

    if (!instance) {
        instance = new FramebufferDevice();
    }

    return *instance;
}

FramebufferDevice::FramebufferDevice()
{
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

FramebufferDevice::~FramebufferDevice()
{
}

void FramebufferDevice::run(const std::string aDevName)
{
    const char *cusearg[] = { "test", "-f", "-d", "-s" }; // -d for debug -s for single thread
    std::string dev_name = "DEVNAME=" + aDevName;
    const char *devarg[] = { dev_name.data() };
    struct cuse_info ci;

    mDeviceName = "/dev/" + aDevName;

    memset(&ci, 0x00, sizeof(ci));
    ci.flags = CUSE_UNRESTRICTED_IOCTL;
    ci.dev_info_argc = 1;
    ci.dev_info_argv = devarg;

    GetView().Render();

    struct fuse_session *se;
    int multithreaded;

    se = cuse_lowlevel_setup(4, const_cast<char**>(cusearg), &ci, &cuse_clop, &multithreaded, nullptr);
    if (se == nullptr) {
        throw std::system_error(errno, std::generic_category(), "cuse_lowlevel_setup failed");
    }

    try {
        if (multithreaded) {
            throw std::runtime_error("cuse_lowlevel_setup MUST NOT be multithreaded");
        }
        else {
            mpFuseSession = se;
            SessionLoop();
        }
        cuse_lowlevel_teardown(se);
    }
    catch (const std::exception &e) {
        cuse_lowlevel_teardown(se);
        throw;
    }
}

void FramebufferDevice::SetVScreenInfo(const void *in_buf, size_t in_bufsz)
{
    if (mpFbView) {
        delete mpFbView;
    }

    memcpy(&mFbVar, in_buf, in_bufsz);

    LOG("Creating Framebuffer: ", mFbVar.xres_virtual, ", ", mFbVar.yres_virtual);

    mpFbView = new FramebufferViewSDL(mFbVar.xres, mFbVar.yres, mFbVar.xres_virtual, mFbVar.yres_virtual);
}

void FramebufferDevice::SessionLoop()
{
    struct fuse_session *se = static_cast<struct fuse_session *>(mpFuseSession);
    bool first_run = true;

    int res = 0;
    struct fuse_buf fbuf = {
        .mem = NULL,
    };

    const int cMAX_EVENTS = 5;
    struct epoll_event events[cMAX_EVENTS];

    Epoll ep;
    ep.Add(fuse_session_fd(se), EPOLLIN);

    while (!fuse_session_exited(se)) {

        int counts = ep.Wait(events, cMAX_EVENTS, 10);
        if (counts > 0) {
            res = fuse_session_receive_buf(se, &fbuf);
            if (res == -EINTR)
                continue;
            if (res <= 0)
                break;

            fuse_session_process_buf(se, &fbuf);

            if (first_run) {
                first_run = false;
                int res = chmod(mDeviceName.c_str(), 0666);
                if (res < 0) {
                    throw std::system_error(errno, std::generic_category(), "chmod failed");
                }

                drop_privileges();
            }

            mpFbView->Render();
        }

        if (!mpFbView->PollEvents()) {
            fuse_session_exit(se);
        }
    }

    free(fbuf.mem);
    fuse_session_reset(se);

    if (res < 0) {
        throw std::system_error(res, std::generic_category(), "fuse_session_receive_buf failed");
    }
}

