/*
 * FramebufferView.h
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
#ifndef FRAMEBUFFERVIEWSDL_H_
#define FRAMEBUFFERVIEWSDL_H_

#include <SDL2pp/SDL2pp.hh>
#include "ViewBase.h"


class FramebufferViewSDL : public ViewBase
{
public:
    FramebufferViewSDL(const std::string aFrameBufferName, const std::string aVievDeviceName);
    virtual ~FramebufferViewSDL();

    void Resize(int aWidth, int aHeight) override;
    void Render() override;
    bool PollEvents() override;

protected:
    SDL2pp::SDL *mpSdl;
    SDL2pp::Window *mpWindow;
    SDL2pp::Renderer *mpRenderer;
    SDL2pp::Texture *mpTexture;
};

#endif /* FRAMEBUFFERVIEWSDL_H_ */
