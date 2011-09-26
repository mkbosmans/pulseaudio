/***
  This file is part of PulseAudio.

  Copyright 2004-2006 Lennart Poettering
  Copyright 2006 Pierre Ossman <ossman@cendio.se> for Cendio AB

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

#include <pulsecore/macro.h>
#include <pulsecore/g711.h>
#include <pulsecore/endianmacros.h>

static void volume_u8_c(uint8_t *samples, int32_t *volumes, unsigned channels, unsigned length) {
    unsigned channel;

    for (channel = 0; length; length--) {
        int32_t t, hi, lo;

        hi = volumes[channel] >> 16;
        lo = volumes[channel] & 0xFFFF;

        t = (int32_t) *samples - 0x80;
        t = ((t * lo) >> 16) + (t * hi);
        t = PA_CLAMP_UNLIKELY(t, -0x80, 0x7F);
        *samples++ = (uint8_t) (t + 0x80);

        if (++channel >= channels)
            channel = 0;
    }
}

static void volume_alaw_c(uint8_t *samples, int32_t *volumes, unsigned channels, unsigned length) {
    unsigned channel;

    for (channel = 0; length; length--) {
        int32_t t, hi, lo;

        hi = volumes[channel] >> 16;
        lo = volumes[channel] & 0xFFFF;

        t = (int32_t) sox_alaw2linear16(*samples);
        t = ((t * lo) >> 16) + (t * hi);
        t = PA_CLAMP_UNLIKELY(t, -0x8000, 0x7FFF);
        *samples++ = (uint8_t) sox_13linear2alaw((int16_t) t >> 3);

        if (++channel >= channels)
            channel = 0;
    }
}

static void volume_ulaw_c(uint8_t *samples, int32_t *volumes, unsigned channels, unsigned length) {
    unsigned channel;

    for (channel = 0; length; length--) {
        int32_t t, hi, lo;

        hi = volumes[channel] >> 16;
        lo = volumes[channel] & 0xFFFF;

        t = (int32_t) sox_ulaw2linear16(*samples);
        t = ((t * lo) >> 16) + (t * hi);
        t = PA_CLAMP_UNLIKELY(t, -0x8000, 0x7FFF);
        *samples++ = (uint8_t) sox_14linear2ulaw((int16_t) t >> 2);

        if (++channel >= channels)
            channel = 0;
    }
}

static void volume_s16ne_c(int16_t *samples, int32_t *volumes, unsigned channels, unsigned length) {
    unsigned channel;

    length /= sizeof(int16_t);

    for (channel = 0; length; length--) {
        int32_t t, hi, lo;

        /* Multiplying the 32bit volume factor with the 16bit
         * sample might result in an 48bit value. We want to
         * do without 64 bit integers and hence do the
         * multiplication independently for the HI and LO part
         * of the volume. */

        hi = volumes[channel] >> 16;
        lo = volumes[channel] & 0xFFFF;

        t = (int32_t)(*samples);
        t = ((t * lo) >> 16) + (t * hi);
        t = PA_CLAMP_UNLIKELY(t, -0x8000, 0x7FFF);
        *samples++ = (int16_t) t;

        if (++channel >= channels)
            channel = 0;
    }
}

static void volume_s16re_c(int16_t *samples, int32_t *volumes, unsigned channels, unsigned length) {
    unsigned channel;

    length /= sizeof(int16_t);

    for (channel = 0; length; length--) {
        int32_t t, hi, lo;

        hi = volumes[channel] >> 16;
        lo = volumes[channel] & 0xFFFF;

        t = (int32_t) PA_INT16_SWAP(*samples);
        t = ((t * lo) >> 16) + (t * hi);
        t = PA_CLAMP_UNLIKELY(t, -0x8000, 0x7FFF);
        *samples++ = PA_INT16_SWAP((int16_t) t);

        if (++channel >= channels)
            channel = 0;
    }
}

static void volume_float32ne_c(float *samples, float *volumes, unsigned channels, unsigned length) {
    unsigned channel;

    length /= sizeof(float);

    for (channel = 0; length; length--) {
        *samples++ *= volumes[channel];

        if (++channel >= channels)
            channel = 0;
    }
}

