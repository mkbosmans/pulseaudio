#ifndef foomixhfoo
#define foomixhfoo

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

#include <pulse/sample.h>
#include <pulse/volume.h>

#include <pulsecore/memblock.h>
#include <pulsecore/memchunk.h>

typedef struct pa_mix_info {
    pa_memchunk chunk;
    pa_cvolume volume;
    void *userdata;
} pa_mix_info;

size_t pa_mix(
    pa_mix_info channels[],
    unsigned nchannels,
    void *data,
    size_t length,
    const pa_sample_spec *spec,
    const pa_cvolume *volume,
    pa_bool_t mute);

typedef void (*pa_mix_func_t) (void *volumes, unsigned nstreams, unsigned nchannels, const void *src[], void *dest, size_t length);

pa_mix_func_t pa_get_mix_func(pa_sample_format_t f);
void pa_set_mix_func(pa_sample_format_t f, pa_mix_func_t func);

#endif
