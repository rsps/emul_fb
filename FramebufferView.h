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
#ifndef FRAMEBUFFERVIEW_H_
#define FRAMEBUFFERVIEW_H_

#include <SDL2pp/SDL2pp.hh>


class FramebufferView
{
public:
    FramebufferView(int aHeight, int aWidth);
    virtual ~FramebufferView();

    void Render();
    bool PollEvents();

    int GetHeight() { return mHeight; }
    int GetWidth()  { return mWidth; }

    uint32_t GetPixel(int aX, int aY) { return mpBuffer[(aY * mWidth) + aX]; }
    void SetPixel(int aX, int aY, uint32_t aValue) { mpBuffer[(aY * mWidth) + aX] = aValue; }

protected:
    SDL2pp::SDL *mpSdl;
    SDL2pp::Window *mpWindow;
    SDL2pp::Renderer *mpRenderer;
    SDL2pp::Texture *mpTexture;

    int mHeight;
    int mWidth;

    uint32_t* mpBuffer;
};

#endif /* FRAMEBUFFERVIEW_H_ */
