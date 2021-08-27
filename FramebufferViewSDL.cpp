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

FramebufferViewSDL::FramebufferViewSDL(uint8_t *apBuffer)
{
    mpBuffer = apBuffer;

    std::memset(mpBuffer, 0, aVirtualHeight * aVirtualWidth * sizeof(uint32_t));

    mpSdl = new SDL(SDL_INIT_VIDEO);

    mpWindow = new Window("Framebuffer Emulator",
            SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
            480, 800,
            SDL_WINDOW_SKIP_TASKBAR);

    // Create accelerated video renderer with default driver
    mpRenderer = new Renderer(*mpWindow, -1, SDL_RENDERER_ACCELERATED);

    mpTexture = new Texture(*mpRenderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, 480, 800);
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
        std::memcpy(pixels, mpBuffer, mHeight * mWidth * sizeof(uint32_t));
    }
//    mpRenderer->Clear();
    mpRenderer->Copy(*mpTexture);
    mpRenderer->Present();
}

/**
 * Function to check for pending events.
 *
 * \return false if termination is requested.
 */
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

/**
 * \fn uint32_t Write(const uint32_t*, uint32_t, uint32_t)
 * \brief Write raw pixel data into the buffer.
 *
 * \param aData Pointer to incoming pixel data
 * \param aSize Number of pixels
 * \param aOffset Offset into destination buffer in pixels.
 * \return Number of pixels written to buffer.
 */
uint32_t FramebufferViewSDL::Write(const uint32_t *aData, uint32_t aSize, uint32_t aOffset, uint32_t aYOffset)
{
    if (aOffset > (mHeight * mWidth)) {
        return 0;
    }

    uint32_t written = std::min(aSize, (mHeight * mWidth) - aOffset);

    std::memcpy(&mpBuffer[aOffset], aData, written * sizeof(uint32_t));

    LOG("FramebufferViewSDL::Write(", aSize, ", ", aOffset, ") -> ", written);

    return written;
}
