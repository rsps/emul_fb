/*
 * FramebufferView.cpp
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

#include <SDL2pp/SDL2pp.hh>
#include <SDL2/SDL.h>
#include <cstring>
#include <algorithm>
#include "FramebufferViewSDL.h"
#include "log.h"

using namespace SDL2pp;

FramebufferViewSDL::FramebufferViewSDL(const std::string aFrameBufferName, const std::string aViewDeviceName)
    : ViewBase(aFrameBufferName, aViewDeviceName)
{
    mpSdl = new SDL(SDL_INIT_VIDEO);

    mpWindow = new Window("Framebuffer Emulator",
            SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
            480, 800,
            SDL_WINDOW_SHOWN | SDL_WINDOW_MOUSE_FOCUS | SDL_WINDOW_MOUSE_CAPTURE);

    // Create accelerated video renderer with default driver
    mpRenderer = new Renderer(*mpWindow, -1, SDL_RENDERER_ACCELERATED);
    mpRenderer->SetDrawBlendMode(SDL_BLENDMODE_NONE);

    mpTexture = new Texture(*mpRenderer, SDL_PIXELFORMAT_XBGR8888, SDL_TEXTUREACCESS_STREAMING, 480, 800);
//    mpTexture->SetBlendMode(SDL_BLENDMODE_BLEND);
}

FramebufferViewSDL::~FramebufferViewSDL()
{
    delete mpTexture;
    delete mpRenderer;
    delete mpWindow;
    delete mpSdl;
}

/**
 * \fn void Render()
 * \brief Render the entire buffer contents in the window
 *
 */
void FramebufferViewSDL::Render()
{
    {
        auto lock = mpTexture->Lock();
        uint32_t* pixels = static_cast<uint32_t*>(lock.GetPixels());

        LOG("xres: ", mFbVar.xres, ", xoffset: ", mFbVar.xoffset);
        LOG("yres: ", mFbVar.yres, ", yoffset: ", mFbVar.yoffset);
        LOG("bits_per_pixel: ", mFbVar.bits_per_pixel, ", line_length: ", mFbFix.line_length, "\n");

        for (uint32_t y = 0 ; y < mFbVar.yres ; y++) {
            long location;
            for (uint32_t x = 0 ; x < mFbVar.xres ; x++) {
                location = ((x + mFbVar.xoffset) * (mFbVar.bits_per_pixel / 8)) + ((y + mFbVar.yoffset) * mFbFix.line_length);
                pixels[x + (y * mFbVar.xres)] = *(mpBuffer + (location / 4));
            }
//            LOG("y: ", y, ", location: ", location);
        }
    }
//    mpRenderer->Clear();
    mpRenderer->Copy(*mpTexture);
    mpRenderer->Present();
}

void FramebufferViewSDL::Resize(int aWidth, int aHeight)
{
    if ((aWidth == mpWindow->GetWidth()) && (aHeight == mpWindow->GetHeight())) {
        return;
    }

    LOG("Resize(", aWidth, ", ", aHeight, ")");

    mpWindow->SetSize(aWidth, aHeight);
    delete mpTexture;
    mpTexture = new Texture(*mpRenderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, aWidth, aHeight);
//    mpTexture->SetBlendMode(SDL_BLENDMODE_BLEND);
}

bool FramebufferViewSDL::PollEvents()
{
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            return false;
        } else if (event.type == SDL_KEYDOWN) {
            switch (event.key.keysym.sym) {
                case SDLK_ESCAPE:
                case SDLK_q:
                    return false;
                default:
                    break;
            }
        }
    }
    return true;
}

