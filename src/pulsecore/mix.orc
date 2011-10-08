#  This file is part of PulseAudio.
#
#  Copyright 2011 Maarten Bosmans
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

# s16ne mono

.function pa_mix_s16_mono_1_orc
.source 2 src1 int16_t
.dest 2 dst int16_t
.param 4 vol1 int32_t


#.temp 4 t1 int32_t
#.temp 4 t2 int32_t

#convswl t1, src1
#mulll t2, t1, vol1
#convhlw dst, t2


.temp 2 vh int16_t
.temp 2 vl int16_t
.temp 2 t1 int16_t
.temp 2 t2 int16_t

splitlw vh, vl, vol1
mullw t1, vh, src1
mulhsw t2, vl, src1
addssw dst, t1, t2




.function pa_mix_s16_mono_3_orc_
.source 2 src1 int16_t
.source 2 src2 int16_t
.source 2 src3 int16_t
.dest 2 dst int16_t
.param 4 vol1 int32_t
.param 4 vol2 int32_t
.param 4 vol3 int32_t

.temp 4 t1 int32_t
.temp 4 t2 int32_t
.temp 4 t3 int32_t
.temp 4 t4 int32_t

convswl t1, src1
convswl t2, src2
convswl t3, src3

mulll t1, t1, vol1
mulll t2, t2, vol2
mulll t3, t3, vol3

addssl t4, t1, t2
addssl t4, t4, t3
convhlw dst, t4




.function pa_mix_s16_mono_3_orc
.source 2 src1 int16_t
.source 2 src2 int16_t
.source 2 src3 int16_t
.dest 2 dst int16_t
.param 4 vol1 int32_t
.param 4 vol2 int32_t
.param 4 vol3 int32_t


.temp 2 vh int16_t
.temp 2 vl int16_t
.temp 2 t1 int16_t
.temp 2 t2 int16_t
.temp 2 t3 int16_t
.temp 2 t4 int16_t

splitlw vh, vl, vol1
mullw t1, vh, src1
mulhsw t2, vl, src1
addssw t3, t1, t2

splitlw vh, vl, vol2
mullw t1, vh, src2
mulhsw t2, vl, src2
addssw t4, t1, t2
addssw t3, t3, t4

splitlw vh, vl, vol3
mullw t1, vh, src3
mulhsw t2, vl, src3
addssw t4, t1, t2

addssw dst, t3, t4





# float32ne mono

.function pa_mix_float_mono_1_orc
.source 4 src1 float
.dest 4 dst float
.floatparam 4 vol1 float

mulf dst, src1, vol1

.function pa_mix_float_mono_2_orc
.source 4 src1 float
.source 4 src2 float
.dest 4 dst float
.floatparam 4 vol1 float
.floatparam 4 vol2 float
.temp 4 t1 float
.temp 4 t2 float

mulf t1, src1, vol1
mulf t2, src2, vol2
addf dst, t1, t2

.function pa_mix_float_mono_3_orc
.source 4 src1 float
.source 4 src2 float
.source 4 src3 float
.dest 4 dst float
.floatparam 4 vol1 float
.floatparam 4 vol2 float
.floatparam 4 vol3 float
.temp 4 t1 float
.temp 4 t2 float

mulf t1, src1, vol1
mulf t2, src2, vol2
addf t1, t1, t2
mulf t2, src3, vol3
addf dst, t1, t2

.function pa_mix_float_mono_4_orc
.source 4 src1 float
.source 4 src2 float
.source 4 src3 float
.source 4 src4 float
.dest 4 dst float
.floatparam 4 vol1 float
.floatparam 4 vol2 float
.floatparam 4 vol3 float
.floatparam 4 vol4 float
.temp 4 t1 float
.temp 4 t2 float

mulf t1, src1, vol1
mulf t2, src2, vol2
addf t1, t1, t2
mulf t2, src3, vol3
addf t1, t1, t2
mulf t2, src4, vol4
addf dst, t1, t2

.function pa_mix_float_mono_add4_orc
.source 4 src1 float
.source 4 src2 float
.source 4 src3 float
.source 4 src4 float
.dest 4 dst float
.floatparam 4 vol1 float
.floatparam 4 vol2 float
.floatparam 4 vol3 float
.floatparam 4 vol4 float
.temp 4 t1 float
.temp 4 t2 float

mulf t1, src1, vol1
mulf t2, src2, vol2
addf t1, t1, t2
mulf t2, src3, vol3
addf t1, t1, t2
mulf t2, src4, vol4
addf t1, t1, t2
addf dst, dst, t1


# float32ne stereo

.function pa_mix_float_stereo_1_orc
.source 8 src1 float
.dest 8 dst float
.longparam 8 vol1 float

x2 mulf dst, src1, vol1

.function pa_mix_float_stereo_2_orc
.source 8 src1 float
.source 8 src2 float
.dest 8 dst float
.longparam 8 vol1 float
.longparam 8 vol2 float
.temp 8 t1 float
.temp 8 t2 float

x2 mulf t1, src1, vol1
x2 mulf t2, src2, vol2
x2 addf dst, t1, t2

.function pa_mix_float_stereo_3_orc
.source 8 src1 float
.source 8 src2 float
.source 8 src3 float
.dest 8 dst float
.longparam 8 vol1 float
.longparam 8 vol2 float
.longparam 8 vol3 float
.temp 8 t1 float
.temp 8 t2 float

x2 mulf t1, src1, vol1
x2 mulf t2, src2, vol2
x2 addf t1, t1, t2
x2 mulf t2, src3, vol3
x2 addf dst, t1, t2

.function pa_mix_float_stereo_4_orc
.source 8 src1 float
.source 8 src2 float
.source 8 src3 float
.source 8 src4 float
.dest 8 dst float
.longparam 8 vol1 float
.longparam 8 vol2 float
.longparam 8 vol3 float
.longparam 8 vol4 float
.temp 8 t1 float
.temp 8 t2 float

x2 mulf t1, src1, vol1
x2 mulf t2, src2, vol2
x2 addf t1, t1, t2
x2 mulf t2, src3, vol3
x2 addf t1, t1, t2
x2 mulf t2, src4, vol4
x2 addf dst, t1, t2

.function pa_mix_float_stereo_add4_orc
.source 8 src1 float
.source 8 src2 float
.source 8 src3 float
.source 8 src4 float
.dest 8 dst float
.longparam 8 vol1 float
.longparam 8 vol2 float
.longparam 8 vol3 float
.longparam 8 vol4 float
.temp 8 t1 float
.temp 8 t2 float

x2 mulf t1, src1, vol1
x2 mulf t2, src2, vol2
x2 addf t1, t1, t2
x2 mulf t2, src3, vol3
x2 addf t1, t1, t2
x2 mulf t2, src4, vol4
x2 addf t1, t1, t2
x2 addf dst, dst, t1
