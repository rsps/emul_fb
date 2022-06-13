/*
 * ViewBase.h
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

/**
 * \class ViewBase
 * \brief Abstract interface class for implementing frame buffer viewers
 *
 */
class ViewBase
{
public:
    /**
     * \fn  ViewBase(const std::string, const std::string)
     * \brief Constructor that opens the the framebuffer and the view notification device.
     *        The frame buffer is mmap'ed into the mpBuffer member variable.
     *
     *
     * \param aFrameBufferName E.g. /dev/fb0
     * \param aViewDeviceName E.g. /dev/fb_view
     */
    ViewBase(const std::string aFrameBufferName, const std::string aViewDeviceName);
    virtual ~ViewBase();

    /**
     * \fn void run()
     * \brief Application loop, polls the notification device for changes and
     *        renders the output window.
     */
    void run();

    /**
     * \fn bool PollEvents()=0
     * \brief Function to check for pending events.
     *
     * \return false to terminate the application.
     */
    virtual bool PollEvents() = 0; // Return false to terminate

    /**
     * \fn void Render()
     * \brief Called whenever the frame buffer content has changed.
     *
     */
    virtual void Render() = 0;

    /**
     * \fn void Resize(int, int)
     * \brief Called before render to let an implementation change the size of the
     *        output window in case frame buffer resolution has changed.
     *
     * \param aWidth in pixels
     * \param aHeight in pixels
     */
    virtual void Resize(int aWidth, int aHeight) = 0;

protected:
    int mViewFd;
    int mFrameBufFd;

    uint32_t* mpBuffer = nullptr;

    struct fb_fix_screeninfo mFbFix;
    struct fb_var_screeninfo mFbVar;
};

#endif /* VIEWBASE_H_ */
