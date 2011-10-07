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
