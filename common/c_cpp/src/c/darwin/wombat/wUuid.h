/* $Id$
 *
 * OpenMAMA: The open middleware agnostic messaging API
 * Copyright (C) 2011 NYSE Technologies, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301 USA
 */


#ifndef WUUID_H__
#define WUUID_H__

#include <uuid/uuid.h>

typedef uuid_t wUuid;

#define wUuid_generate              uuid_generate
#define wUuid_generate_time         uuid_generate_time
int wUuid_generate_time_safe (wUuid myUuid);
#define wUuid_generate_random       uuid_generate_random

#define wUuid_unparse uuid_unparse

#define wUuid_clear(UUID) uuid_clear(UUID)

#endif /* WUUID_H__ */
