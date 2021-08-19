/*
 * Epoll.h
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

#ifndef EPOLL_H_
#define EPOLL_H_

#include <sys/epoll.h>

class Epoll
{
public:
    Epoll();
    virtual ~Epoll();

    void Add(int aFd, enum EPOLL_EVENTS aEpollEvents);
    void Del(int aFd);
    int Wait(struct epoll_event *aEvents, int aMaxEvents, int aTimeoutMilliSeconds);

protected:
    int mFd;
};

#endif /* EPOLL_H_ */
