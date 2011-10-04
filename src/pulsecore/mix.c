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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <math.h>

#include <pulse/gccmacro.h>

#include <pulsecore/endianmacros.h>
#include <pulsecore/g711.h>
#include <pulsecore/log.h>
#include <pulsecore/macro.h>
#include <pulsecore/sample-util.h>

#include "mix.h"

static void calc_linear_integer_stream_volumes(pa_mix_info streams[], unsigned nstreams, const pa_cvolume *volume, const pa_sample_spec *spec) {
    unsigned k, channel, nchannels;
    double linear[PA_CHANNELS_MAX];

    pa_assert(streams);
    pa_assert(spec);
    pa_assert(volume);
    pa_assert(volume->channels == spec->channels);

    nchannels = volume->channels;

    for (channel = 0; channel < nchannels; channel++)
        linear[channel] = pa_sw_volume_to_linear(volume->values[channel]);

    for (k = 0; k < nstreams; k++) {
        for (channel = 0; channel < nchannels; channel++) {
            pa_mix_info *m = streams + k;
            m->linear[channel].i = (int32_t) lrint(pa_sw_volume_to_linear(m->volume.values[channel]) * linear[channel] * 0x10000);
        }
    }
}

static void calc_linear_float_stream_volumes(pa_mix_info streams[], unsigned nstreams, const pa_cvolume *volume, const pa_sample_spec *spec) {
    unsigned k, channel, nchannels;
    double linear[PA_CHANNELS_MAX];

    pa_assert(streams);
    pa_assert(spec);
    pa_assert(volume);
    pa_assert(volume->channels == spec->channels);

    nchannels = volume->channels;

    for (channel = 0; channel < nchannels; channel++)
        linear[channel] = pa_sw_volume_to_linear(volume->values[channel]);

    for (k = 0; k < nstreams; k++) {
        for (channel = 0; channel < nchannels; channel++) {
            pa_mix_info *m = streams + k;
            m->linear[channel].f = (float) (pa_sw_volume_to_linear(m->volume.values[channel]) * linear[channel]);
        }
    }
}

