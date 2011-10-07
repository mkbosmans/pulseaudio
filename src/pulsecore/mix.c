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
#include "mix_c.c"

static pa_mix_func_t mix_table[] = {
    [PA_SAMPLE_U8]        = (pa_mix_func_t) mix_u8_c,
    [PA_SAMPLE_ALAW]      = (pa_mix_func_t) mix_alaw_c,
    [PA_SAMPLE_ULAW]      = (pa_mix_func_t) mix_ulaw_c,
    [PA_SAMPLE_S16NE]     = (pa_mix_func_t) mix_s16ne_c,
    [PA_SAMPLE_S16RE]     = (pa_mix_func_t) mix_s16re_c,
    [PA_SAMPLE_FLOAT32NE] = (pa_mix_func_t) mix_float32ne_c,
    [PA_SAMPLE_FLOAT32RE] = (pa_mix_func_t) mix_float32re_c,
    [PA_SAMPLE_S32NE]     = (pa_mix_func_t) mix_s32ne_c,
    [PA_SAMPLE_S32RE]     = (pa_mix_func_t) mix_s32re_c,
    [PA_SAMPLE_S24NE]     = (pa_mix_func_t) mix_s24ne_c,
    [PA_SAMPLE_S24RE]     = (pa_mix_func_t) mix_s24re_c,
    [PA_SAMPLE_S24_32NE]  = (pa_mix_func_t) mix_s24_32ne_c,
    [PA_SAMPLE_S24_32RE]  = (pa_mix_func_t) mix_s24_32re_c
};

pa_mix_func_t pa_get_mix_func(pa_sample_format_t f) {
    pa_assert(f >= 0);
    pa_assert(f < PA_SAMPLE_MAX);

    return mix_table[f];
}

void pa_set_mix_func(pa_sample_format_t f, pa_mix_func_t func) {
    pa_assert(f >= 0);
    pa_assert(f < PA_SAMPLE_MAX);

    mix_table[f] = func;
}

size_t pa_mix(
        pa_mix_info streams[],
        unsigned nstreams,
        void *data,
        size_t length,
        const pa_sample_spec *spec,
        const pa_cvolume *volume,
        pa_bool_t mute) {

    pa_mix_func_t mix_func;
    pa_cvolume full_volume;
    unsigned k, c, nchannels;

    const void *stream_ptr[nstreams];

    pa_assert(streams);
    pa_assert(data);
    pa_assert(length);
    pa_assert(spec);

    nchannels = spec->channels;

    if (!volume)
        volume = pa_cvolume_reset(&full_volume, nchannels);

    pa_assert(volume);
    pa_assert(volume->channels == nchannels);

    if (mute || pa_cvolume_is_muted(volume) || nstreams <= 0) {
        pa_silence_memory(data, length, spec);
        return length;
    }

    for (k = 0; k < nstreams; k++)
        stream_ptr[k] = (uint8_t*) pa_memblock_acquire(streams[k].chunk.memblock) + streams[k].chunk.index;

    for (k = 0; k < nstreams; k++)
        if (length > streams[k].chunk.length)
            length = streams[k].chunk.length;

    mix_func = pa_get_mix_func(spec->format);
    switch (spec->format) {

        case PA_SAMPLE_S16NE:
        case PA_SAMPLE_S16RE:
        case PA_SAMPLE_S32NE:
        case PA_SAMPLE_S32RE:
        case PA_SAMPLE_S24NE:
        case PA_SAMPLE_S24RE:
        case PA_SAMPLE_S24_32NE:
        case PA_SAMPLE_S24_32RE:
        case PA_SAMPLE_U8:
        case PA_SAMPLE_ULAW:
        case PA_SAMPLE_ALAW: {
            int32_t stream_linear_volume[nstreams][PA_CHANNELS_MAX];

            for (k = 0; k < nstreams; k++) {
                for (c = 0; c < nchannels; c++) {
                    stream_linear_volume[k][c] = (int32_t) lrint(pa_sw_volume_to_linear(streams[k].volume.values[c]) *
                                                                 pa_sw_volume_to_linear(volume->values[c]) * 0x10000);
                }
            }

            mix_func(stream_linear_volume, nstreams, nchannels, stream_ptr, data, length);

            break;
        }

        case PA_SAMPLE_FLOAT32NE:
        case PA_SAMPLE_FLOAT32RE: {
            float stream_linear_volume[nstreams][PA_CHANNELS_MAX];

            for (k = 0; k < nstreams; k++) {
                for (c = 0; c < nchannels; c++) {
                    stream_linear_volume[k][c] = (float) (pa_sw_volume_to_linear(streams[k].volume.values[c]) *
                                                          pa_sw_volume_to_linear(volume->values[c]));
                }
            }

            mix_func(stream_linear_volume, nstreams, nchannels, stream_ptr, data, length);

            break;
        }

        default:
            pa_assert_not_reached();
    }

    for (k = 0; k < nstreams; k++)
        pa_memblock_release(streams[k].chunk.memblock);

    return length;
}
