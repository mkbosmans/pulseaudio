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
#include <pulsecore/sample-util.h>

#include <tests/sample-test-util.h>

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
    unsigned i;

    pa_assert_se(r = pa_memblock_new(pool, pa_frame_size(ss) * 10));
    d = pa_memblock_acquire(r);

    switch (ss->format) {

        case PA_SAMPLE_U8:
        case PA_SAMPLE_ULAW:
        case PA_SAMPLE_ALAW: {
            static const uint8_t u8_samples[] = {
                0x00, 0xFF, 0x7F, 0x80, 0x9f,
                0x3f, 0x01, 0xF0, 0x20, 0x21
            };

            memcpy(d, u8_samples, sizeof(u8_samples));
            break;
        }

        case PA_SAMPLE_S16NE:
        case PA_SAMPLE_S16RE: {
            static const uint16_t u16_samples[] = {
                0x0000, 0xFFFF, 0x7FFF, 0x8000, 0x9fff,
                0x3fff, 0x0001, 0xF000, 0x0020, 0x0021
            };

            memcpy(d, u16_samples, sizeof(u16_samples));
            break;
        }

        case PA_SAMPLE_S24_32NE:
        case PA_SAMPLE_S24_32RE:
        case PA_SAMPLE_S32NE:
        case PA_SAMPLE_S32RE: {
            static const uint32_t u32_samples[] = {
                0x00000001, 0xFFFF0002, 0x7FFF0003, 0x80000004, 0x9fff0005,
                0x3fff0006, 0x00010007, 0xF0000008, 0x00200009, 0x0021000A
            };

            memcpy(d, u32_samples, sizeof(u32_samples));
            break;
        }

        case PA_SAMPLE_S24NE:
        case PA_SAMPLE_S24RE: {
            /* Need to be on a byte array because they are not aligned */
            static const uint8_t u24_samples[] = {
                0x00, 0x00, 0x01,
                0xFF, 0xFF, 0x02,
                0x7F, 0xFF, 0x03,
                0x80, 0x00, 0x04,
                0x9f, 0xff, 0x05,
                0x3f, 0xff, 0x06,
                0x01, 0x00, 0x07,
                0xF0, 0x00, 0x08,
                0x20, 0x00, 0x09,
                0x21, 0x00, 0x0A
            };

            memcpy(d, u24_samples, sizeof(u24_samples));
            break;
        }

        case PA_SAMPLE_FLOAT32NE:
        case PA_SAMPLE_FLOAT32RE: {
            float *u = d;
            static const float float_samples[] = {
                0.0f, -1.0f, 1.0f, 4711.0f, 0.222f,
                0.33f, -.3f, 99.0f, -0.555f, -.123f
            };

            if (ss->format == PA_SAMPLE_FLOAT32RE) {
                for (i = 0; i < 10; i++)
                    u[i] = PA_FLOAT32_SWAP(float_samples[i]);
            } else
              memcpy(d, float_samples, sizeof(float_samples));

            break;
        }

        default:
            pa_assert_not_reached();
    }

    pa_memblock_release(r);

    return r;
}

int main(int argc, char *argv[]) {
    pa_mempool *pool;
    pa_sample_spec a;
    pa_cvolume v;

    if (!getenv("MAKE_CHECK"))
        pa_log_set_level(PA_LOG_DEBUG);

    pa_assert_se(pool = pa_mempool_new(FALSE, 0));

    a.channels = 1;
    a.rate = 44100;

    v.channels = a.channels;
    v.values[0] = pa_sw_volume_from_linear(0.9);

    for (a.format = 0; a.format < PA_SAMPLE_MAX; a.format ++) {
        pa_memchunk i, j, k;
        pa_mix_info m[2];
        void *ptr;

        pa_log_debug("=== mixing: %s\n", pa_sample_format_to_string(a.format));

        /* Generate block */
        i.memblock = generate_block(pool, &a);
        i.length = pa_memblock_get_length(i.memblock);
        i.index = 0;

        dump_block(&a, &i);

        /* Make a copy */
        j = i;
        pa_memblock_ref(j.memblock);
        pa_memchunk_make_writable(&j, 0);

        /* Adjust volume of the copy */
        pa_volume_memchunk(&j, &a, &v);

        dump_block(&a, &j);

        m[0].chunk = i;
        m[0].volume.values[0] = PA_VOLUME_NORM;
        m[0].volume.channels = a.channels;
        m[1].chunk = j;
        m[1].volume.values[0] = PA_VOLUME_NORM;
        m[1].volume.channels = a.channels;

        k.memblock = pa_memblock_new(pool, i.length);
        k.length = i.length;
        k.index = 0;

        ptr = (uint8_t*) pa_memblock_acquire(k.memblock) + k.index;
        pa_mix(m, 2, ptr, k.length, &a, NULL, FALSE);
        pa_memblock_release(k.memblock);

        dump_block(&a, &k);

        pa_memblock_unref(i.memblock);
        pa_memblock_unref(j.memblock);
        pa_memblock_unref(k.memblock);
    }

    pa_mempool_free(pool);

    return 0;
}
