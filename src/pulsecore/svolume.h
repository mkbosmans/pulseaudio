#ifndef foosvolumehfoo
#define foosvolumehfoo

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

#include <pulse/sample.h>

typedef void (*pa_do_volume_func_t) (void *samples, void *volumes, unsigned channels, unsigned length);

pa_do_volume_func_t pa_get_volume_func(pa_sample_format_t f);
void pa_set_volume_func(pa_sample_format_t f, pa_do_volume_func_t func);

#endif
