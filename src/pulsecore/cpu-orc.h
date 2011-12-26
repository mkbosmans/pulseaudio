#ifndef foocpuorchfoo
#define foocpuorchfoo

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

#include <pulsecore/cpu.h>

/* Orc-optimised bits */

void pa_cpu_init_orc(pa_cpu_info cpu_info);

void pa_volume_func_init_orc(void);

void pa_mix_func_init_orc(void);

void pa_remap_func_init_orc(void);

void pa_convert_func_init_orc(void);

#endif /* foocpuorchfoo */
