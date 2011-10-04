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

    pa_assert_se(r = pa_memblock_new(pool, pa_frame_size(ss) * 10));
    d = pa_memblock_acquire(r);
    generate_samples(d, ss->format, 10);
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
