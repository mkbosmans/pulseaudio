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

#include <pulsecore/log.h>
#include <pulsecore/macro.h>
#include <pulsecore/mix.h>
#include <pulsecore/mix-orc-gen.h>
#include <pulsecore/sample-util.h>

#include "cpu-orc.h"

static pa_mix_func_t fallback_float;

static void mix_float32ne_orc(float stream_volume[][PA_CHANNELS_MAX], unsigned nstreams, unsigned nchannels, const void *src[], void *dest, size_t length) {
    if (nchannels == 1) {
        if (nstreams % 4 == 1)
            pa_mix_float_mono_1_orc(dest, src[0], stream_volume[0][0], length/4);
        else if (nstreams % 4 == 2)
            pa_mix_float_mono_2_orc(dest, src[0], src[1], stream_volume[0][0], stream_volume[1][0], length/4);
        else if (nstreams % 4 == 3)
            pa_mix_float_mono_3_orc(dest, src[0], src[1], src[2], stream_volume[0][0], stream_volume[1][0], stream_volume[2][0], length/4);
        else if (nstreams % 4 == 0)
            pa_mix_float_mono_4_orc(dest, src[0], src[1], src[2], src[3], stream_volume[0][0], stream_volume[1][0], stream_volume[2][0], stream_volume[3][0], length/4);

        for (unsigned n = (nstreams-1) % 4; n+4 < nstreams; n += 4) {
            pa_mix_float_mono_add4_orc(dest, src[n+1], src[n+2], src[n+3], src[n+4], stream_volume[n+1][0], stream_volume[n+2][0], stream_volume[n+3][0], stream_volume[n+4][0], length/4);
        }
    } else if (nchannels == 2) {
        if (nstreams == 2) {

            int64_t v1 = ((int64_t *) stream_volume)[0];
            int64_t v2 = ((int64_t *) stream_volume)[PA_CHANNELS_MAX/2];

            pa_mix_float_stereo_2_orc(dest, src[0], src[1], v1, v2, length/8);

        } else {
            fallback_float(stream_volume, nstreams, nchannels, src, dest, length);
        }
    } else {
        fallback_float(stream_volume, nstreams, nchannels, src, dest, length);
    }
}

void pa_mix_func_init_orc(void) {
    pa_log_info("Initialising ORC optimized mix functions.");

    fallback_float = pa_get_mix_func(PA_SAMPLE_FLOAT32NE);
    pa_set_mix_func(PA_SAMPLE_FLOAT32NE, (pa_mix_func_t) mix_float32ne_orc);
}
