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
#include <string.h>

#include <pulsecore/endianmacros.h>
#include <pulsecore/macro.h>
#include <pulsecore/sconv.h>
#include <pulsecore/cpu.h>
#include <pulsecore/cpu-orc.h>

#include <tests/sample-test-util.h>

#define N_SAMPLES 20000
#define N_REPEAT 500

struct sconv_table {
    pa_sample_format_t format;
    int int_int_tolerance, float_int_tolerance;
    float int_float_tolerance, float_float_tolerance;
};

static int16_t i_source[N_SAMPLES], i_dest[N_SAMPLES];
static float f_source[N_SAMPLES], f_dest[N_SAMPLES];
static uint8_t intermediate[4*N_SAMPLES];

/* Compare samples before and after the conversion and print the samples if there is a significant difference */
static int compare_dest_to_source(const char *sf, pa_sample_format_t f, int tolerance_i, float tolerance_f) {
    int max_error_i;
    float max_error_f;
    int fail_i, fail_f;

    max_error_i = compare_samples(i_dest, i_source, PA_SAMPLE_S16NE, N_SAMPLES);
    max_error_f = (float) compare_samples(f_dest, f_source, PA_SAMPLE_FLOAT32NE, N_SAMPLES) / (float) 0x1000000;

    fail_i = (max_error_i > tolerance_i);
    fail_f = (max_error_f > tolerance_f);

    printf("%s\t%10s\t\t%s\t%s\t\t[errors:%5d  %.8f]\n",
        sf, pa_sample_format_to_string(f),
        (fail_i ? "FAIL" : "OK"), (fail_f ? "FAIL" : "OK"),
        max_error_i, max_error_f);

    if (fail_i || fail_f) {
        printf("    %-12s: ", "input s16");
        dump_n_samples(i_source, PA_SAMPLE_S16NE, PA_MIN(N_SAMPLES, 16), 1, TRUE);
        printf("    %-12s: ", "output s16");
        dump_n_samples(i_dest, PA_SAMPLE_S16NE, PA_MIN(N_SAMPLES, 16), 1, TRUE);
        printf("    %-12s: ", "input float");
        dump_n_samples(f_source, PA_SAMPLE_FLOAT32NE, PA_MIN(N_SAMPLES, 16), 1, TRUE);
        printf("    %-12s: ", "output float");
        dump_n_samples(f_dest, PA_SAMPLE_FLOAT32NE, PA_MIN(N_SAMPLES, 16), 1, TRUE);
        printf("    %-12s: ", "intermediate");
        dump_n_samples(intermediate, f, PA_MIN(N_SAMPLES, 16), 1, TRUE);
    }

    return fail_i + fail_f;
}

static int do_sconv_test(pa_sample_format_t s_format, pa_sample_format_t d_format, pa_convert_func_t ref_func, pa_convert_func_t opt_func) {
    static uint8_t source[4*N_SAMPLES];
    static uint8_t dest[4*N_SAMPLES];
    static uint8_t ref[4*N_SAMPLES];

    double t_ref, t_opt;
    int ret = 0;

    printf("      %9s -> %9s:\t", pa_sample_format_to_string(s_format), pa_sample_format_to_string(d_format));

    generate_random_samples(source, s_format, N_SAMPLES);

    /* Measure reference and optimized function */
    INIT_TIMED_TEST(4)
    START_TIMED_TEST(N_REPEAT)
        ref_func(N_SAMPLES, source, ref);
    END_TIMED_TEST(t_ref, t_ref, t_ref, t_ref)

    INIT_TIMED_TEST(4)
    START_TIMED_TEST(N_REPEAT)
        opt_func(N_SAMPLES, source, dest);
    END_TIMED_TEST(t_opt, t_opt, t_opt, t_opt)

    if (ref_func == opt_func) {
        printf("N/A");
    } else {
        int sample_error = compare_samples(dest, ref, d_format, N_SAMPLES);
        if (sample_error == 0) {
            printf("OK");
        } else if (sample_error == 1) {
            printf("~OK");
        } else {
            printf("FAIL");
            ret++;
        }
    }
    printf("\t\t[%6.1f ms,%6.1f ms]\n", t_ref, t_opt);

    return ret;
}

