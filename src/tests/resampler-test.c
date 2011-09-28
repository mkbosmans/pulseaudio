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
#include <getopt.h>
#include <locale.h>

#include <pulse/rtclock.h>
#include <pulse/sample.h>
#include <pulse/timeval.h>
#include <pulse/volume.h>

#include <pulsecore/i18n.h>
#include <pulsecore/log.h>
#include <pulsecore/resampler.h>
#include <pulsecore/macro.h>
#include <pulsecore/endianmacros.h>
#include <pulsecore/memblock.h>
#include <pulsecore/sample-util.h>
#include <pulsecore/core-util.h>

#include <tests/sample-test-util.h>

static void dump_block(const char *label, const pa_sample_spec *ss, const pa_memchunk *chunk) {
    void *d;

    if (getenv("MAKE_CHECK"))
        return;
    printf("%s:  \t", label);

    d = pa_memblock_acquire(chunk->memblock);
    dump_n_samples(d, ss->format, chunk->length / pa_frame_size(ss), 1, TRUE);
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

static void help(const char *argv0) {
    printf(_("%s [options]\n\n"
             "-h, --help                            Show this help\n"
             "-v, --verbose                         Print debug messages\n"
             "      --from-rate=SAMPLERATE          From sample rate in Hz (defaults to 44100)\n"
             "      --from-format=SAMPLEFORMAT      From sample type (defaults to s16le)\n"
             "      --from-channels=CHANNELS        From number of channels (defaults to 1)\n"
             "      --to-rate=SAMPLERATE            To sample rate in Hz (defaults to 44100)\n"
             "      --to-format=SAMPLEFORMAT        To sample type (defaults to s16le)\n"
             "      --to-channels=CHANNELS          To number of channels (defaults to 1)\n"
             "      --resample-method=METHOD        Resample method (defaults to auto)\n"
             "      --seconds=SECONDS               From stream duration (defaults to 60)\n"
             "\n"
             "If the formats are not specified, the test performs all formats combinations,\n"
             "back and forth.\n"
             "\n"
             "Sample type must be one of s16le, s16be, u8, float32le, float32be, ulaw, alaw,\n"
             "32le, s32be (defaults to s16ne)\n"
             "\n"
             "See --dump-resample-methods for possible values of resample methods.\n"),
             argv0);
}

enum {
    ARG_VERSION = 256,
    ARG_FROM_SAMPLERATE,
    ARG_FROM_SAMPLEFORMAT,
    ARG_FROM_CHANNELS,
    ARG_TO_SAMPLERATE,
    ARG_TO_SAMPLEFORMAT,
    ARG_TO_CHANNELS,
    ARG_SECONDS,
    ARG_RESAMPLE_METHOD,
    ARG_DUMP_RESAMPLE_METHODS
};

static void dump_resample_methods(void) {
    int i;

    for (i = 0; i < PA_RESAMPLER_MAX; i++)
        if (pa_resample_method_supported(i))
            printf("%s\n", pa_resample_method_to_string(i));

}

int main(int argc, char *argv[]) {
    pa_mempool *pool = NULL;
    pa_sample_spec a, b;
    int ret = 1, c;
    pa_bool_t all_formats = TRUE;
    pa_resample_method_t method;
    int seconds;

    static const struct option long_options[] = {
        {"help",                  0, NULL, 'h'},
        {"verbose",               0, NULL, 'v'},
        {"version",               0, NULL, ARG_VERSION},
        {"from-rate",             1, NULL, ARG_FROM_SAMPLERATE},
        {"from-format",           1, NULL, ARG_FROM_SAMPLEFORMAT},
        {"from-channels",         1, NULL, ARG_FROM_CHANNELS},
        {"to-rate",               1, NULL, ARG_TO_SAMPLERATE},
        {"to-format",             1, NULL, ARG_TO_SAMPLEFORMAT},
        {"to-channels",           1, NULL, ARG_TO_CHANNELS},
        {"seconds",               1, NULL, ARG_SECONDS},
        {"resample-method",       1, NULL, ARG_RESAMPLE_METHOD},
        {"dump-resample-methods", 0, NULL, ARG_DUMP_RESAMPLE_METHODS},
        {NULL,                    0, NULL, 0}
    };

    setlocale(LC_ALL, "");
#ifdef ENABLE_NLS
    bindtextdomain(GETTEXT_PACKAGE, PULSE_LOCALEDIR);
#endif

    pa_log_set_level(PA_LOG_WARN);
    if (!getenv("MAKE_CHECK"))
        pa_log_set_level(PA_LOG_INFO);

    pa_assert_se(pool = pa_mempool_new(FALSE, 0));

    a.channels = b.channels = 1;
    a.rate = b.rate = 44100;
    a.format = b.format = PA_SAMPLE_S16LE;

    method = PA_RESAMPLER_AUTO;
    seconds = 60;

    while ((c = getopt_long(argc, argv, "hv", long_options, NULL)) != -1) {

        switch (c) {
            case 'h' :
                help(argv[0]);
                ret = 0;
                goto quit;

            case 'v':
                pa_log_set_level(PA_LOG_DEBUG);
                break;

            case ARG_VERSION:
                printf(_("%s %s\n"), argv[0], PACKAGE_VERSION);
                ret = 0;
                goto quit;

            case ARG_DUMP_RESAMPLE_METHODS:
                dump_resample_methods();
                ret = 0;
                goto quit;

            case ARG_FROM_CHANNELS:
                a.channels = (uint8_t) atoi(optarg);
                break;

            case ARG_FROM_SAMPLEFORMAT:
                a.format = pa_parse_sample_format(optarg);
                all_formats = FALSE;
                break;

            case ARG_FROM_SAMPLERATE:
                a.rate = (uint32_t) atoi(optarg);
                break;

            case ARG_TO_CHANNELS:
                b.channels = (uint8_t) atoi(optarg);
                break;

            case ARG_TO_SAMPLEFORMAT:
                b.format = pa_parse_sample_format(optarg);
                all_formats = FALSE;
                break;

            case ARG_TO_SAMPLERATE:
                b.rate = (uint32_t) atoi(optarg);
                break;

            case ARG_SECONDS:
                seconds = atoi(optarg);
                break;

            case ARG_RESAMPLE_METHOD:
                if (*optarg == '\0' || pa_streq(optarg, "help")) {
                    dump_resample_methods();
                    ret = 0;
                    goto quit;
                }
                method = pa_parse_resample_method(optarg);
                break;

            default:
                goto quit;
        }
    }

    ret = 0;
    pa_assert_se(pool = pa_mempool_new(FALSE, 0));

    if (!all_formats) {

        pa_resampler *resampler;
        pa_memchunk i, j;
        pa_usec_t ts;

        pa_log_debug(_("Compilation CFLAGS: %s"), PA_CFLAGS);
        pa_log_debug(_("=== %d seconds: %d Hz %d ch (%s) -> %d Hz %d ch (%s)"), seconds,
                   a.rate, a.channels, pa_sample_format_to_string(a.format),
                   b.rate, b.channels, pa_sample_format_to_string(b.format));

        ts = pa_rtclock_now();
        pa_assert_se(resampler = pa_resampler_new(pool, &a, NULL, &b, NULL, method, 0));
        pa_log_info("init: %llu", (long long unsigned)(pa_rtclock_now() - ts));

        i.memblock = pa_memblock_new(pool, pa_usec_to_bytes(1*PA_USEC_PER_SEC, &a));

        ts = pa_rtclock_now();
        i.length = pa_memblock_get_length(i.memblock);
        i.index = 0;
        while (seconds--) {
            pa_resampler_run(resampler, &i, &j);
            pa_memblock_unref(j.memblock);
        }
        pa_log_info("resampling: %llu", (long long unsigned)(pa_rtclock_now() - ts));
        pa_memblock_unref(i.memblock);

        pa_resampler_free(resampler);

        goto quit;
    }

    for (a.format = 0; a.format < PA_SAMPLE_MAX; a.format ++) {
        for (b.format = 0; b.format < PA_SAMPLE_MAX; b.format ++) {
            pa_resampler *forth, *back;
            pa_memchunk i, j, k;

            pa_log_debug("=== %s -> %s -> %s -> /2",
                       pa_sample_format_to_string(a.format),
                       pa_sample_format_to_string(b.format),
                       pa_sample_format_to_string(a.format));

            pa_assert_se(forth = pa_resampler_new(pool, &a, NULL, &b, NULL, method, 0));
            pa_assert_se(back = pa_resampler_new(pool, &b, NULL, &a, NULL, method, 0));

            i.memblock = generate_block(pool, &a);
            i.length = pa_memblock_get_length(i.memblock);
            i.index = 0;
            pa_resampler_run(forth, &i, &j);
            pa_resampler_run(back, &j, &k);

            dump_block("before", &a, &i);
            dump_block("after", &b, &j);
            dump_block("reverse", &a, &k);

            pa_memblock_unref(i.memblock);
            pa_memblock_unref(j.memblock);
            pa_memblock_unref(k.memblock);

            pa_resampler_free(forth);
            pa_resampler_free(back);
        }
    }

 quit:
    if (pool)
        pa_mempool_free(pool);

    return ret;
}
