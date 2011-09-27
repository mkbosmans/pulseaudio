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
