/***
  This file is part of PulseAudio.

  Copyright 2009 Wim Taymans <wim.taymans@collabora.co.uk>
  Copyright 2011 Maarten Bosmans <mkbosmans@gmail.com>

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

#include <pulse/rtclock.h>
#include <pulse/volume.h>

#include <pulsecore/macro.h>
#include <pulsecore/cpu.h>
#include <pulsecore/cpu-orc.h>
#include <pulsecore/svolume.h>

#include <tests/sample-test-util.h>

#define MAX_CHANNELS 4
#define N_SAMPLES 9600
#define REPEAT 500
#define TIMES 40
#define PADDING 16

static void do_svolume_perf_test(pa_do_volume_func_t func, const char *name, uint8_t *samples_orig, uint8_t *samples, int32_t *volumes, unsigned channels, size_t length) {
    int j, k;
    pa_usec_t start, stop;
    double time; /* in ms */
    double min = 1e9, max = 0, avg1 = 0, avg2 = 0;

    for (k = 0; k < TIMES; k++) {
        memcpy(samples, samples_orig, length);
        start = pa_rtclock_now();
        for (j = 0; j < REPEAT; j++)
            func(samples, volumes, channels, length);
        stop = pa_rtclock_now();
        time = (stop - start) / 1000.0;

        min = PA_MIN(min, time);
        max = PA_MAX(max, time);
        avg1 += time / (double) TIMES;
        avg2 += time * time / (double) TIMES;
    }

    pa_log_info("\t%s:%6.1f ms\t(min =%6.1f, max =%6.1f, stddev =%5.1f)", name, avg1, min, max, sqrt(avg2 - avg1 * avg1));
}

static int compare_samples(const char *name, pa_sample_format_t format, uint8_t *samples_orig, uint8_t *samples_ref, uint8_t *samples_opt, int32_t *volumes, unsigned channels) {
    int ret = 0;
    unsigned i;

    if (format == PA_SAMPLE_S16LE || format == PA_SAMPLE_S16BE) {
        for (i = 0; i < N_SAMPLES * channels; i++) {
            if (((int16_t *) samples_opt)[i] != ((int16_t *) samples_ref)[i])
                ret++;
        }
    } else if (format == PA_SAMPLE_FLOAT32LE || format == PA_SAMPLE_FLOAT32BE) {
        for (i = 0; i < N_SAMPLES * channels; i++) {
            if (fabsf(((float *) samples_opt)[i] - ((float *) samples_ref)[i]) > 1e-8)
                ret++;
        }
    } else {
        if (memcmp(samples_opt, samples_ref, pa_sample_size_of_format(format) * N_SAMPLES * channels))
            ret++;
    }

    if (ret)
        printf("%s not identical to reference\n", name);
    return ret;
}

static void set_channel_volumes(int32_t *volumes, unsigned channels, pa_bool_t use_fixed, pa_sample_format_t format) {
    unsigned i, padding;

    if (format == PA_SAMPLE_FLOAT32LE || format == PA_SAMPLE_FLOAT32BE) {
        for (i = 0; i < channels; i++)
            ((float *) volumes)[i] = use_fixed ? 0.8f : (float) drand48() * 2.0f;
    } else {
        for (i = 0; i < channels; i++)
            volumes[i] = use_fixed ? 0x0000A000 : PA_CLAMP_VOLUME((uint32_t) rand() >> 10);
    }
    for (padding = 0; padding < PADDING; padding++, i++)
        volumes[i] = volumes[padding];
}

