/***
  This file is part of PulseAudio.

  Copyright 2004-2006 Lennart Poettering
  Copyright 2009 Wim Taymans <wim.taymans@collabora.co.uk>
  Copyright 2010 Arun Raghavan <arun.raghavan@collabora.co.uk>

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

#include <math.h>

#include <pulsecore/log.h>
#include <pulsecore/sample-util.h>
#include <pulsecore/svolume-orc-gen.h>

#include "cpu-orc.h"

static pa_do_volume_func_t fallback_s16, fallback_float;

static void volume_s16ne_orc(int16_t *samples, int32_t *volumes, unsigned channels, unsigned length) {
    if (channels == 2) {
        int64_t v = (int64_t)volumes[1] << 32 | volumes[0];
        pa_volume_s16ne_orc_2ch (samples, v, ((length / (sizeof(int16_t))) / 2));
    } else if (channels == 1) {
        pa_volume_s16ne_orc_1ch (samples, volumes[0], length / (sizeof(int16_t)));
    } else {
        int32_t volume = volumes[0];
        for (unsigned i = 0; i < channels; i++) {
            if (volume != volumes[i])
                volume = -1;
        }
        if (volume > -1)
            pa_volume_s16ne_orc_1ch(samples, volumes[0], length / (sizeof(int16_t)));
        else
            fallback_s16(samples, volumes, channels, length);
    }
}

static void volume_float32ne_orc(float *samples, float *volumes, unsigned channels, unsigned length) {
    if (channels == 1) {
        pa_volume_float32ne_orc_1ch(samples, volumes[0], length / (sizeof(float)));
    } else {
        float volume = volumes[0];
        for (unsigned i = 0; i < channels; i++) {
            if (fabsf(volume - volumes[i]) > 0.0f)
                volume = -1;
        }
        if (volume > -1)
            pa_volume_float32ne_orc_1ch(samples, volumes[0], length / (sizeof(float)));
        else
            fallback_float(samples, volumes, channels, length);
    }
}

void pa_volume_func_init_orc(void) {
    pa_log_info("Initialising ORC optimized volume functions.");

    fallback_s16 = pa_get_volume_func(PA_SAMPLE_S16NE);
    pa_set_volume_func(PA_SAMPLE_S16NE, (pa_do_volume_func_t) volume_s16ne_orc);

    fallback_float = pa_get_volume_func(PA_SAMPLE_FLOAT32NE);
    pa_set_volume_func(PA_SAMPLE_FLOAT32NE, (pa_do_volume_func_t) volume_float32ne_orc);
}