static void volume_float32re_c(float *samples, float *volumes, unsigned channels, unsigned length) {
    unsigned channel;

    length /= sizeof(float);

    for (channel = 0; length; length--) {
        float t;

        t = PA_FLOAT32_SWAP(*samples);
        t *= volumes[channel];
        *samples++ = PA_FLOAT32_SWAP(t);

        if (++channel >= channels)
            channel = 0;
    }
}

static void volume_s32ne_c(int32_t *samples, int32_t *volumes, unsigned channels, unsigned length) {
    unsigned channel;

    length /= sizeof(int32_t);

    for (channel = 0; length; length--) {
        int64_t t;

        t = (int64_t)(*samples);
        t = (t * volumes[channel]) >> 16;
        t = PA_CLAMP_UNLIKELY(t, -0x80000000LL, 0x7FFFFFFFLL);
        *samples++ = (int32_t) t;

        if (++channel >= channels)
            channel = 0;
    }
}

static void volume_s32re_c(int32_t *samples, int32_t *volumes, unsigned channels, unsigned length) {
    unsigned channel;

    length /= sizeof(int32_t);

    for (channel = 0; length; length--) {
        int64_t t;

        t = (int64_t) PA_INT32_SWAP(*samples);
        t = (t * volumes[channel]) >> 16;
        t = PA_CLAMP_UNLIKELY(t, -0x80000000LL, 0x7FFFFFFFLL);
        *samples++ = PA_INT32_SWAP((int32_t) t);

        if (++channel >= channels)
            channel = 0;
    }
}

static void volume_s24ne_c(uint8_t *samples, int32_t *volumes, unsigned channels, unsigned length) {
    unsigned channel;
    uint8_t *e;

    e = samples + length;

    for (channel = 0; samples < e; samples += 3) {
        int64_t t;

        t = (int64_t)((int32_t) (PA_READ24NE(samples) << 8));
        t = (t * volumes[channel]) >> 16;
        t = PA_CLAMP_UNLIKELY(t, -0x80000000LL, 0x7FFFFFFFLL);
        PA_WRITE24NE(samples, ((uint32_t) (int32_t) t) >> 8);

        if (++channel >= channels)
            channel = 0;
    }
}

static void volume_s24re_c(uint8_t *samples, int32_t *volumes, unsigned channels, unsigned length) {
    unsigned channel;
    uint8_t *e;

    e = samples + length;

    for (channel = 0; samples < e; samples += 3) {
        int64_t t;

        t = (int64_t)((int32_t) (PA_READ24RE(samples) << 8));
        t = (t * volumes[channel]) >> 16;
        t = PA_CLAMP_UNLIKELY(t, -0x80000000LL, 0x7FFFFFFFLL);
        PA_WRITE24RE(samples, ((uint32_t) (int32_t) t) >> 8);

        if (++channel >= channels)
            channel = 0;
    }
}

static void volume_s24_32ne_c(uint32_t *samples, int32_t *volumes, unsigned channels, unsigned length) {
    unsigned channel;

    length /= sizeof(uint32_t);

    for (channel = 0; length; length--) {
        int64_t t;

        t = (int64_t) ((int32_t) (*samples << 8));
        t = (t * volumes[channel]) >> 16;
        t = PA_CLAMP_UNLIKELY(t, -0x80000000LL, 0x7FFFFFFFLL);
        *samples++ = ((uint32_t) ((int32_t) t)) >> 8;

        if (++channel >= channels)
            channel = 0;
    }
}

static void volume_s24_32re_c(uint32_t *samples, int32_t *volumes, unsigned channels, unsigned length) {
    unsigned channel;

    length /= sizeof(uint32_t);

    for (channel = 0; length; length--) {
        int64_t t;

        t = (int64_t) ((int32_t) (PA_UINT32_SWAP(*samples) << 8));
        t = (t * volumes[channel]) >> 16;
        t = PA_CLAMP_UNLIKELY(t, -0x80000000LL, 0x7FFFFFFFLL);
        *samples++ = PA_UINT32_SWAP(((uint32_t) ((int32_t) t)) >> 8);

        if (++channel >= channels)
            channel = 0;
    }
}
