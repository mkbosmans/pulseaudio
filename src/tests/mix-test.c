/***
  This file is part of PulseAudio.

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

#include <pulse/sample.h>
#include <pulse/volume.h>

#include <pulsecore/macro.h>
#include <pulsecore/endianmacros.h>
#include <pulsecore/memblock.h>
#include <pulsecore/memchunk.h>
#include <pulsecore/mix.h>
#include <pulsecore/sample-util.h>
#include <pulsecore/cpu.h>
#include <pulsecore/cpu-orc.h>

#include <tests/sample-test-util.h>

#define MAX_CHANNELS 3
#define MAX_STREAMS 5
#define N_SAMPLES 1000

static void dump_block(const pa_sample_spec *ss, const pa_memchunk *chunk) {
    void *d;

    if (getenv("MAKE_CHECK"))
        return;

    d = pa_memblock_acquire(chunk->memblock);
    dump_n_samples(d, ss->format, chunk->length / pa_frame_size(ss), 1, FALSE);
    pa_memblock_release(chunk->memblock);
}

static pa_memblock* generate_block(pa_mempool *pool, const pa_sample_spec *ss) {
    pa_memblock *r;
    void *d;

    pa_assert_se(r = pa_memblock_new(pool, pa_frame_size(ss) * 10));
    d = pa_memblock_acquire(r);
    generate_samples(d, ss->format, 10);
    pa_memblock_release(r);

    return r;
}

/* Verify correctness of pa_mix implementation */
static int test_correctness() {
    pa_mempool *pool;
    pa_memchunk i, j, k;
    pa_sample_spec ss;
    pa_cvolume v;
    pa_mix_info m[2];
    void *ptr;
    int ret = 0;

    if (!getenv("MAKE_CHECK"))
        pa_log_set_level(PA_LOG_DEBUG);

    pa_assert_se(pool = pa_mempool_new(FALSE, 0));

    ss.channels = v.channels = 1;
    ss.rate = 48000;

    for (ss.format = 0; ss.format < PA_SAMPLE_MAX; ss.format++) {
        pa_log_debug("=== mixing: %s\n", pa_sample_format_to_string(a.format));

        /* Generate block */
        i.memblock = generate_block(pool, &ss);
        i.length = pa_memblock_get_length(i.memblock);
        i.index = 0;

        /* Make a copy and adjust volume */
        j = i;
        v.values[0] = pa_sw_volume_from_linear(0.3);
        pa_memblock_ref(j.memblock);
        pa_memchunk_make_writable(&j, 0);
        pa_volume_memchunk(&j, &ss, &v);

        /* Mix the two blocks */
        m[0].chunk = i;
        m[0].volume.values[0] = pa_sw_volume_from_linear(0.5);
        m[0].volume.channels = ss.channels;
        m[1].chunk = j;
        m[1].volume.values[0] = pa_sw_volume_from_linear(2.5);
        m[1].volume.channels = ss.channels;
        v.values[0] = pa_sw_volume_from_linear(0.8);

        k.memblock = pa_memblock_new(pool, i.length);
        k.length = i.length;
        k.index = 0;
        ptr = (uint8_t*) pa_memblock_acquire(k.memblock) + k.index;
        pa_mix(m, 2, ptr, k.length, &ss, &v, FALSE);
        pa_memblock_release(k.memblock);

        /* Compare results
         * Result should be the same as the original: (1.0 * 0.5 + 0.3 * 2.5) * 0.8 = 1.0
         */
        dump_block(&ss, &i);
        dump_block(&ss, &k);

        pa_memblock_unref(i.memblock);
        pa_memblock_unref(j.memblock);
        pa_memblock_unref(k.memblock);
    }

    pa_mempool_free(pool);

    return ret;
}

