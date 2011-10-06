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

static void calc_linear_integer_stream_volumes(pa_mix_info streams[], int32_t linear_volume[][PA_CHANNELS_MAX], unsigned nstreams, const pa_cvolume *volume) {
    unsigned k, channel, nchannels;
    double linear[PA_CHANNELS_MAX];

    nchannels = volume->channels;

    for (channel = 0; channel < nchannels; channel++)
        linear[channel] = pa_sw_volume_to_linear(volume->values[channel]);

    for (k = 0; k < nstreams; k++) {
        for (channel = 0; channel < nchannels; channel++) {
            pa_mix_info *m = streams + k;
            linear_volume[k][channel] = (int32_t) lrint(pa_sw_volume_to_linear(m->volume.values[channel]) * linear[channel] * 0x10000);
        }
    }
}

static void calc_linear_float_stream_volumes(pa_mix_info streams[], float linear_volume[][PA_CHANNELS_MAX], unsigned nstreams, const pa_cvolume *volume) {
    unsigned k, channel, nchannels;
    double linear[PA_CHANNELS_MAX];

    nchannels = volume->channels;

    for (channel = 0; channel < nchannels; channel++)
        linear[channel] = pa_sw_volume_to_linear(volume->values[channel]);

    for (k = 0; k < nstreams; k++) {
        for (channel = 0; channel < nchannels; channel++) {
            pa_mix_info *m = streams + k;
            linear_volume[k][channel] = (float) (pa_sw_volume_to_linear(m->volume.values[channel]) * linear[channel]);
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

    void *stream_ptr[nstreams];

    pa_assert(streams);
    pa_assert(data);
    pa_assert(length);
    pa_assert(spec);

    if (!volume)
        volume = pa_cvolume_reset(&full_volume, spec->channels);

    pa_assert(volume);
    pa_assert(volume->channels == spec->channels);

    if (mute || pa_cvolume_is_muted(volume) || nstreams <= 0) {
        pa_silence_memory(data, length, spec);
        return length;
    }

    for (k = 0; k < nstreams; k++)
        stream_ptr[k] = (uint8_t*) pa_memblock_acquire(streams[k].chunk.memblock) + streams[k].chunk.index;

    for (k = 0; k < nstreams; k++)
        if (length > streams[k].chunk.length)
            length = streams[k].chunk.length;

    switch (spec->format) {

        case PA_SAMPLE_S16NE: {
            int32_t stream_linear_volume[nstreams][PA_CHANNELS_MAX];

            calc_linear_integer_stream_volumes(streams, stream_linear_volume, nstreams, volume);
            mix_s16ne_c(stream_linear_volume, nstreams, spec->channels, stream_ptr, data, length);

            break;
        }

        case PA_SAMPLE_S16RE: {
            int32_t stream_linear_volume[nstreams][PA_CHANNELS_MAX];

            calc_linear_integer_stream_volumes(streams, stream_linear_volume, nstreams, volume);
            mix_s16re_c(stream_linear_volume, nstreams, spec->channels, stream_ptr, data, length);

            break;
        }

        case PA_SAMPLE_S32NE: {
            int32_t stream_linear_volume[nstreams][PA_CHANNELS_MAX];

            calc_linear_integer_stream_volumes(streams, stream_linear_volume, nstreams, volume);
            mix_s32ne_c(stream_linear_volume, nstreams, spec->channels, stream_ptr, data, length);

            break;
        }

        case PA_SAMPLE_S32RE: {
            int32_t stream_linear_volume[nstreams][PA_CHANNELS_MAX];

            calc_linear_integer_stream_volumes(streams, stream_linear_volume, nstreams, volume);
            mix_s32re_c(stream_linear_volume, nstreams, spec->channels, stream_ptr, data, length);

            break;
        }

        case PA_SAMPLE_S24NE: {
            int32_t stream_linear_volume[nstreams][PA_CHANNELS_MAX];

            calc_linear_integer_stream_volumes(streams, stream_linear_volume, nstreams, volume);
            mix_s24ne_c(stream_linear_volume, nstreams, spec->channels, stream_ptr, data, length);

            break;
        }

        case PA_SAMPLE_S24RE: {
            int32_t stream_linear_volume[nstreams][PA_CHANNELS_MAX];

            calc_linear_integer_stream_volumes(streams, stream_linear_volume, nstreams, volume);
            mix_s24re_c(stream_linear_volume, nstreams, spec->channels, stream_ptr, data, length);

            break;
        }

        case PA_SAMPLE_S24_32NE: {
            int32_t stream_linear_volume[nstreams][PA_CHANNELS_MAX];

            calc_linear_integer_stream_volumes(streams, stream_linear_volume, nstreams, volume);
            mix_s24_32ne_c(stream_linear_volume, nstreams, spec->channels, stream_ptr, data, length);

            break;
        }

        case PA_SAMPLE_S24_32RE: {
            int32_t stream_linear_volume[nstreams][PA_CHANNELS_MAX];

            calc_linear_integer_stream_volumes(streams, stream_linear_volume, nstreams, volume);
            mix_s24_32re_c(stream_linear_volume, nstreams, spec->channels, stream_ptr, data, length);

            break;
        }

        case PA_SAMPLE_U8: {
            int32_t stream_linear_volume[nstreams][PA_CHANNELS_MAX];

            calc_linear_integer_stream_volumes(streams, stream_linear_volume, nstreams, volume);
            mix_u8_c(stream_linear_volume, nstreams, spec->channels, stream_ptr, data, length);

            break;
        }

        case PA_SAMPLE_ULAW: {
            int32_t stream_linear_volume[nstreams][PA_CHANNELS_MAX];

            calc_linear_integer_stream_volumes(streams, stream_linear_volume, nstreams, volume);
            mix_ulaw_c(stream_linear_volume, nstreams, spec->channels, stream_ptr, data, length);

            break;
        }

        case PA_SAMPLE_ALAW: {
            int32_t stream_linear_volume[nstreams][PA_CHANNELS_MAX];

            calc_linear_integer_stream_volumes(streams, stream_linear_volume, nstreams, volume);
            mix_alaw_c(stream_linear_volume, nstreams, spec->channels, stream_ptr, data, length);

            break;
        }

        case PA_SAMPLE_FLOAT32NE: {
            float stream_linear_volume[nstreams][PA_CHANNELS_MAX];

            calc_linear_float_stream_volumes(streams, stream_linear_volume, nstreams, volume);
            mix_float32ne_c(stream_linear_volume, nstreams, spec->channels, stream_ptr, data, length);

            break;
        }

        case PA_SAMPLE_FLOAT32RE: {
            float stream_linear_volume[nstreams][PA_CHANNELS_MAX];

            calc_linear_float_stream_volumes(streams, stream_linear_volume, nstreams, volume);
            mix_float32re_c(stream_linear_volume, nstreams, spec->channels, stream_ptr, data, length);

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
