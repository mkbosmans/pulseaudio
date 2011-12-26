/***
  This file is part of PulseAudio.

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

#include "cpu-orc.h"

void pa_cpu_init_orc(pa_cpu_info cpu_info)
{
#ifndef DISABLE_ORC
    /* Update these as we test on more architectures */
    pa_cpu_x86_flag_t x86_want_flags = PA_CPU_X86_MMX | PA_CPU_X86_SSE | PA_CPU_X86_SSE2 | PA_CPU_X86_SSE3 | PA_CPU_X86_SSSE3 | PA_CPU_X86_SSE4_1 | PA_CPU_X86_SSE4_2;

    /* Enable Orc optimizations */
    if ((cpu_info.cpu_type == PA_CPU_X86) && (cpu_info.flags.x86 & x86_want_flags)) {
        pa_volume_func_init_orc();
        pa_mix_func_init_orc();
        pa_remap_func_init_orc();
        pa_convert_func_init_orc();
    }
#endif
}
