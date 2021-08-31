/*
 * log.h
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

#ifndef LOG_H_
#define LOG_H_

#include <iostream>

/**
 * \fn void LOG(Args&&...)
 * \brief Template function to aid in output logging.
 *
 * \tparam Args
 * \param args
 */
template< typename ...Args >
void LOG(Args &&...args)
{
//#ifdef DEBUG
    (std::cerr << ... << args) << std::endl;
//#endif
}

#endif /* LOG_H_ */