int main(int argc, char *argv[]) {
    int ret = 0;

    pa_mix_func_t mix_funcs[2][PA_SAMPLE_MAX];
    struct pa_cpu_info cpu_info;
    pa_sample_format_t format;
    unsigned channels;

    pa_log_set_level(PA_LOG_DEBUG);

    ret += test_correctness();

    /* Measure performance of pa_mix implementations */
    pa_log("\n\nPerformance test");

    for (format = 0; format < PA_SAMPLE_MAX; format++)
        mix_funcs[0][format] = pa_get_mix_func(format);
    pa_log_set_level(PA_LOG_ERROR);
    cpu_info.cpu_type = PA_CPU_UNDEFINED;
    if (pa_cpu_init_arm(&(cpu_info.flags.arm)))
        cpu_info.cpu_type = PA_CPU_ARM;
    if (pa_cpu_init_x86(&(cpu_info.flags.x86)))
        cpu_info.cpu_type = PA_CPU_X86;
    pa_cpu_init_orc(cpu_info);
    pa_log_set_level(PA_LOG_DEBUG);
    for (format = 0; format < PA_SAMPLE_MAX; format++)
        mix_funcs[1][format] = pa_get_mix_func(format);

    for (format = 3; format < PA_SAMPLE_MAX-7; format++) {
        static int32_t volumes_i[MAX_STREAMS][PA_CHANNELS_MAX] = {{0x1000, 0x2000, 0x3000},
                                                                  {0x2000, 0x3000, 0x4000},
                                                                  {0x3000, 0x4000, 0x5000},
                                                                  {0x4000, 0x5000, 0x6000},
                                                                  {0x5000, 0x6000, 0x7000}};
        static float volumes_f[MAX_STREAMS][PA_CHANNELS_MAX] = {{0.1, 0.2, 0.3},
                                                                {0.2, 0.3, 0.4},
                                                                {0.3, 0.4, 0.5},
                                                                {0.4, 0.5, 0.6},
                                                                {0.5, 0.6, 0.7}};
        PA_DECLARE_ALIGNED(8, uint8_t, dest[4*MAX_CHANNELS*N_SAMPLES]);
        uint8_t src[4*MAX_CHANNELS*N_SAMPLES*MAX_STREAMS];
        const void *src_ptr[MAX_STREAMS];

        generate_random_samples(src, format, MAX_CHANNELS * N_SAMPLES * MAX_STREAMS);

        for (channels = 1; channels <= MAX_CHANNELS; channels++) {
            printf("%12s\t%u", pa_sample_format_to_string(format), channels);

            for (unsigned nstreams = 1; nstreams <= MAX_STREAMS; nstreams++) {
                void *volumes;
                float unused PA_GCC_UNUSED, min;

                if (format == PA_SAMPLE_FLOAT32LE || format == PA_SAMPLE_FLOAT32BE)
                    volumes = volumes_f;
                else
                    volumes = volumes_i;

                INIT_TIMED_TEST(20)
                START_TIMED_TEST(500)
                    for (unsigned n = 0; n < nstreams; n++)
                        src_ptr[n] = src + n * 4 * MAX_CHANNELS * N_SAMPLES;
                    mix_funcs[0][format](volumes, nstreams, channels, src_ptr, dest, pa_sample_size_of_format(format) * channels * N_SAMPLES);
                END_TIMED_TEST(unused, min, unused, unused)
                printf("\t%8.1f", min);

                if (mix_funcs[1][format] == mix_funcs[0][format])
                    continue;

                INIT_TIMED_TEST(20)
                START_TIMED_TEST(500)
                    for (unsigned n = 0; n < nstreams; n++)
                        src_ptr[n] = src + n * 4 * MAX_CHANNELS * N_SAMPLES;
                    mix_funcs[1][format](volumes, nstreams, channels, src_ptr, dest, pa_sample_size_of_format(format) * channels * N_SAMPLES);
                END_TIMED_TEST(unused, min, unused, unused)
                printf("%6.1f", min);
            }

            printf("\n");
        }
    }

    return ret;
}
