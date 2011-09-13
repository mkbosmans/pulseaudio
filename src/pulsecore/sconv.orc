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

.init sconv_orc_init

# float -> s16
.function sconv_float32ne_to_s16ne_orc_impl
.dest 2 dst int16_t
.source 4 src float
.temp 4 t1 float
.temp 4 t2 int32_t

mulf t1, src, 32768.0
convfl t2, t1
convssslw dst, t2

.function sconv_float32re_to_s16ne_orc_impl
.dest 2 dst int16_t
.source 4 src float
.temp 4 t0 float
.temp 4 t1 float
.temp 4 t2 int32_t

swapl t0, src
mulf t1, t0, 32768.0
convfl t2, t1
convssslw dst, t2

.function sconv_float32ne_to_s16re_orc_impl
.dest 2 dst int16_t
.source 4 src float
.temp 4 t1 float
.temp 4 t2 int32_t
.temp 2 t3 int16_t

mulf t1, src, 32768.0
convfl t2, t1
convssslw t3, t2
swapw dst, t3


# s16 -> float
.function sconv_s16ne_to_float32ne_orc_impl
.dest 4 dst float
.source 2 src int16_t
.temp 4 t1 int32_t
.temp 4 t2 float

convswl t1, src
convlf t2, t1
mulf dst, t2, 0.0000305175781
              # 1.0 / 0x8000

.function sconv_s16re_to_float32ne_orc_impl
.dest 4 dst float
.source 2 src int16_t
.temp 2 t0 int16_t
.temp 4 t1 int32_t
.temp 4 t2 float

swapw t0, src
convswl t1, t0
convlf t2, t1
mulf dst, t2, 0.0000305175781
              # 1.0 / 0x8000

.function sconv_s16ne_to_float32re_orc_impl
.dest 4 dst float
.source 2 src int16_t
.temp 4 t1 int32_t
.temp 4 t2 float
.temp 4 t3 float

convswl t1, src
convlf t2, t1
mulf t3, t2, 0.0000305175781
              # 1.0 / 0x8000
swapl dst, t3


# s16 endianness swap
.function sconv_16bits_swap_orc_impl
.dest 2 dst int16_t
.source 2 src int16_t

swapw dst, src


# s16 -> s32
.function sconv_s16ne_to_s32ne_orc_impl
.dest 4 dst int32_t
.source 2 src int16_t
.temp 4 t1 int32_t

convswl t1, src
shll dst, t1, 16
