/*
 * Epoll.cpp
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

#include "Epoll.h"
#include <unistd.h>
#include <system_error>

Epoll::Epoll()
{
    mFd = epoll_create1(0);
    if(mFd == -1) {
        throw std::system_error(errno, std::generic_category(), "Failed to create epoll file descriptor");
    }
}

Epoll::~Epoll()
{
    if(close(mFd)) {
//     Dont throw from destructor....
//        throw std::system_error(errno, std::generic_category(), "Failed to close epoll file descriptor");
    }
}

void Epoll::Add(int aFd, enum EPOLL_EVENTS aEpollEvents)
{
    struct epoll_event event;
    event.events = aEpollEvents;
    event.data.fd = aFd;

    if(epoll_ctl(mFd, EPOLL_CTL_ADD, aFd, &event) == -1) {
        throw std::system_error(errno, std::generic_category(), "Failed to add file descriptor to epoll");
    }
}

void Epoll::Del(int aFd)
{
    if(epoll_ctl(mFd, EPOLL_CTL_DEL, aFd, nullptr) == -1) {
        throw std::system_error(errno, std::generic_category(), "Failed to remove file descriptor from epoll");
    }
}

int Epoll::Wait(struct epoll_event *aEvents, int aMaxEvents, int aTimeoutMilliSeconds)
{
    int event_count = epoll_wait(mFd, aEvents, aMaxEvents, aTimeoutMilliSeconds);
    if (event_count == -1) {
        throw std::system_error(errno, std::generic_category(), "Failed waiting for epoll file descriptor");
    }
    return event_count;
}
