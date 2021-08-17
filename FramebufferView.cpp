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
#include "FramebufferView.h"

using namespace SDL2pp;

FramebufferView::FramebufferView(int aHeight, int aWidth)
{
    mHeight = aHeight;
    mWidth = aWidth;
    mpBuffer = new uint32_t[aHeight * aWidth];

    mpSdl = new SDL(SDL_INIT_VIDEO);

    mpWindow = new Window("Framebuffer Emulator",
            SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
            mWidth, mHeight,
            SDL_WINDOW_SKIP_TASKBAR | SDL_WINDOW_ALWAYS_ON_TOP | SDL_WINDOW_BORDERLESS);

    // Create accelerated video renderer with default driver
    mpRenderer = new Renderer(*mpWindow, -1, SDL_RENDERER_ACCELERATED);

    mpTexture = new Texture(*mpRenderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, mWidth, mHeight);
}

FramebufferView::~FramebufferView()
{
    delete mpTexture;
    delete mpRenderer;
    delete mpWindow;
    delete mpSdl;
    delete mpBuffer;
}

void FramebufferView::Render()
{
    {
        auto lock = mpTexture->Lock();
        uint32_t* pixels = static_cast<uint32_t*>(lock.GetPixels());
        std::memcpy(pixels, mpBuffer, mHeight * mWidth);
    }
    mpRenderer->Clear();
    mpRenderer->Copy(*mpTexture);
    mpRenderer->Present();
}

/**
 * Function to check for pending events.
 *
 * @return false if termination is requested.
 */
bool FramebufferView::PollEvents()
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
            }
        }
    }
    return true;
}
