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
#include <math.h>
#include <sys/time.h>

#include <pulse/timeval.h>

#include <pulsecore/core-rtclock.h>
#include <pulsecore/macro.h>
#include <pulsecore/remap.h>
#include <pulsecore/cpu.h>
#include <pulsecore/cpu-orc.h>

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

/* Do an int16 and a float remap into the specified destination array */
static void do_remap(pa_remap_t *remap, pa_sample_format_t *format, int16_t *i_d, float *f_d) {
    struct timeval t1, t2;

    *format = PA_SAMPLE_S16NE;
    pa_init_remap(remap);
    memset(i_d, 0, N_SAMPLES * (remap->o_ss->channels) * sizeof(int16_t));
    pa_rtclock_get(&t1);
    for (int i = 0; i < N_REPEAT; i++)
        remap->do_remap(remap, i_d, i_source, N_SAMPLES);
    pa_rtclock_get(&t2);
    printf("Elapsed: %8.1f ms\n", (float) pa_timeval_diff(&t1, &t2) / 1000);

    *format = PA_SAMPLE_FLOAT32NE;
    pa_init_remap(remap);
    memset(f_d, 0, N_SAMPLES * (remap->o_ss->channels) * sizeof(float));
    pa_rtclock_get(&t1);
    for (int i = 0; i < N_REPEAT; i++)
        remap->do_remap(remap, f_d, f_source, N_SAMPLES);
    pa_rtclock_get(&t2);
    printf("Elapsed: %8.1f ms\n", (float) pa_timeval_diff(&t1, &t2) / 1000);
}

/* Compare results against reference implementation and print the samples if there is a difference */
static void dump_samples(const char *str, const int16_t *samples, unsigned nc) {
#if 0
    for (unsigned c = 0; c < nc; c++) {
        printf("%-12s: ", c > 0 ? "" : str);
        for (unsigned i = 0; i < N_SAMPLES; i++)
            printf("  0x%04x", (uint16_t) samples[i*nc+c]);
        printf("\n");
    }
#endif
}
static void dump_samples_f(const char *str, const float *samples, unsigned nc) {
#if 0
    for (unsigned c = 0; c < nc; c++) {
        printf("%-12s: ", c > 0 ? "" : str);
        for (unsigned i = 0; i < N_SAMPLES; i++)
            printf(" %8.5f", samples[i*nc+c]);
        printf("\n");
    }
#endif
}
static int compare_result_to_reference(unsigned n_ic, unsigned n_oc) {
    for (unsigned i = 0; i < n_oc * N_SAMPLES; i++) {
        if (abs(i_dest[i] - i_dest_ref[i]) > 1) {
            printf("FAIL\n");
            dump_samples("input", i_source, n_ic);
            dump_samples("reference", i_dest_ref, n_oc);
            dump_samples("output", i_dest, n_oc);
            return 1;
        }
        if (fabsf(f_dest[i] - f_dest_ref[i]) > 1e-6) {
            printf("FAIL\n");
            dump_samples_f("input", f_source, n_ic);
            dump_samples_f("reference", f_dest_ref, n_oc);
            dump_samples_f("output", f_dest, n_oc);
            return 1;
        }
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

    for (i = 0; i < MAX_CHANNELS*N_SAMPLES; i++) {
        i_source[i] = (int16_t) mrand48();
        f_source[i] = (float) drand48() * 2 - 1;
    }

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
        do_remap(&remap, &format, i_dest_ref, f_dest_ref);

        /* Use c remap functions */
        pa_set_init_remap_func((pa_init_remap_func_t) init_remap_fallback);
        do_remap(&remap, &format, i_dest, f_dest);
        ret += compare_result_to_reference(n_ic, n_oc);

        /* Use optimized remap functions */
        pa_log_set_level(PA_LOG_ERROR);
        cpu_info.cpu_type = PA_CPU_UNDEFINED;
        if (pa_cpu_init_x86(&(cpu_info.flags.x86)))
            cpu_info.cpu_type = PA_CPU_X86;
        if (pa_cpu_init_arm(&(cpu_info.flags.arm)))
            cpu_info.cpu_type = PA_CPU_ARM;
        pa_cpu_init_orc(cpu_info);
        pa_log_set_level(PA_LOG_DEBUG);
        do_remap(&remap, &format, i_dest, f_dest);
        ret += compare_result_to_reference(n_ic, n_oc);
    }

    return ret;
}
