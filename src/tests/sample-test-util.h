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

#include <stdlib.h>
#include <math.h>

#include <pulse/timeval.h>

#include <pulsecore/core-rtclock.h>
#include <pulsecore/endianmacros.h>

static PA_GCC_UNUSED void dump_n_samples(const void *samples, pa_sample_format_t format, size_t n_samples, unsigned stride, pa_bool_t align_output) {

    switch (format) {

        case PA_SAMPLE_U8:
        case PA_SAMPLE_ULAW:
        case PA_SAMPLE_ALAW: {
            const uint8_t *u = samples;

            if (align_output) {
                for (; n_samples; n_samples--, u+=stride)
                    printf("      0x%02x ", *u);
            } else {
                for (; n_samples; n_samples--, u+=stride)
                    printf("0x%02x ", *u);
            }

            break;
        }

        case PA_SAMPLE_S16NE:
        case PA_SAMPLE_S16RE: {
            const uint16_t *u = samples;

            if (align_output) {
                for (; n_samples; n_samples--, u+=stride)
                    printf("    0x%04x ", *u);
            } else {
                for (; n_samples; n_samples--, u+=stride)
                    printf("0x%04x ", *u);
            }

            break;
        }

        case PA_SAMPLE_S32NE:
        case PA_SAMPLE_S32RE:
        case PA_SAMPLE_S24_32NE:
        case PA_SAMPLE_S24_32RE: {
            const uint32_t *u = samples;

            for (; n_samples; n_samples--, u+=stride)
                printf("0x%08x ", *u);

            break;
        }

        case PA_SAMPLE_S24NE:
        case PA_SAMPLE_S24RE: {
            const uint8_t *u = samples;

            if (align_output) {
                for (; n_samples; n_samples--, u+=3*stride)
                    printf("  0x%06x ", PA_READ24NE(u));
            } else {
                for (; n_samples; n_samples--, u+=3*stride)
                    printf("0x%06x ", PA_READ24NE(u));
            }

            break;
        }

        case PA_SAMPLE_FLOAT32NE:
        case PA_SAMPLE_FLOAT32RE: {
            const float *u = samples;

            if (align_output) {
                for (; n_samples; n_samples--, u+=stride)
                    printf("%10.6f ", format == PA_SAMPLE_FLOAT32NE ? *u : PA_FLOAT32_SWAP(*u));
            } else {
                for (; n_samples; n_samples--, u+=stride)
                    printf("%8.5f ", format == PA_SAMPLE_FLOAT32NE ? *u : PA_FLOAT32_SWAP(*u));
            }

            break;
        }

        default:
            pa_assert_not_reached();
    }

    printf("\n");
}

static PA_GCC_UNUSED void dump_channel_samples(const char *str, const void *samples, pa_sample_format_t format, size_t n_samples, unsigned nc) {
    for (unsigned c = 0; c < nc; c++) {
        printf("%-12s: ", c > 0 ? "" : str);
        dump_n_samples(((uint8_t *) samples)+c*pa_sample_size_of_format(format), format, n_samples, nc, FALSE);
    }
}

/* Generate fixed set of 10 samples */
static PA_GCC_UNUSED void generate_samples(void *samples, pa_sample_format_t format, size_t n_samples) {
    pa_assert(n_samples == 10);

    switch (format) {

        case PA_SAMPLE_U8:
        case PA_SAMPLE_ULAW:
        case PA_SAMPLE_ALAW: {
            static const uint8_t u8_samples[] = {
                0x00, 0xFF, 0x7F, 0x80, 0x9f,
                0x3f, 0x01, 0xF0, 0x20, 0x21
            };

            memcpy(samples, u8_samples, sizeof(u8_samples));
            break;
        }

        case PA_SAMPLE_S16NE:
        case PA_SAMPLE_S16RE: {
            static const uint16_t u16_samples[] = {
                0x0000, 0xFFFF, 0x7FFF, 0x8000, 0x9fff,
                0x3fff, 0x0001, 0xF000, 0x0020, 0x0021
            };

            memcpy(samples, u16_samples, sizeof(u16_samples));
            break;
        }

        case PA_SAMPLE_S32NE:
        case PA_SAMPLE_S32RE: {
            static const uint32_t u32_samples[] = {
                0x00000001, 0xFFFF0002, 0x7FFF0003, 0x80000004, 0x9fff0005,
                0x3fff0006, 0x00010007, 0xF0000008, 0x00200009, 0x0021000A
            };

            memcpy(samples, u32_samples, sizeof(u32_samples));
            break;
        }

        case PA_SAMPLE_S24_32NE:
        case PA_SAMPLE_S24_32RE: {
            static const uint32_t u32_samples[] = {
                0x000001, 0xFF0002, 0xFF0003, 0x000004, 0xff0005,
                0xff0006, 0x010007, 0x000008, 0x002009, 0x00210A
            };

            memcpy(samples, u32_samples, sizeof(u32_samples));
            break;
        }

        case PA_SAMPLE_S24NE:
        case PA_SAMPLE_S24RE: {
            /* Need to be on a byte array because they are not aligned */
            static const uint8_t u24_samples[] = {
                0x00,0x00,0x01,   0xFF,0xFF,0x02,   0x7F,0xFF,0x03,   0x80,0x00,0x04,   0x9f,0xff,0x05,
                0x3f,0xff,0x06,   0x01,0x00,0x07,   0xF0,0x00,0x08,   0x20,0x00,0x09,   0x21,0x00,0x0A
            };

            memcpy(samples, u24_samples, sizeof(u24_samples));
            break;
        }

        case PA_SAMPLE_FLOAT32NE:
        case PA_SAMPLE_FLOAT32RE: {
            static const float float_samples[] = {
                0.0f, -1.0f, 1.0f, 4711.0f, 0.222f,
                0.33f, -.3f, 99.0f, -0.555f, -.123f
            };

            if (format == PA_SAMPLE_FLOAT32RE) {
                float *u = samples;
                for (unsigned i = 0; i < 10; i++)
                    u[i] = PA_FLOAT32_SWAP(float_samples[i]);
            } else
              memcpy(samples, float_samples, sizeof(float_samples));

            break;
        }

        default:
            pa_assert_not_reached();
    }
}

