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
