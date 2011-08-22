/***
  This file is part of PulseAudio.

  Copyright 2011 Maarten Bosmans <mkbosmans@gmail.com>

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

#include <pulsecore/sample-util.h>
#include <pulsecore/remap-orc-gen.h>

#include "remap.h"
#include "cpu-orc.h"

static void remap_mono_to_stereo_orc(pa_remap_t *m, void *dst, const void *src, unsigned n) {
    switch (*m->format) {
        case PA_SAMPLE_FLOAT32NE:
            remap_mono_to_stereo_float_orc(dst, src, n);
            break;
        case PA_SAMPLE_S16NE:
            remap_mono_to_stereo_s16_orc(dst, src, n);
            break;
        default:
            pa_assert_not_reached();
    }
}

static void remap_stereo_to_mono_orc(pa_remap_t *m, void *dst, const void *src, unsigned n) {
    switch (*m->format) {
        case PA_SAMPLE_FLOAT32NE:
            remap_stereo_to_mono_float_orc(dst, src, n);
            break;
        case PA_SAMPLE_S16NE:
            remap_stereo_to_mono_s16_orc(dst, src, n);
            break;
        default:
            pa_assert_not_reached();
    }
}

static void remap_stereo_swap_orc(pa_remap_t *m, void *dst, const void *src, unsigned n) {
    switch (*m->format) {
        case PA_SAMPLE_FLOAT32NE:
            remap_stereo_swap_float_orc(dst, src, n);
            break;
        case PA_SAMPLE_S16NE:
            remap_stereo_swap_s16_orc(dst, src, n);
            break;
        default:
            pa_assert_not_reached();
    }
}


/* set the function that will execute the remapping based on the matrices */
static void init_remap_orc(pa_remap_t *m) {
    unsigned n_oc, n_ic;

    n_oc = m->o_ss->channels;
    n_ic = m->i_ss->channels;

    /* find some common channel remappings, fall back to full matrix operation. */
    if (n_ic == 1 && n_oc == 2 &&
            m->map_table_f[0][0] >= 1.0 && m->map_table_f[1][0] >= 1.0) {
        m->do_remap = (pa_do_remap_func_t) remap_mono_to_stereo_orc;
        pa_log_info("Using ORC mono to stereo remapping");
    } else if (n_ic == 2 && n_oc == 1 &&
            m->map_table_i[0][0] == 0x8000 && m->map_table_i[0][1] == 0x8000) {
        m->do_remap = (pa_do_remap_func_t) remap_stereo_to_mono_orc;
        pa_log_info("Using ORC stereo to mono remapping");
    } else if (n_ic == 2 && n_oc == 2 &&
            m->map_table_i[0][0] == 0x00000 && m->map_table_i[0][1] == 0x10000 &&
            m->map_table_i[1][0] == 0x10000 && m->map_table_i[1][1] == 0x00000) {
        m->do_remap = (pa_do_remap_func_t) remap_stereo_swap_orc;
        pa_log_info("Using ORC stereo swap remapping");
    }
}

void pa_remap_func_init_orc(void) {
    pa_log_info("Initialising ORC optimized remappers.");
    remap_orc_init();

    pa_set_init_remap_func((pa_init_remap_func_t) init_remap_orc);
}