size_t pa_mix(
        pa_mix_info streams[],
        unsigned nstreams,
        void *data,
        size_t length,
        const pa_sample_spec *spec,
        const pa_cvolume *volume,
        pa_bool_t mute) {

    pa_cvolume full_volume;
    unsigned k;
    unsigned z;
    void *end;

    pa_assert(streams);
    pa_assert(data);
    pa_assert(length);
    pa_assert(spec);

    if (!volume)
        volume = pa_cvolume_reset(&full_volume, spec->channels);

    if (mute || pa_cvolume_is_muted(volume) || nstreams <= 0) {
        pa_silence_memory(data, length, spec);
        return length;
    }

    for (k = 0; k < nstreams; k++)
        streams[k].ptr = (uint8_t*) pa_memblock_acquire(streams[k].chunk.memblock) + streams[k].chunk.index;

    for (z = 0; z < nstreams; z++)
        if (length > streams[z].chunk.length)
            length = streams[z].chunk.length;

    end = (uint8_t*) data + length;

    switch (spec->format) {

        case PA_SAMPLE_S16NE:{
            unsigned channel = 0;

            calc_linear_integer_stream_volumes(streams, nstreams, volume, spec);

            while (data < end) {
                int32_t sum = 0;
                unsigned i;

                for (i = 0; i < nstreams; i++) {
                    pa_mix_info *m = streams + i;
                    int32_t v, lo, hi, cv = m->linear[channel].i;

                    if (PA_UNLIKELY(cv <= 0))
                        continue;

                    /* Multiplying the 32bit volume factor with the
                     * 16bit sample might result in an 48bit value. We
                     * want to do without 64 bit integers and hence do
                     * the multiplication independently for the HI and
                     * LO part of the volume. */

                    hi = cv >> 16;
                    lo = cv & 0xFFFF;

                    v = *((int16_t*) m->ptr);
                    v = ((v * lo) >> 16) + (v * hi);
                    sum += v;

                    m->ptr = (uint8_t*) m->ptr + sizeof(int16_t);
                }

                sum = PA_CLAMP_UNLIKELY(sum, -0x8000, 0x7FFF);
                *((int16_t*) data) = (int16_t) sum;

                data = (uint8_t*) data + sizeof(int16_t);

                if (PA_UNLIKELY(++channel >= spec->channels))
                    channel = 0;
            }

            break;
        }

        case PA_SAMPLE_S16RE:{
            unsigned channel = 0;

            calc_linear_integer_stream_volumes(streams, nstreams, volume, spec);

            while (data < end) {
                int32_t sum = 0;
                unsigned i;

                for (i = 0; i < nstreams; i++) {
                    pa_mix_info *m = streams + i;
                    int32_t v, lo, hi, cv = m->linear[channel].i;

                    if (PA_UNLIKELY(cv <= 0))
                        continue;

                    hi = cv >> 16;
                    lo = cv & 0xFFFF;

                    v = PA_INT16_SWAP(*((int16_t*) m->ptr));
                    v = ((v * lo) >> 16) + (v * hi);
                    sum += v;

                    m->ptr = (uint8_t*) m->ptr + sizeof(int16_t);
                }

                sum = PA_CLAMP_UNLIKELY(sum, -0x8000, 0x7FFF);
                *((int16_t*) data) = PA_INT16_SWAP((int16_t) sum);

                data = (uint8_t*) data + sizeof(int16_t);

                if (PA_UNLIKELY(++channel >= spec->channels))
                    channel = 0;
            }

            break;
        }

        case PA_SAMPLE_S32NE:{
            unsigned channel = 0;

            calc_linear_integer_stream_volumes(streams, nstreams, volume, spec);

            while (data < end) {
                int64_t sum = 0;
                unsigned i;

                for (i = 0; i < nstreams; i++) {
                    pa_mix_info *m = streams + i;
                    int32_t cv = m->linear[channel].i;
                    int64_t v;

                    if (PA_UNLIKELY(cv <= 0))
                        continue;

                    v = *((int32_t*) m->ptr);
                    v = (v * cv) >> 16;
                    sum += v;

                    m->ptr = (uint8_t*) m->ptr + sizeof(int32_t);
                }

                sum = PA_CLAMP_UNLIKELY(sum, -0x80000000LL, 0x7FFFFFFFLL);
                *((int32_t*) data) = (int32_t) sum;

                data = (uint8_t*) data + sizeof(int32_t);

                if (PA_UNLIKELY(++channel >= spec->channels))
                    channel = 0;
            }

            break;
        }

        case PA_SAMPLE_S32RE:{
            unsigned channel = 0;

            calc_linear_integer_stream_volumes(streams, nstreams, volume, spec);

            while (data < end) {
                int64_t sum = 0;
                unsigned i;

                for (i = 0; i < nstreams; i++) {
                    pa_mix_info *m = streams + i;
                    int32_t cv = m->linear[channel].i;
                    int64_t v;

                    if (PA_UNLIKELY(cv <= 0))
                        continue;

                    v = PA_INT32_SWAP(*((int32_t*) m->ptr));
                    v = (v * cv) >> 16;
                    sum += v;

                    m->ptr = (uint8_t*) m->ptr + sizeof(int32_t);
                }

                sum = PA_CLAMP_UNLIKELY(sum, -0x80000000LL, 0x7FFFFFFFLL);
                *((int32_t*) data) = PA_INT32_SWAP((int32_t) sum);

                data = (uint8_t*) data + sizeof(int32_t);

                if (PA_UNLIKELY(++channel >= spec->channels))
                    channel = 0;
            }

            break;
        }

        case PA_SAMPLE_S24NE: {
            unsigned channel = 0;

            calc_linear_integer_stream_volumes(streams, nstreams, volume, spec);

            while (data < end) {
                int64_t sum = 0;
                unsigned i;

                for (i = 0; i < nstreams; i++) {
                    pa_mix_info *m = streams + i;
                    int32_t cv = m->linear[channel].i;
                    int64_t v;

                    if (PA_UNLIKELY(cv <= 0))
                        continue;

                    v = (int32_t) (PA_READ24NE(m->ptr) << 8);
                    v = (v * cv) >> 16;
                    sum += v;

                    m->ptr = (uint8_t*) m->ptr + 3;
                }

                sum = PA_CLAMP_UNLIKELY(sum, -0x80000000LL, 0x7FFFFFFFLL);
                PA_WRITE24NE(data, ((uint32_t) sum) >> 8);

                data = (uint8_t*) data + 3;

                if (PA_UNLIKELY(++channel >= spec->channels))
                    channel = 0;
            }

            break;
        }

        case PA_SAMPLE_S24RE: {
            unsigned channel = 0;

            calc_linear_integer_stream_volumes(streams, nstreams, volume, spec);

            while (data < end) {
                int64_t sum = 0;
                unsigned i;

                for (i = 0; i < nstreams; i++) {
                    pa_mix_info *m = streams + i;
                    int32_t cv = m->linear[channel].i;
                    int64_t v;

                    if (PA_UNLIKELY(cv <= 0))
                        continue;

                    v = (int32_t) (PA_READ24RE(m->ptr) << 8);
                    v = (v * cv) >> 16;
                    sum += v;

                    m->ptr = (uint8_t*) m->ptr + 3;
                }

                sum = PA_CLAMP_UNLIKELY(sum, -0x80000000LL, 0x7FFFFFFFLL);
                PA_WRITE24RE(data, ((uint32_t) sum) >> 8);

                data = (uint8_t*) data + 3;

                if (PA_UNLIKELY(++channel >= spec->channels))
                    channel = 0;
            }

            break;
        }

        case PA_SAMPLE_S24_32NE: {
            unsigned channel = 0;

            calc_linear_integer_stream_volumes(streams, nstreams, volume, spec);

            while (data < end) {
                int64_t sum = 0;
                unsigned i;

                for (i = 0; i < nstreams; i++) {
                    pa_mix_info *m = streams + i;
                    int32_t cv = m->linear[channel].i;
                    int64_t v;

                    if (PA_UNLIKELY(cv <= 0))
                        continue;

                    v = (int32_t) (*((uint32_t*)m->ptr) << 8);
                    v = (v * cv) >> 16;
                    sum += v;

                    m->ptr = (uint8_t*) m->ptr + sizeof(int32_t);
                }

                sum = PA_CLAMP_UNLIKELY(sum, -0x80000000LL, 0x7FFFFFFFLL);
                *((uint32_t*) data) = ((uint32_t) (int32_t) sum) >> 8;

                data = (uint8_t*) data + sizeof(uint32_t);

                if (PA_UNLIKELY(++channel >= spec->channels))
                    channel = 0;
            }

            break;
        }

        case PA_SAMPLE_S24_32RE: {
            unsigned channel = 0;

            calc_linear_integer_stream_volumes(streams, nstreams, volume, spec);

            while (data < end) {
                int64_t sum = 0;
                unsigned i;

                for (i = 0; i < nstreams; i++) {
                    pa_mix_info *m = streams + i;
                    int32_t cv = m->linear[channel].i;
                    int64_t v;

                    if (PA_UNLIKELY(cv <= 0))
                        continue;

                    v = (int32_t) (PA_UINT32_SWAP(*((uint32_t*) m->ptr)) << 8);
                    v = (v * cv) >> 16;
                    sum += v;

                    m->ptr = (uint8_t*) m->ptr + 3;
                }

                sum = PA_CLAMP_UNLIKELY(sum, -0x80000000LL, 0x7FFFFFFFLL);
                *((uint32_t*) data) = PA_INT32_SWAP(((uint32_t) (int32_t) sum) >> 8);

                data = (uint8_t*) data + sizeof(uint32_t);

                if (PA_UNLIKELY(++channel >= spec->channels))
                    channel = 0;
            }

            break;
        }

        case PA_SAMPLE_U8: {
            unsigned channel = 0;

            calc_linear_integer_stream_volumes(streams, nstreams, volume, spec);

            while (data < end) {
                int32_t sum = 0;
                unsigned i;

                for (i = 0; i < nstreams; i++) {
                    pa_mix_info *m = streams + i;
                    int32_t v, cv = m->linear[channel].i;

                    if (PA_UNLIKELY(cv <= 0))
                        continue;

                    v = (int32_t) *((uint8_t*) m->ptr) - 0x80;
                    v = (v * cv) >> 16;
                    sum += v;

                    m->ptr = (uint8_t*) m->ptr + 1;
                }

                sum = PA_CLAMP_UNLIKELY(sum, -0x80, 0x7F);
                *((uint8_t*) data) = (uint8_t) (sum + 0x80);

                data = (uint8_t*) data + 1;

                if (PA_UNLIKELY(++channel >= spec->channels))
                    channel = 0;
            }

            break;
        }

        case PA_SAMPLE_ULAW: {
            unsigned channel = 0;

            calc_linear_integer_stream_volumes(streams, nstreams, volume, spec);

            while (data < end) {
                int32_t sum = 0;
                unsigned i;

                for (i = 0; i < nstreams; i++) {
                    pa_mix_info *m = streams + i;
                    int32_t v, hi, lo, cv = m->linear[channel].i;

                    if (PA_UNLIKELY(cv <= 0))
                        continue;

                    hi = cv >> 16;
                    lo = cv & 0xFFFF;

                    v = (int32_t) sox_ulaw2linear16(*((uint8_t*) m->ptr));
                    v = ((v * lo) >> 16) + (v * hi);
                    sum += v;

                    m->ptr = (uint8_t*) m->ptr + 1;
                }

                sum = PA_CLAMP_UNLIKELY(sum, -0x8000, 0x7FFF);
                *((uint8_t*) data) = (uint8_t) sox_14linear2ulaw((int16_t) sum >> 2);

                data = (uint8_t*) data + 1;

                if (PA_UNLIKELY(++channel >= spec->channels))
                    channel = 0;
            }

            break;
        }

        case PA_SAMPLE_ALAW: {
            unsigned channel = 0;

            calc_linear_integer_stream_volumes(streams, nstreams, volume, spec);

            while (data < end) {
                int32_t sum = 0;
                unsigned i;

                for (i = 0; i < nstreams; i++) {
                    pa_mix_info *m = streams + i;
                    int32_t v, hi, lo, cv = m->linear[channel].i;

                    if (PA_UNLIKELY(cv <= 0))
                        continue;

                    hi = cv >> 16;
                    lo = cv & 0xFFFF;

                    v = (int32_t) sox_alaw2linear16(*((uint8_t*) m->ptr));
                    v = ((v * lo) >> 16) + (v * hi);
                    sum += v;

                    m->ptr = (uint8_t*) m->ptr + 1;
                }

                sum = PA_CLAMP_UNLIKELY(sum, -0x8000, 0x7FFF);
                *((uint8_t*) data) = (uint8_t) sox_13linear2alaw((int16_t) sum >> 3);

                data = (uint8_t*) data + 1;

                if (PA_UNLIKELY(++channel >= spec->channels))
                    channel = 0;
            }

            break;
        }

        case PA_SAMPLE_FLOAT32NE: {
            unsigned channel = 0;

            calc_linear_float_stream_volumes(streams, nstreams, volume, spec);

            while (data < end) {
                float sum = 0;
                unsigned i;

                for (i = 0; i < nstreams; i++) {
                    pa_mix_info *m = streams + i;
                    float v, cv = m->linear[channel].f;

                    if (PA_UNLIKELY(cv <= 0))
                        continue;

                    v = *((float*) m->ptr);
                    v *= cv;
                    sum += v;

                    m->ptr = (uint8_t*) m->ptr + sizeof(float);
                }

                *((float*) data) = sum;

                data = (uint8_t*) data + sizeof(float);

                if (PA_UNLIKELY(++channel >= spec->channels))
                    channel = 0;
            }

            break;
        }

        case PA_SAMPLE_FLOAT32RE: {
            unsigned channel = 0;

            calc_linear_float_stream_volumes(streams, nstreams, volume, spec);

            while (data < end) {
                float sum = 0;
                unsigned i;

                for (i = 0; i < nstreams; i++) {
                    pa_mix_info *m = streams + i;
                    float v, cv = m->linear[channel].f;

                    if (PA_UNLIKELY(cv <= 0))
                        continue;

                    v = PA_FLOAT32_SWAP(*(float*) m->ptr);
                    v *= cv;
                    sum += v;

                    m->ptr = (uint8_t*) m->ptr + sizeof(float);
                }

                *((float*) data) = PA_FLOAT32_SWAP(sum);

                data = (uint8_t*) data + sizeof(float);

                if (PA_UNLIKELY(++channel >= spec->channels))
                    channel = 0;
            }

            break;
        }

        default:
            pa_log_error("Unable to mix audio data of format %s.", pa_sample_format_to_string(spec->format));
            pa_assert_not_reached();
    }

    for (k = 0; k < nstreams; k++)
        pa_memblock_release(streams[k].chunk.memblock);

    return length;
}
