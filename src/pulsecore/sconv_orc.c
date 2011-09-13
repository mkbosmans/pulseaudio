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

#include <pulsecore/sconv.h>
#include <pulsecore/sconv-orc-gen.h>

#include "cpu-orc.h"

#define MAKE_ORC_FUNCTION(name)                                                \
static void sconv_ ## name ## _orc(unsigned n, const void *a, void *b) {       \
    pa_assert(a);                                                              \
    pa_assert(b);                                                              \
    sconv_ ## name ## _orc_impl(b, a, n);                                      \
}

MAKE_ORC_FUNCTION(float32ne_to_s16ne)
MAKE_ORC_FUNCTION(float32re_to_s16ne)
MAKE_ORC_FUNCTION(float32ne_to_s16re)

MAKE_ORC_FUNCTION(s16ne_to_float32ne)
MAKE_ORC_FUNCTION(s16re_to_float32ne)
MAKE_ORC_FUNCTION(s16ne_to_float32re)

MAKE_ORC_FUNCTION(16bits_swap)

MAKE_ORC_FUNCTION(s16ne_to_s32ne)

void pa_convert_func_init_orc() {
    pa_log_info("Initialising Orc optimized conversions.");
    sconv_orc_init();

    pa_set_convert_from_float32ne_function(PA_SAMPLE_S16NE, sconv_float32ne_to_s16ne_orc);
    pa_set_convert_from_float32ne_function(PA_SAMPLE_S16RE, sconv_float32ne_to_s16re_orc);
    pa_set_convert_to_s16ne_function(PA_SAMPLE_FLOAT32RE, sconv_float32re_to_s16ne_orc);
    pa_set_convert_to_s16ne_function(PA_SAMPLE_FLOAT32NE, sconv_float32ne_to_s16ne_orc);

    pa_set_convert_from_s16ne_function(PA_SAMPLE_FLOAT32NE, sconv_s16ne_to_float32ne_orc);
    pa_set_convert_from_s16ne_function(PA_SAMPLE_FLOAT32RE, sconv_s16ne_to_float32re_orc);
    pa_set_convert_to_float32ne_function(PA_SAMPLE_S16RE, sconv_s16re_to_float32ne_orc);
    pa_set_convert_to_float32ne_function(PA_SAMPLE_S16NE, sconv_s16ne_to_float32ne_orc);

    pa_set_convert_from_s16ne_function(PA_SAMPLE_S16RE, sconv_16bits_swap_orc);
    pa_set_convert_to_s16ne_function(PA_SAMPLE_S16RE, sconv_16bits_swap_orc);

    pa_set_convert_from_s16ne_function(PA_SAMPLE_S32NE, sconv_s16ne_to_s32ne_orc);
}
