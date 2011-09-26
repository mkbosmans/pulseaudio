#  This file is part of PulseAudio.
#
#  Copyright 2011 Maarten Bosmans <mkbosmans@gmail.com>
#
#  PulseAudio is free software; you can redistribute it and/or modify
#  it under the terms of the GNU Lesser General Public License as published
#  by the Free Software Foundation; either version 2.1 of the License,
#  or (at your option) any later version.
#
#  PulseAudio is distributed in the hope that it will be useful, but
#  WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
#  General Public License for more details.
#
#  You should have received a copy of the GNU Lesser General Public License
#  along with PulseAudio; if not, write to the Free Software
#  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
#  USA.

.function remap_mono_to_stereo_float_orc
.init remap_mono_to_stereo_orc_init
.dest 8 dst float
.source 4 src float
.temp 8 t1
.temp 8 t2
convulq t1 src
swaplq t2 t1
orq dst t1 t2

.function remap_mono_to_stereo_s16_orc
.init remap_mono_to_stereo_orc_init
.dest 4 dst int16_t
.source 2 src int16_t
mergewl dst, src, src


.function remap_stereo_to_mono_float_orc
.init remap_stereo_to_mono_orc_init
.dest 4 dst float
.source 8 src float
.temp 4 t1
.temp 4 t2
.temp 4 t3
splitql t1, t2, src
addf t3, t1, t2
mulf dst, t3, 0.5

.function remap_stereo_to_mono_s16_orc
.init remap_stereo_to_mono_orc_init
.dest 2 dst int16_t
.source 4 src int16_t
.temp 2 t1
.temp 2 t2
splitlw t1, t2, src
avgsw dst, t1, t2


.function remap_stereo_swap_float_orc
.init remap_stereo_swap_orc_init
.dest 8 dst float
.source 8 src float
swaplq dst, src

.function remap_stereo_swap_s16_orc
.init remap_stereo_swap_orc_init
.dest 4 dst int16_t
.source 4 src int16_t
swapwl dst, src
