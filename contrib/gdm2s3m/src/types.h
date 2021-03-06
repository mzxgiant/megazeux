/**
 *  Copyright (C) 2003-2004  Alistair John Strachan  (alistair@devzero.co.uk)
 *
 *  This is free software; you can redistribute it and/or modify it under
 *  the terms of the GNU General Public License as published by the Free
 *  Software Foundation; either version 2, or (at your option) any later
 *  version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef __TYPES_H
#define __TYPES_H

#ifdef __cplusplus
#define __G_BEGIN_DECLS extern "C" {
#define __G_END_DECLS   }
#else
#define __G_BEGIN_DECLS
#define __G_END_DECLS
#endif /* __cplusplus */

__G_BEGIN_DECLS

#ifndef _MSC_VER

#include <inttypes.h>

#else // _MSC_VER

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

/* MSVC can use windows types from windows.h */
typedef BYTE uint8_t;
typedef USHORT uint16_t;
typedef ULONG uint32_t;

/* MSVC doesn't understand inline */
#define inline

#endif // !_MSC_VER

__G_END_DECLS

#endif /* __TYPES_H */