int main(int argc, char *argv[]) {

    static const struct sconv_table convs[] = {
        {PA_SAMPLE_U8,        255, 255, 255.0/0x8000, 256.0/0x8000},
        {PA_SAMPLE_ALAW,      512, 516, 512.0/0x8000, 516.0/0x8000},
        {PA_SAMPLE_ULAW,      644, 644, 644.0/0x8000, 644.0/0x8000},
        {PA_SAMPLE_S16LE,       0,   0,          0.0,  1.0/0x10000},
        {PA_SAMPLE_S16BE,       0,   0,          0.0,  1.0/0x10000},
        {PA_SAMPLE_FLOAT32LE,   0,   0,          0.0,          0.0},
        {PA_SAMPLE_FLOAT32BE,   0,   0,          0.0,          0.0},
        {PA_SAMPLE_S32LE,       0,   1,          0.0,         1e-8},
        {PA_SAMPLE_S32BE,       0,   1,          0.0,         1e-8},
        {PA_SAMPLE_S24LE,       0,   1,          0.0,         1e-6},
        {PA_SAMPLE_S24BE,       0,   1,          0.0,         1e-6},
        {PA_SAMPLE_S24_32LE,    0,   1,          0.0,         1e-6},
        {PA_SAMPLE_S24_32BE,    0,   1,          0.0,         1e-6},
        {PA_SAMPLE_INVALID,     0,   0,          0.0,          0.0}
    };

    int ret = 0;
    unsigned c;
    struct pa_cpu_info cpu_info;
    pa_sample_format_t f;
    pa_convert_func_t sconv_ref_funcs[PA_SAMPLE_MAX][4];

    /* First check for sanity of plain c sconv implementations.
     *
     * This is done by generating random samples and converting that from one of the working
     * formats (s16ne or float32ne) to the intermediate format.  As no reference implementation
     * is available, the reverse conversion is also done, the result of the two conversions
     * should be equal the original random samples again.
     * Also a conversion between the working formats going through the intermediate format is
     * compared to the result of a direct conversion between the two working formats.
     * As some of the formats are smaller than the working formats, some tolerance is allowed
     * when comparing the results.  This tolerance also accounts for rounding errors.
     */
    pa_log("*** Testing plain c sconv implementations ***");
    printf("FROM\tINTERMEDIATE\tTO:\tint16\tfloat\n");

    /* Generate random int16 samples and convert directly to float, as reference */
    generate_random_samples(i_source, PA_SAMPLE_S16NE, N_SAMPLES);
    pa_get_convert_from_s16ne_function(PA_SAMPLE_FLOAT32NE)(N_SAMPLES, i_source, f_source);

    for (c = 0; (f = convs[c].format) != PA_SAMPLE_INVALID; c++) {
        pa_get_convert_from_s16ne_function(f)(N_SAMPLES, i_source, intermediate);
        pa_get_convert_to_s16ne_function(f)(N_SAMPLES, intermediate, i_dest);
        pa_get_convert_to_float32ne_function(f)(N_SAMPLES, intermediate, f_dest);

        ret += compare_dest_to_source("int16", f, convs[c].int_int_tolerance, convs[c].int_float_tolerance);
    }

    /* Generate random float samples and convert directly to int16, as reference */
    generate_random_samples(f_source, PA_SAMPLE_FLOAT32NE, N_SAMPLES);
    pa_get_convert_from_float32ne_function(PA_SAMPLE_S16NE)(N_SAMPLES, f_source, i_source);

    for (c = 0; (f = convs[c].format) != PA_SAMPLE_INVALID; c++) {
        pa_get_convert_from_float32ne_function(f)(N_SAMPLES, f_source, intermediate);
        pa_get_convert_to_float32ne_function(f)(N_SAMPLES, intermediate, f_dest);
        pa_get_convert_to_s16ne_function(f)(N_SAMPLES, intermediate, i_dest);

        ret += compare_dest_to_source("float", f, convs[c].float_int_tolerance, convs[c].float_float_tolerance);
    }

    /* Now we can check the correctness and performance of the optimized implementations.
     *
     * For each possible conversion the optimized version should be bit-for-bit identical to the
     * basic implementation.  For s16 results a sample error of 1 is allowed.
     */
    pa_log("*** Testing optimized sconv implementations ***");
    printf("          FROM           TO\tRESULT\t\t reference  optimized\n");

    /* Store reference c implementation before loading optimized functions */
    for (c = 0; (f = convs[c].format) != PA_SAMPLE_INVALID; c++) {
        sconv_ref_funcs[f][0] = pa_get_convert_to_s16ne_function(f);
        sconv_ref_funcs[f][1] = pa_get_convert_from_s16ne_function(f);
        sconv_ref_funcs[f][2] = pa_get_convert_to_float32ne_function(f);
        sconv_ref_funcs[f][3] = pa_get_convert_from_float32ne_function(f);
    }

    /* Load optimized sconv functions */
    cpu_info.cpu_type = PA_CPU_UNDEFINED;
    if (pa_cpu_init_x86(&(cpu_info.flags.x86)))
        cpu_info.cpu_type = PA_CPU_X86;
    if (pa_cpu_init_arm(&(cpu_info.flags.arm)))
        cpu_info.cpu_type = PA_CPU_ARM;
    pa_cpu_init_orc(cpu_info);

    /* Test optimized functions against reference c implementation */
    for (c = 0; (f = convs[c].format) != PA_SAMPLE_INVALID; c++) {
        printf("=== %s ===\n", pa_sample_format_to_string(f));
        ret += do_sconv_test(f, PA_SAMPLE_S16NE, sconv_ref_funcs[f][0], pa_get_convert_to_s16ne_function(f));
        ret += do_sconv_test(PA_SAMPLE_S16NE, f, sconv_ref_funcs[f][1], pa_get_convert_from_s16ne_function(f));
        ret += do_sconv_test(f, PA_SAMPLE_FLOAT32NE, sconv_ref_funcs[f][2], pa_get_convert_to_float32ne_function(f));
        ret += do_sconv_test(PA_SAMPLE_FLOAT32NE, f, sconv_ref_funcs[f][3], pa_get_convert_from_float32ne_function(f));
    }

    printf("\nNumber of tests failed: %d\n", ret);
    return ret;
}