/* Generate random samples */
static PA_GCC_UNUSED void generate_random_samples(void *samples, pa_sample_format_t format, size_t n_samples) {
    if (format == PA_SAMPLE_FLOAT32NE) {
        float *u = samples;

        for (; n_samples; n_samples--, u++)
            *u = (float) drand48() * 2.0 - 1.0;

    } else if (format == PA_SAMPLE_FLOAT32RE) {
        float *u = samples;

        for (; n_samples; n_samples--, u++) {
            float v = (float) drand48() * 2.0 - 1.0;
            *u = PA_FLOAT32_SWAP(v);
        }

    } else if (format == PA_SAMPLE_S16LE || format == PA_SAMPLE_S16BE) {
        uint16_t *u = samples;

        for (; n_samples; n_samples--, u++)
            *u = (uint16_t) lrand48();

    } else {
        uint8_t *u = samples;

        n_samples *= pa_sample_size_of_format(format);
        for (; n_samples; n_samples--, u++)
            *u = (uint8_t) lrand48();

    }
}

/* Compare two sets of samples and return the greatest difference */
static PA_GCC_UNUSED uint32_t compare_samples(const void *samples, const void *samples_ref, pa_sample_format_t format, size_t n_samples) {
    uint32_t max_error = 0;

    if (format == PA_SAMPLE_FLOAT32NE) {
        const float *u1 = samples, *u2 = samples_ref;
        float max_error_f = 0.0f;

        for (unsigned i = 0; i < n_samples; i++)
            max_error_f = PA_MAX(max_error_f, fabsf(u1[i] - u2[i]));

        max_error = (uint32_t) (max_error_f * (float) 0x1000000);

    } else if (format == PA_SAMPLE_S16NE) {
        const int16_t *u1 = samples, *u2 = samples_ref;

        for (unsigned i = 0; i < n_samples; i++)
            max_error = PA_MAX(max_error, (uint32_t) abs(u1[i] - u2[i]));

    } else if (format == PA_SAMPLE_S16RE) {
        const int16_t *u1 = samples, *u2 = samples_ref;

        for (unsigned i = 0; i < n_samples; i++)
            max_error = PA_MAX(max_error, (uint32_t) abs(PA_INT16_SWAP(u1[i]) - PA_INT16_SWAP(u2[i])));

    } else {
        /* Don't bother */
        if (memcmp(samples, samples_ref, pa_sample_size_of_format(format) * n_samples))
            max_error = 255;
    }

    return max_error;
}

/* Macros for timing the performance of code */
#define INIT_TIMED_TEST(n_outer)                                        \
{                                                                       \
    double TT_time, TT_n_outer = n_outer;                               \
    double TT_min = 1e9, TT_max = 0.0, TT_sum = 0.0, TT_sum2 = 0.0;     \
    struct timeval TT_t1, TT_t2;                                        \
    for (int TT_i_outer = 0; TT_i_outer < n_outer; TT_i_outer++) {

#define START_TIMED_TEST(n_inner)                                       \
        pa_rtclock_get(&TT_t1);                                         \
        for (int TT_i_inner = 0; TT_i_inner < n_inner; TT_i_inner++) {

#define END_TIMED_TEST(avg_var, min_var, max_var, stddev_var)           \
        }                                                               \
        pa_rtclock_get(&TT_t2);                                         \
        TT_time = (double) pa_timeval_diff(&TT_t1, &TT_t2) / 1000.0;    \
        TT_min = PA_MIN(TT_min, TT_time);                               \
        TT_max = PA_MAX(TT_max, TT_time);                               \
        TT_sum += TT_time;                                              \
        TT_sum2 += TT_time * TT_time;                                   \
    }                                                                   \
    stddev_var = sqrt(TT_sum2*TT_n_outer - TT_sum*TT_sum) / TT_n_outer; \
    min_var = TT_min;                                                   \
    max_var = TT_max;                                                   \
    avg_var = TT_sum / TT_n_outer;                                      \
}
