/***
  This file is part of PulseAudio.

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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>

#include <pulsecore/macro.h>
#include <pulsecore/remap.h>
#include <pulsecore/cpu.h>
#include <pulsecore/cpu-orc.h>

#include <tests/sample-test-util.h>

#define MAX_CHANNELS 6
#define N_SAMPLES 16000
#define N_REPEAT 300

struct channel_map_table {
    unsigned n_ic, n_oc;
    int32_t map_table[MAX_CHANNELS*MAX_CHANNELS];
};

static int16_t i_source[MAX_CHANNELS*N_SAMPLES], i_dest[MAX_CHANNELS*N_SAMPLES], i_dest_ref[MAX_CHANNELS*N_SAMPLES];
static float f_source[MAX_CHANNELS*N_SAMPLES], f_dest[MAX_CHANNELS*N_SAMPLES], f_dest_ref[MAX_CHANNELS*N_SAMPLES];

/* These two functions act as reference remap implementations
 * to which all the optimized ones in pulsecore are compared
 */
static void reference_remap_channels_float(pa_remap_t *m, void *dst, const void *src, unsigned n) {
    unsigned n_ic = m->i_ss->channels;
    unsigned n_oc = m->o_ss->channels;

    memset(dst, 0, n * sizeof(float) * n_oc);
    for (unsigned oc = 0; oc < n_oc; oc++) {
        for (unsigned ic = 0; ic < n_ic; ic++) {
            float vol = m->map_table_f[oc][ic];
            float *d = (float *)dst + oc;
            float *s = (float *)src + ic;
            for (unsigned i = n; i > 0; i--, s += n_ic, d += n_oc)
                *d += *s * vol;
        }
    }
}
static void reference_remap_channels_int16(pa_remap_t *m, void *dst, const void *src, unsigned n) {
    unsigned n_ic = m->i_ss->channels;
    unsigned n_oc = m->o_ss->channels;

    memset(dst, 0, n * sizeof(int16_t) * n_oc);
    for (unsigned oc = 0; oc < n_oc; oc++) {
        for (unsigned ic = 0; ic < n_ic; ic++) {
            int32_t vol = m->map_table_i[oc][ic];
            int16_t *d = (int16_t *)dst + oc;
            int16_t *s = (int16_t *)src + ic;
            for (unsigned i = n; i > 0; i--, s += n_ic, d += n_oc)
                *d += (int16_t) (((int32_t)*s * vol) >> 16);
        }
    }
}

static void init_remap_reference(pa_remap_t *m) {
    if (*m->format == PA_SAMPLE_S16NE) {
        m->do_remap = (pa_do_remap_func_t) reference_remap_channels_int16;
        pa_log_info("Using int16 reference implementation");
    } else if (*m->format == PA_SAMPLE_FLOAT32NE) {
        m->do_remap = (pa_do_remap_func_t) reference_remap_channels_float;
        pa_log_info("Using float reference implementation");
    }
}

static void init_remap_fallback(pa_remap_t *m) {}

/* Do an int16 or a float remap into the specified destination array */
static void do_remap(pa_remap_t *remap, void *source, void *dest, pa_sample_format_t format) {
    double avg, min, max, stddev;

    pa_init_remap(remap);
    INIT_TIMED_TEST(10)
        memset(dest, 0, N_SAMPLES * (remap->o_ss->channels) * pa_sample_size_of_format(format));
    START_TIMED_TEST(N_REPEAT)
        remap->do_remap(remap, dest, source, N_SAMPLES);
    END_TIMED_TEST(avg, min, max, stddev)

    printf("\t%6.1f ms\t(min =%6.1f, max =%6.1f, stddev =%5.1f)\n", avg, min, max, stddev);
}

/* Compare results against reference implementation and print the samples if there is a difference */
static int compare_result_to_reference(void *source, void *dest_ref, void *dest, pa_sample_format_t format, unsigned n_ic, unsigned n_oc, unsigned max_error) {
    if (compare_samples(dest, dest_ref, format, n_oc * N_SAMPLES) > max_error) {
        printf("FAIL\n");
        dump_channel_samples("input", source, format, PA_MIN(N_SAMPLES, 20), n_ic);
        dump_channel_samples("reference", dest_ref, format, PA_MIN(N_SAMPLES, 20), n_oc);
        dump_channel_samples("output", dest, format, PA_MIN(N_SAMPLES, 20), n_oc);
        return 1;
    }
    return 0;
}


/* This test generates random samples and compares the remap functions
 * to a reference implementation.  For S16NE samples the test succeeds
 * when the remapped samples deviate from the reference by at most one
 * sample value, for float samples the test succeeds when the absolute
 * difference is smaller than 1e-6.
 */
