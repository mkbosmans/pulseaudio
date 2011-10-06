/***
  This file is part of PulseAudio.

  Copyright 2004-2006 Lennart Poettering
  Copyright 2006 Pierre Ossman <ossman@cendio.se> for Cendio AB
  Copyright 2011 Maarten Bosmans

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

/* This file is not compiled directly, but included in mix.c */

static void mix_s16ne_c(int32_t stream_volume[][PA_CHANNELS_MAX], unsigned nstreams, unsigned nchannels, const void *src[], void *dest, size_t length) {
    unsigned i, channel = 0;
    void *end;

    end = (uint8_t*) dest + length;
    while (dest < end) {
        int32_t sum = 0;

        for (i = 0; i < nstreams; i++) {
            int32_t v, lo, hi, cv = stream_volume[i][channel];

            if (PA_UNLIKELY(cv <= 0))
                continue;

            /* Multiplying the 32bit volume factor with the 16bit sample
             * might result in an 48bit value. We want to do without
             * 64bit integers and hence do the multiplication
             * independently for the HI and LO part of the volume.
             */
            hi = cv >> 16;
            lo = cv & 0xFFFF;

            v = *((int16_t*) src[i]);
            v = ((v * lo) >> 16) + (v * hi);
            sum += v;

            src[i] = (uint8_t*) src[i] + sizeof(int16_t);
        }

        sum = PA_CLAMP_UNLIKELY(sum, -0x8000, 0x7FFF);
        *((int16_t*) dest) = (int16_t) sum;

        dest = (uint8_t*) dest + sizeof(int16_t);

        if (++channel >= nchannels)
            channel = 0;
    }
}

static void mix_s16re_c(int32_t stream_volume[][PA_CHANNELS_MAX], unsigned nstreams, unsigned nchannels, const void *src[], void *dest, size_t length) {
    unsigned i, channel = 0;
    void *end;

    end = (uint8_t*) dest + length;
    while (dest < end) {
        int32_t sum = 0;

        for (i = 0; i < nstreams; i++) {
            int32_t v, lo, hi, cv = stream_volume[i][channel];

            if (PA_UNLIKELY(cv <= 0))
                continue;

            hi = cv >> 16;
            lo = cv & 0xFFFF;

            v = PA_INT16_SWAP(*((int16_t*) src[i]));
            v = ((v * lo) >> 16) + (v * hi);
            sum += v;

            src[i] = (uint8_t*) src[i] + sizeof(int16_t);
        }

        sum = PA_CLAMP_UNLIKELY(sum, -0x8000, 0x7FFF);
        *((int16_t*) dest) = PA_INT16_SWAP((int16_t) sum);

        dest = (uint8_t*) dest + sizeof(int16_t);

        if (++channel >= nchannels)
            channel = 0;
    }
}

static void mix_s32ne_c(int32_t stream_volume[][PA_CHANNELS_MAX], unsigned nstreams, unsigned nchannels, const void *src[], void *dest, size_t length) {
    unsigned i, channel = 0;
    void *end;

    end = (uint8_t*) dest + length;
    while (dest < end) {
        int64_t sum = 0;

        for (i = 0; i < nstreams; i++) {
            int32_t cv = stream_volume[i][channel];
            int64_t v;

            if (PA_UNLIKELY(cv <= 0))
                continue;

            v = *((int32_t*) src[i]);
            v = (v * cv) >> 16;
            sum += v;

            src[i] = (uint8_t*) src[i] + sizeof(int32_t);
        }

        sum = PA_CLAMP_UNLIKELY(sum, -0x80000000LL, 0x7FFFFFFFLL);
        *((int32_t*) dest) = (int32_t) sum;

        dest = (uint8_t*) dest + sizeof(int32_t);

        if (++channel >= nchannels)
            channel = 0;
    }
}

static void mix_s32re_c(int32_t stream_volume[][PA_CHANNELS_MAX], unsigned nstreams, unsigned nchannels, const void *src[], void *dest, size_t length) {
    unsigned i, channel = 0;
    void *end;

    end = (uint8_t*) dest + length;
    while (dest < end) {
        int64_t sum = 0;

        for (i = 0; i < nstreams; i++) {
            int32_t cv = stream_volume[i][channel];
            int64_t v;

            if (PA_UNLIKELY(cv <= 0))
                continue;

            v = PA_INT32_SWAP(*((int32_t*) src[i]));
            v = (v * cv) >> 16;
            sum += v;

            src[i] = (uint8_t*) src[i] + sizeof(int32_t);
        }

        sum = PA_CLAMP_UNLIKELY(sum, -0x80000000LL, 0x7FFFFFFFLL);
        *((int32_t*) dest) = PA_INT32_SWAP((int32_t) sum);

        dest = (uint8_t*) dest + sizeof(int32_t);

        if (++channel >= nchannels)
            channel = 0;
    }
}

