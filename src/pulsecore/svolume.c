/***
  This file is part of PulseAudio.

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

#include "svolume.h"

#include "svolume_c.c"

static pa_do_volume_func_t do_volume_table[] = {
    [PA_SAMPLE_U8]        = (pa_do_volume_func_t) volume_u8_c,
    [PA_SAMPLE_ALAW]      = (pa_do_volume_func_t) volume_alaw_c,
    [PA_SAMPLE_ULAW]      = (pa_do_volume_func_t) volume_ulaw_c,
    [PA_SAMPLE_S16NE]     = (pa_do_volume_func_t) volume_s16ne_c,
    [PA_SAMPLE_S16RE]     = (pa_do_volume_func_t) volume_s16re_c,
    [PA_SAMPLE_FLOAT32NE] = (pa_do_volume_func_t) volume_float32ne_c,
    [PA_SAMPLE_FLOAT32RE] = (pa_do_volume_func_t) volume_float32re_c,
    [PA_SAMPLE_S32NE]     = (pa_do_volume_func_t) volume_s32ne_c,
    [PA_SAMPLE_S32RE]     = (pa_do_volume_func_t) volume_s32re_c,
    [PA_SAMPLE_S24NE]     = (pa_do_volume_func_t) volume_s24ne_c,
    [PA_SAMPLE_S24RE]     = (pa_do_volume_func_t) volume_s24re_c,
    [PA_SAMPLE_S24_32NE]  = (pa_do_volume_func_t) volume_s24_32ne_c,
    [PA_SAMPLE_S24_32RE]  = (pa_do_volume_func_t) volume_s24_32re_c
};

pa_do_volume_func_t pa_get_volume_func(pa_sample_format_t f) {
    pa_assert(f >= 0);
    pa_assert(f < PA_SAMPLE_MAX);

    return do_volume_table[f];
}

void pa_set_volume_func(pa_sample_format_t f, pa_do_volume_func_t func) {
    pa_assert(f >= 0);
    pa_assert(f < PA_SAMPLE_MAX);

    do_volume_table[f] = func;
}