int main(int argc, char *argv[]) {

    static const pa_sample_format_t formats[] = {
        PA_SAMPLE_S16NE,
        PA_SAMPLE_S16RE,
        PA_SAMPLE_FLOAT32NE,
        PA_SAMPLE_INVALID
    };
    pa_do_volume_func_t svolume_ref_funcs[PA_SAMPLE_MAX][3];

    uint8_t samples_orig[4*N_SAMPLES*MAX_CHANNELS];
    uint8_t samples_ref[4*N_SAMPLES*MAX_CHANNELS];
    uint8_t samples_asm[4*N_SAMPLES*MAX_CHANNELS];
    uint8_t samples_orc[4*N_SAMPLES*MAX_CHANNELS];
    int32_t volumes[MAX_CHANNELS + PADDING];

    pa_sample_format_t format;
    struct pa_cpu_info cpu_info;
    unsigned f, channels, fixed_volume;
    int ret = 0;

    /* Store reference and optimized functions in a table */
    for (f = 0; (format = formats[f]) != PA_SAMPLE_INVALID; f++)
        svolume_ref_funcs[format][0] = pa_get_volume_func(format);

    pa_log_set_level(PA_LOG_ERROR);
    cpu_info.cpu_type = PA_CPU_UNDEFINED;
    if (pa_cpu_init_arm(&(cpu_info.flags.arm)))
        cpu_info.cpu_type = PA_CPU_ARM;
    if (pa_cpu_init_x86(&(cpu_info.flags.x86)))
        cpu_info.cpu_type = PA_CPU_X86;
    for (f = 0; (format = formats[f]) != PA_SAMPLE_INVALID; f++)
        svolume_ref_funcs[format][1] = pa_get_volume_func(format);

    pa_cpu_init_orc(cpu_info);
    pa_log_set_level(PA_LOG_DEBUG);
    for (f = 0; (format = formats[f]) != PA_SAMPLE_INVALID; f++)
        svolume_ref_funcs[format][2] = pa_get_volume_func(format);

    /* Testing correctness of the optimized functions */
    for (f = 0; (format = formats[f]) != PA_SAMPLE_INVALID; f++) {
        generate_random_samples(samples_orig, format, N_SAMPLES);

        for (fixed_volume = 0; fixed_volume <= 1; fixed_volume++) {
            for (channels = 1; channels <= MAX_CHANNELS; channels++) {
                int fail_asm, fail_orc;

                printf("checking %s - %d channels - %s volume\n", pa_sample_format_to_string(format), channels, fixed_volume ? "fixed" : "random");
                set_channel_volumes(volumes, channels, fixed_volume, format);

                /* func_ref */
                memcpy(samples_ref, samples_orig, sizeof(samples_orig));
                svolume_ref_funcs[format][0](samples_ref, volumes, channels, pa_sample_size_of_format(format) * N_SAMPLES * channels);
                /* func_asm */
                memcpy(samples_asm, samples_orig, sizeof(samples_orig));
                svolume_ref_funcs[format][1](samples_asm, volumes, channels, pa_sample_size_of_format(format) * N_SAMPLES * channels);
                fail_asm = compare_samples("asm", format, samples_orig, samples_ref, samples_asm, volumes, channels);
                /* func_orc */
                memcpy(samples_orc, samples_orig, sizeof(samples_orig));
                svolume_ref_funcs[format][2](samples_orc, volumes, channels, pa_sample_size_of_format(format) * N_SAMPLES * channels);
                fail_orc = compare_samples("orc", format, samples_orig, samples_ref, samples_orc, volumes, channels);

                if (fail_asm || fail_orc) {
                    printf("%-12s: ", "volumes");
                    dump_n_samples(volumes, (format == PA_SAMPLE_FLOAT32LE || format == PA_SAMPLE_FLOAT32BE) ? PA_SAMPLE_FLOAT32NE : PA_SAMPLE_S32NE, channels, 1, FALSE);
                    dump_channel_samples("orig", samples_orig, format, PA_MIN(N_SAMPLES, 20), channels);
                    dump_channel_samples("ref", samples_ref, format, PA_MIN(N_SAMPLES, 20), channels);
                    if (fail_asm)
                        dump_channel_samples("asm", samples_asm, format, PA_MIN(N_SAMPLES, 20), channels);
                    if (fail_orc)
                        dump_channel_samples("orc", samples_orc, format, PA_MIN(N_SAMPLES, 20), channels);
                }
                ret += fail_asm + fail_orc;
            }
        }
    }

    /* Testing performance of the optimized functions */
    for (f = 0; (format = formats[f]) != PA_SAMPLE_INVALID; f++) {
        generate_random_samples(samples_orig, format, N_SAMPLES);

        for (fixed_volume = 0; fixed_volume <= 1; fixed_volume++) {
            for (channels = 1; channels <= MAX_CHANNELS; channels++) {
                printf("\nchecking %s - %d channels - %s volume\n", pa_sample_format_to_string(format), channels, fixed_volume ? "fixed" : "random");
                set_channel_volumes(volumes, channels, fixed_volume, format);

                do_svolume_perf_test(svolume_ref_funcs[format][0], "ref", samples_orig, samples_ref, volumes, channels, pa_sample_size_of_format(format) * N_SAMPLES * channels);
                if (svolume_ref_funcs[format][1] != svolume_ref_funcs[format][0])
                    do_svolume_perf_test(svolume_ref_funcs[format][1], "asm", samples_orig, samples_asm, volumes, channels, pa_sample_size_of_format(format) * N_SAMPLES * channels);
                if (svolume_ref_funcs[format][2] != svolume_ref_funcs[format][1])
                    do_svolume_perf_test(svolume_ref_funcs[format][2], "orc", samples_orig, samples_orc, volumes, channels, pa_sample_size_of_format(format) * N_SAMPLES * channels);
            }
        }
    }

    return ret;
}