static void mix_s24ne_c(int32_t stream_volume[][PA_CHANNELS_MAX], unsigned nstreams, unsigned nchannels, const void *src[], void *dest, size_t length) {
    unsigned i, channel = 0;
    void *end;

    end = (uint8_t*) dest + length;
    while (dest < end) {
        int64_t sum = 0;

        for (i = 0; i < nstreams; i++) {
            int32_t cv = stream_volume[i][channel];
            int64_t v;

            if (PA_UNLIKELY(cv <= 0))
                continue;

            v = (int32_t) (PA_READ24NE(src[i]) << 8);
            v = (v * cv) >> 16;
            sum += v;

            src[i] = (uint8_t*) src[i] + 3;
        }

        sum = PA_CLAMP_UNLIKELY(sum, -0x80000000LL, 0x7FFFFFFFLL);
        PA_WRITE24NE(dest, ((uint32_t) sum) >> 8);

        dest = (uint8_t*) dest + 3;

        if (++channel >= nchannels)
            channel = 0;
    }
}

static void mix_s24re_c(int32_t stream_volume[][PA_CHANNELS_MAX], unsigned nstreams, unsigned nchannels, const void *src[], void *dest, size_t length) {
    unsigned i, channel = 0;
    void *end;

    end = (uint8_t*) dest + length;
    while (dest < end) {
        int64_t sum = 0;

        for (i = 0; i < nstreams; i++) {
            int32_t cv = stream_volume[i][channel];
            int64_t v;

            if (PA_UNLIKELY(cv <= 0))
                continue;

            v = (int32_t) (PA_READ24RE(src[i]) << 8);
            v = (v * cv) >> 16;
            sum += v;

            src[i] = (uint8_t*) src[i] + 3;
        }

        sum = PA_CLAMP_UNLIKELY(sum, -0x80000000LL, 0x7FFFFFFFLL);
        PA_WRITE24RE(dest, ((uint32_t) sum) >> 8);

        dest = (uint8_t*) dest + 3;

        if (++channel >= nchannels)
            channel = 0;
    }
}

static void mix_s24_32ne_c(int32_t stream_volume[][PA_CHANNELS_MAX], unsigned nstreams, unsigned nchannels, const void *src[], void *dest, size_t length) {
    unsigned i, channel = 0;
    void *end;

    end = (uint8_t*) dest + length;
    while (dest < end) {
        int64_t sum = 0;

        for (i = 0; i < nstreams; i++) {
            int32_t cv = stream_volume[i][channel];
            int64_t v;

            if (PA_UNLIKELY(cv <= 0))
                continue;

            v = (int32_t) (*((uint32_t*)src[i]) << 8);
            v = (v * cv) >> 16;
            sum += v;

            src[i] = (uint8_t*) src[i] + sizeof(int32_t);
        }

        sum = PA_CLAMP_UNLIKELY(sum, -0x80000000LL, 0x7FFFFFFFLL);
        *((uint32_t*) dest) = ((uint32_t) (int32_t) sum) >> 8;

        dest = (uint8_t*) dest + sizeof(uint32_t);

        if (++channel >= nchannels)
            channel = 0;
    }
}

static void mix_s24_32re_c(int32_t stream_volume[][PA_CHANNELS_MAX], unsigned nstreams, unsigned nchannels, const void *src[], void *dest, size_t length) {
    unsigned i, channel = 0;
    void *end;

    end = (uint8_t*) dest + length;
    while (dest < end) {
        int64_t sum = 0;

        for (i = 0; i < nstreams; i++) {
            int32_t cv = stream_volume[i][channel];
            int64_t v;

            if (PA_UNLIKELY(cv <= 0))
                continue;

            v = (int32_t) (PA_UINT32_SWAP(*((uint32_t*) src[i])) << 8);
            v = (v * cv) >> 16;
            sum += v;

            src[i] = (uint8_t*) src[i] + 3;
        }

        sum = PA_CLAMP_UNLIKELY(sum, -0x80000000LL, 0x7FFFFFFFLL);
        *((uint32_t*) dest) = PA_INT32_SWAP(((uint32_t) (int32_t) sum) >> 8);

        dest = (uint8_t*) dest + sizeof(uint32_t);

        if (++channel >= nchannels)
            channel = 0;
    }
}

