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

/* This file contains all the endianness-dependant sample format conversions in c code.
 * It's not supposed to be compiled directly, but only through the files sconv_c-[bl]e.c
 * that define some macros before including this file.
 */

#include <inttypes.h>
#include <stdio.h>
#include <math.h>

#include <pulsecore/macro.h>

void pa_sconv_s16le_to_float32ne(unsigned n, const int16_t *a, float *b) {
    pa_assert(a);
    pa_assert(b);

    for (; n > 0; n--) {
        int16_t s = *(a++);
        *(b++) = ((float) INT16_FROM(s))/(float) 0x8000;
    }
}

void pa_sconv_s32le_to_float32ne(unsigned n, const int32_t *a, float *b) {
    pa_assert(a);
    pa_assert(b);

    for (; n > 0; n--) {
        int32_t s = *(a++);
        *(b++) = (float) (((double) INT32_FROM(s))/0x80000000);
    }
}

void pa_sconv_s16le_from_float32ne(unsigned n, const float *a, int16_t *b) {
    pa_assert(a);
    pa_assert(b);

    for (; n > 0; n--) {
        int16_t s;
        float v = *(a++);

        v = PA_CLAMP_UNLIKELY(v * 0x8000, - (float) 0x8000, (float) 0x7FFF);
        s = (int16_t) lrintf(v);
        *(b++) = INT16_TO(s);
    }
}

void pa_sconv_s32le_from_float32ne(unsigned n, const float *a, int32_t *b) {
    pa_assert(a);
    pa_assert(b);

    for (; n > 0; n--) {
        int32_t s;
        float v = *(a++);

        v = PA_CLAMP_UNLIKELY(v, -1.0f, 1.0f);
        s = (int32_t) lrint((double) v * (double) 0x80000000);
        *(b++) = INT32_TO(s);
    }
}

void pa_sconv_s16le_to_float32re(unsigned n, const int16_t *a, float *b) {
    pa_assert(a);
    pa_assert(b);

    for (; n > 0; n--) {
        int16_t s = *(a++);
        float k = ((float) INT16_FROM(s))/0x8000;
        k = PA_FLOAT32_SWAP(k);
        *(b++) = k;
    }
}

void pa_sconv_s16le_from_float32re(unsigned n, const float *a, int16_t *b) {
    pa_assert(a);
    pa_assert(b);

    for (; n > 0; n--) {
        int16_t s;
        float v = *(a++);
        v = PA_FLOAT32_SWAP(v);
        v = PA_CLAMP_UNLIKELY(v * 0x8000, - (float) 0x8000, (float) 0x7FFF);
        s = (int16_t) lrintf(v);
        *(b++) = INT16_TO(s);
    }
}

void pa_sconv_s32le_to_s16ne(unsigned n, const int32_t*a, int16_t *b) {
    pa_assert(a);
    pa_assert(b);

    for (; n > 0; n--) {
        *b = (int16_t) (INT32_FROM(*a) >> 16);
        a++;
        b++;
    }
}

void pa_sconv_s32le_from_s16ne(unsigned n, const int16_t *a, int32_t *b) {
    pa_assert(a);
    pa_assert(b);

    for (; n > 0; n--) {
        *b = INT32_TO(((int32_t) *a) << 16);
        a++;
        b++;
    }
}

void pa_sconv_s24le_to_s16ne(unsigned n, const uint8_t *a, int16_t *b) {
    pa_assert(a);
    pa_assert(b);

    for (; n > 0; n--) {
        *b = (int16_t) (READ24(a) >> 8);
        a += 3;
        b++;
    }
}

void pa_sconv_s24le_from_s16ne(unsigned n, const int16_t *a, uint8_t *b) {
    pa_assert(a);
    pa_assert(b);

    for (; n > 0; n--) {
        WRITE24(b, ((uint32_t) *a) << 8);
        a++;
        b += 3;
    }
}

void pa_sconv_s24le_to_float32ne(unsigned n, const uint8_t *a, float *b) {
    pa_assert(a);
    pa_assert(b);

    for (; n > 0; n--) {
        int32_t s = READ24(a) << 8;
        *b = ((float) s) / 0x80000000;
        a += 3;
        b ++;
    }
}

void pa_sconv_s24le_from_float32ne(unsigned n, const float *a, uint8_t *b) {
    pa_assert(a);
    pa_assert(b);

    for (; n > 0; n--) {
        int32_t s;
        float v = *a;
        v = PA_CLAMP_UNLIKELY(v, -1.0f, 1.0f);
        s = (int32_t) lrint((double) v * (double) 0x80000000);
        WRITE24(b, ((uint32_t) s) >> 8);
        a++;
        b+=3;
    }
}

void pa_sconv_s24_32le_to_s16ne(unsigned n, const uint32_t *a, int16_t *b) {
    pa_assert(a);
    pa_assert(b);

    for (; n > 0; n--) {
        *b = (int16_t) (((int32_t) (UINT32_FROM(*a) << 8)) >> 16);
        a++;
        b++;
    }
}

void pa_sconv_s24_32le_from_s16ne(unsigned n, const int16_t *a, uint32_t *b) {
    pa_assert(a);
    pa_assert(b);

    for (; n > 0; n--) {
        *b = UINT32_TO(((uint32_t) ((int32_t) *a << 16)) >> 8);
        a++;
        b++;
    }
}

void pa_sconv_s24_32le_to_float32ne(unsigned n, const uint32_t *a, float *b) {
    pa_assert(a);
    pa_assert(b);

    for (; n > 0; n--) {
        int32_t s = (int32_t) (UINT32_FROM(*a) << 8);
        *b = (float) s / (float) 0x80000000;
        a ++;
        b ++;
    }
}

void pa_sconv_s24_32le_from_float32ne(unsigned n, const float *a, uint32_t *b) {
    pa_assert(a);
    pa_assert(b);

    for (; n > 0; n--) {
        int32_t s;
        float v = *a;
        v = PA_CLAMP_UNLIKELY(v, -1.0f, 1.0f);
        s = (int32_t) lrint((double) v * (double) 0x80000000);
        *b = UINT32_TO(((uint32_t) s) >> 8);
        a++;
        b++;
    }
}