int main(int argc, char *argv[]) {

    static const struct channel_map_table maps[] = {
        { 1, 2, { 0x10000, 0x10000 } },
        { 1, 2, { 0x09999, 0x08000 } },
        { 2, 1, { 0x08000, 0x08000 } },
        { 2, 1, { 0x0E999, 0x01000 } },
        { 2, 2, { 0x00000, 0x10000, 0x10000, 0x00000 } },
       /* i  o     0->0     1->0     0->1     1->1     0->2     1->2   */
        { 2, 3, { 0x0A999, 0x00999, 0x00999, 0x01000, 0x00700, 0x00000 } },
        { 2, 6, { 0x10000, 0x00000, 0x00000, 0x10000, 0x08000, 0x08000, 0x10000, 0x00000, 0x00000, 0x10000, 0x08000, 0x08000 } },
        { 2, 6, { 0x0F000, 0x00000, 0x00000, 0x0F000, 0x08000, 0x08000, 0x0F000, 0x00000, 0x00000, 0x0F000, 0x08000, 0x08000 } },
        { 6, 2, { 0x10000, 0x00000, 0x00000, 0x00000, 0x00000, 0x00000, 0x00000, 0x10000, 0x00000, 0x00000, 0x00000, 0x00000 } },
        { 6, 2, { 0x0ACCD, 0x00000, 0x0199A, 0x00000, 0x06000, 0x06000, 0x00000, 0x0ACCD, 0x00000, 0x0199A, 0x06000, 0x06000 } },
        { 0, 0, { 0 } }
    };

    struct pa_cpu_info cpu_info;
    unsigned m, i, j;
    unsigned n_oc, n_ic;
    int ret = 0;

    pa_sample_format_t format = PA_SAMPLE_S16NE;
    pa_sample_spec i_ss, o_ss;
    pa_remap_t remap;

    i_ss.rate = o_ss.rate = 1000;
    i_ss.format = o_ss.format = format;
    remap.format = &format;
    remap.i_ss = &i_ss;
    remap.o_ss = &o_ss;

    generate_random_samples(i_source, PA_SAMPLE_S16NE, MAX_CHANNELS*N_SAMPLES);
    generate_random_samples(f_source, PA_SAMPLE_FLOAT32NE, MAX_CHANNELS*N_SAMPLES);

    pa_log_set_level(PA_LOG_DEBUG);

    for (m = 0; maps[m].n_oc > 0; m++) {
        n_ic = maps[m].n_ic;
        n_oc = maps[m].n_oc;
        printf("\n%d -> %d\n", n_ic, n_oc);

        /* set remap parameters */
        i_ss.channels = n_ic;
        o_ss.channels = n_oc;
        memset(remap.map_table_f, 0, sizeof(remap.map_table_f));
        memset(remap.map_table_i, 0, sizeof(remap.map_table_i));
        for (i = 0; i < n_oc; i++) {
            for (j = 0; j < n_ic; j++) {
              remap.map_table_f[i][j] = (float) maps[m].map_table[i*n_ic+j] / (float) 0x10000;
              remap.map_table_i[i][j] = maps[m].map_table[i*n_ic+j];
            }
        }

        /* Use reference remap function implementation */
        pa_set_init_remap_func((pa_init_remap_func_t) init_remap_reference);
        format = PA_SAMPLE_S16NE;
        do_remap(&remap, i_source, i_dest_ref, format);
        format = PA_SAMPLE_FLOAT32NE;
        do_remap(&remap, f_source, f_dest_ref, format);

        /* Use c remap functions */
        pa_set_init_remap_func((pa_init_remap_func_t) init_remap_fallback);
        format = PA_SAMPLE_S16NE;
        do_remap(&remap, i_source, i_dest, format);
        format = PA_SAMPLE_FLOAT32NE;
        do_remap(&remap, f_source, f_dest, format);
        ret += compare_result_to_reference(i_source, i_dest_ref, i_dest, PA_SAMPLE_S16NE, n_ic, n_oc, 1);
        ret += compare_result_to_reference(f_source, f_dest_ref, f_dest, PA_SAMPLE_FLOAT32NE, n_ic, n_oc, 2);

        /* Use optimized remap functions */
        pa_log_set_level(PA_LOG_ERROR);
        cpu_info.cpu_type = PA_CPU_UNDEFINED;
        if (pa_cpu_init_x86(&(cpu_info.flags.x86)))
            cpu_info.cpu_type = PA_CPU_X86;
        if (pa_cpu_init_arm(&(cpu_info.flags.arm)))
            cpu_info.cpu_type = PA_CPU_ARM;
        pa_cpu_init_orc(cpu_info);
        pa_log_set_level(PA_LOG_DEBUG);
        format = PA_SAMPLE_S16NE;
        do_remap(&remap, i_source, i_dest, format);
        format = PA_SAMPLE_FLOAT32NE;
        do_remap(&remap, f_source, f_dest, format);
        ret += compare_result_to_reference(i_source, i_dest_ref, i_dest, PA_SAMPLE_S16NE, n_ic, n_oc, 1);
        ret += compare_result_to_reference(f_source, f_dest_ref, f_dest, PA_SAMPLE_FLOAT32NE, n_ic, n_oc, 2);
    }

    return ret;
}