static void mix_u8_c(int32_t stream_volume[][PA_CHANNELS_MAX], unsigned nstreams, unsigned nchannels, const void *src[], void *dest, size_t length) {
    unsigned i, channel = 0;
    void *end;

    end = (uint8_t*) dest + length;
    while (dest < end) {
        int32_t sum = 0;

        for (i = 0; i < nstreams; i++) {
            int32_t v, cv = stream_volume[i][channel];

            if (PA_UNLIKELY(cv <= 0))
                continue;

            v = (int32_t) *((uint8_t*) src[i]) - 0x80;
            v = (v * cv) >> 16;
            sum += v;

            src[i] = (uint8_t*) src[i] + 1;
        }

        sum = PA_CLAMP_UNLIKELY(sum, -0x80, 0x7F);
        *((uint8_t*) dest) = (uint8_t) (sum + 0x80);

        dest = (uint8_t*) dest + 1;

        if (++channel >= nchannels)
            channel = 0;
    }

}

static void mix_ulaw_c(int32_t stream_volume[][PA_CHANNELS_MAX], unsigned nstreams, unsigned nchannels, const void *src[], void *dest, size_t length) {
    unsigned i, channel = 0;
    void *end;

    end = (uint8_t*) dest + length;
    while (dest < end) {
        int32_t sum = 0;

        for (i = 0; i < nstreams; i++) {
            int32_t v, hi, lo, cv = stream_volume[i][channel];

            if (PA_UNLIKELY(cv <= 0))
                continue;

            hi = cv >> 16;
            lo = cv & 0xFFFF;

            v = (int32_t) sox_ulaw2linear16(*((uint8_t*) src[i]));
            v = ((v * lo) >> 16) + (v * hi);
            sum += v;

            src[i] = (uint8_t*) src[i] + 1;
        }

        sum = PA_CLAMP_UNLIKELY(sum, -0x8000, 0x7FFF);
        *((uint8_t*) dest) = (uint8_t) sox_14linear2ulaw((int16_t) sum >> 2);

        dest = (uint8_t*) dest + 1;

        if (++channel >= nchannels)
            channel = 0;
    }
}

static void mix_alaw_c(int32_t stream_volume[][PA_CHANNELS_MAX], unsigned nstreams, unsigned nchannels, const void *src[], void *dest, size_t length) {
    unsigned i, channel = 0;
    void *end;

    end = (uint8_t*) dest + length;
    while (dest < end) {
        int32_t sum = 0;

        for (i = 0; i < nstreams; i++) {
            int32_t v, hi, lo, cv = stream_volume[i][channel];

            if (PA_UNLIKELY(cv <= 0))
                continue;

            hi = cv >> 16;
            lo = cv & 0xFFFF;

            v = (int32_t) sox_alaw2linear16(*((uint8_t*) src[i]));
            v = ((v * lo) >> 16) + (v * hi);
            sum += v;

            src[i] = (uint8_t*) src[i] + 1;
        }

        sum = PA_CLAMP_UNLIKELY(sum, -0x8000, 0x7FFF);
        *((uint8_t*) dest) = (uint8_t) sox_13linear2alaw((int16_t) sum >> 3);

        dest = (uint8_t*) dest + 1;

        if (++channel >= nchannels)
            channel = 0;
    }
}

static void mix_float32ne_c(float stream_volume[][PA_CHANNELS_MAX], unsigned nstreams, unsigned nchannels, const void *src[], void *dest, size_t length) {
    unsigned i, channel = 0;
    void *end;

    end = (uint8_t*) dest + length;
    while (dest < end) {
        float sum = 0;

        for (i = 0; i < nstreams; i++) {
            float v, cv = stream_volume[i][channel];

            if (PA_UNLIKELY(cv <= 0))
                continue;

            v = *((float*) src[i]);
            v *= cv;
            sum += v;

            src[i] = (uint8_t*) src[i] + sizeof(float);
        }

        *((float*) dest) = sum;

        dest = (uint8_t*) dest + sizeof(float);

        if (++channel >= nchannels)
            channel = 0;
    }
}

static void mix_float32re_c(float stream_volume[][PA_CHANNELS_MAX], unsigned nstreams, unsigned nchannels, const void *src[], void *dest, size_t length) {
    unsigned i, channel = 0;
    void *end;

    end = (uint8_t*) dest + length;
    while (dest < end) {
        float sum = 0;

        for (i = 0; i < nstreams; i++) {
            float v, cv = stream_volume[i][channel];

            if (PA_UNLIKELY(cv <= 0))
                continue;

            v = PA_FLOAT32_SWAP(*(float*) src[i]);
            v *= cv;
            sum += v;

            src[i] = (uint8_t*) src[i] + sizeof(float);
        }

        *((float*) dest) = PA_FLOAT32_SWAP(sum);

        dest = (uint8_t*) dest + sizeof(float);

        if (++channel >= nchannels)
            channel = 0;
    }
}
