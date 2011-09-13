/***
  This file is part of PulseAudio.

  Copyright 2004-2006 Lennart Poettering

  PulseAudio is free software; you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License as published
  by the Free Software Foundation; either version 2.1 of the License,
  or (at your option) any later version.

  PulseAudio is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with PulseAudio; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
  USA.
***/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <pulsecore/endianmacros.h>

#include "sconv_c.h"

#define INT16_FROM PA_INT16_FROM_LE
#define INT16_TO PA_INT16_TO_LE
#define FLOAT32_FROM PA_FLOAT32_FROM_LE
#define FLOAT32_TO PA_FLOAT32_TO_LE

#define INT32_FROM PA_INT32_FROM_LE
#define INT32_TO PA_INT32_TO_LE
#define UINT32_FROM PA_UINT32_FROM_LE
#define UINT32_TO PA_UINT32_TO_LE

#define READ24 PA_READ24LE
#define WRITE24 PA_WRITE24LE

#include "sconv_c.c"
