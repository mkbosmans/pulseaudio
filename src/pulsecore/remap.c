/***
  This file is part of PulseAudio.

  Copyright 2004-2006 Lennart Poettering
  Copyright 2009 Wim Taymans <wim.taymans@collabora.co.uk.com>

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

#include <string.h>

#include <pulse/sample.h>
#include <pulsecore/log.h>
#include <pulsecore/macro.h>

#include "remap.h"

static void remap_mono_to_stereo_c(pa_remap_t *m, void *dst, const void *src, unsigned n) {
    unsigned i;

    switch (*m->format) {
        case PA_SAMPLE_FLOAT32NE:
        {
            float *d, *s;

            d = (float *) dst;
            s = (float *) src;

            for (i = n >> 2; i; i--) {
                d[0] = d[1] = s[0];
                d[2] = d[3] = s[1];
                d[4] = d[5] = s[2];
                d[6] = d[7] = s[3];
                s += 4;
                d += 8;
            }
            for (i = n & 3; i; i--) {
                d[0] = d[1] = s[0];
                s++;
                d += 2;
            }
            break;
        }
        case PA_SAMPLE_S16NE:
        {
            int16_t *d, *s;

            d = (int16_t *) dst;
            s = (int16_t *) src;

            for (i = n >> 2; i; i--) {
                d[0] = d[1] = s[0];
                d[2] = d[3] = s[1];
                d[4] = d[5] = s[2];
                d[6] = d[7] = s[3];
                s += 4;
                d += 8;
            }
            for (i = n & 3; i; i--) {
                d[0] = d[1] = s[0];
                s++;
                d += 2;
            }
            break;
        }
        default:
            pa_assert_not_reached();
    }
}

static void remap_stereo_to_mono_c(pa_remap_t *m, void *dst, const void *src, unsigned n) {
    unsigned i;

    pa_assert(m->i_ss->channels == 2);
    pa_assert(m->o_ss->channels == 1);

    switch (*m->format) {
        case PA_SAMPLE_FLOAT32NE:
        {
            float *d = (float *)dst;
            float *s = (float *)src;

            for (i = n; i; i--, s += 2, d += 1)
                *d = (s[0] + s[1]) * 0.5;

            break;
        }
        case PA_SAMPLE_S16NE:
        {
            int16_t *d = (int16_t *)dst;
            int16_t *s = (int16_t *)src;

            for (i = n; i; i--, s += 2, d += 1)
                *d = (s[0] >> 1) + (s[1] >> 1);

            break;
        }
        default:
            pa_assert_not_reached();
    }
}

static void remap_stereo_to_nchannels_c(pa_remap_t *m, void *dst, const void *src, unsigned n) {
    unsigned oc, i;
    unsigned n_ic, n_oc;

    n_ic = m->i_ss->channels;
    n_oc = m->o_ss->channels;

    pa_assert(n_ic == 2);

    switch (*m->format) {
        case PA_SAMPLE_FLOAT32NE:
        {
            float *d, *s;

            for (oc = 0; oc < n_oc; oc++) {
                float vol_l, vol_r;

                vol_l = m->map_table_f[oc][0];
                vol_r = m->map_table_f[oc][1];

                d = (float *)dst + oc;
                s = (float *)src;

                if (vol_l <= 0.0 && vol_r <= 0.0) {
                    for (i = n; i > 0; i--, s += n_ic, d += n_oc)
                        *d = 0.0;
                } else if ((vol_l >= 1.0 && vol_r <= 0.0) ||            /* Exactly one of the channels */
                           (vol_r >= 1.0 && vol_l <= 0.0)) {
                    if (vol_l >= 1.0 && vol_r <= 0.0)                       /* left channel */
                        s += 0;
                    else if (vol_l <= 0.0 && vol_r >= 1.0)                  /* right channel */
                        s += 1;
                    for (i = n >> 2; i; i--) {
                        d[0*n_oc] = s[0];
                        d[1*n_oc] = s[2];
                        d[2*n_oc] = s[4];
                        d[3*n_oc] = s[6];
                        s += 4*n_ic;
                        d += 4*n_oc;
                    }
                    for (i = n & 3; i; i--) {
                        d[0] = s[0];
                        s += n_ic;
                        d += n_oc;
                    }
                } else if ((m->map_table_i[oc][0] == 0x08000 && m->map_table_i[oc][1] == 0x08000)) {
                    for (i = n; i; i--, s += n_ic, d += n_oc)
                        *d = (s[0] + s[1]) * 0.5;
                } else {                                                /* Mix left and right channel */
                    for (i = n; i; i--, s += n_ic, d += n_oc)
                        *d = s[0] * vol_l + s[1] * vol_r;
                }

            }
            break;
        }
        case PA_SAMPLE_S16NE:
        {
            int16_t *d, *s;

            for (oc = 0; oc < n_oc; oc++) {
                int32_t vol_l, vol_r;

                vol_l = m->map_table_i[oc][0];
                vol_r = m->map_table_i[oc][1];

                d = (int16_t *)dst + oc;
                s = (int16_t *)src;

                if (vol_l <= 0 && vol_r <= 0) {
                    for (i = n; i > 0; i--, s += n_ic, d += n_oc)
                        *d = 0;
                } else if ((vol_l >= 0x10000 && vol_r <= 0) ||          /* Exactly one of the channels */
                           (vol_r >= 0x10000 && vol_l <= 0)) {
                    if (vol_l >= 0x10000 && vol_r <= 0)                     /* left channel */
                        s += 0;
                    else if (vol_l <= 0 && vol_r >= 0x10000)                /* right channel */
                        s += 1;
                    for (i = n >> 2; i; i--) {
                        d[0*n_oc] = s[0];
                        d[1*n_oc] = s[2];
                        d[2*n_oc] = s[4];
                        d[3*n_oc] = s[6];
                        s += 4*n_ic;
                        d += 4*n_oc;
                    }
                    for (i = n & 3; i; i--) {
                        d[0] = s[0];
                        s += n_ic;
                        d += n_oc;
                    }
                } else if ((vol_l == 0x08000 && vol_r == 0x08000)) {
                    for (i = n; i; i--, s += n_ic, d += n_oc)
                        *d = (s[0] >> 1) + (s[1] >> 1);
                } else {                                                /* Mix left and right channel */
                    for (i = n; i; i--, s += n_ic, d += n_oc)
                        *d = (int16_t) (((int32_t) s[0] * vol_l + (int32_t) s[1] * vol_r) >> 16);
                }

            }
            break;
        }
        default:
            pa_assert_not_reached();
    }
}

static void remap_51_to_stereo_c(pa_remap_t *m, void *dst, const void *src, unsigned n) {
    unsigned i, n_ic, n_oc;

    n_ic = m->i_ss->channels;
    n_oc = m->o_ss->channels;

    pa_assert(n_ic == 6);
    pa_assert(n_oc == 2);
    pa_assert(m->map_table_i[0][0] == m->map_table_i[1][1]);
    pa_assert(m->map_table_i[0][2] == m->map_table_i[1][3]);
    pa_assert(m->map_table_i[0][1] == 0x0000 && m->map_table_i[1][0] == 0x0000);
    pa_assert(m->map_table_i[0][3] == 0x0000 && m->map_table_i[1][2] == 0x0000);
    pa_assert(m->map_table_i[0][4] == m->map_table_i[1][4]);
    pa_assert(m->map_table_i[0][4] == m->map_table_i[0][5]);
    pa_assert(m->map_table_i[0][4] == m->map_table_i[1][5]);

    switch (*m->format) {
        case PA_SAMPLE_FLOAT32NE:
        {
            float *d, *s;
            float vol_f, vol_r, vol_m;

            vol_f = m->map_table_f[0][0];
            vol_r = m->map_table_f[0][2];
            vol_m = m->map_table_f[0][4];

            d = (float *)dst;
            s = (float *)src;

            for (i = n; i; i--, s += n_ic, d += n_oc) {
                d[0] = d[1] = (s[4] + s[5]) * vol_m;
                d[0] += s[0] * vol_f;
                d[1] += s[1] * vol_f;
                d[0] += s[2] * vol_r;
                d[1] += s[3] * vol_r;
            }

            break;
        }
        case PA_SAMPLE_S16NE:
        {
            int16_t *d, *s;
            int32_t vol_f, vol_r, vol_m;

            vol_f = m->map_table_i[0][0];
            vol_r = m->map_table_i[0][2];
            vol_m = m->map_table_i[0][4];

            d = (int16_t *)dst;
            s = (int16_t *)src;

            for (i = n; i; i--, s += n_ic, d += n_oc) {
                d[0] = d[1] = (int16_t) ((((int32_t) s[4] + (int32_t) s[5]) * vol_m) >> 16);
                d[0] += (int16_t) (((int32_t) s[0] * vol_f) >> 16);
                d[1] += (int16_t) (((int32_t) s[1] * vol_f) >> 16);
                d[0] += (int16_t) (((int32_t) s[2] * vol_r) >> 16);
                d[1] += (int16_t) (((int32_t) s[3] * vol_r) >> 16);
            }

            break;
        }
        default:
            pa_assert_not_reached();
    }
}

static void remap_channels_matrix_c(pa_remap_t *m, void *dst, const void *src, unsigned n) {
    unsigned oc, ic, i;
    unsigned n_ic, n_oc;

    n_ic = m->i_ss->channels;
    n_oc = m->o_ss->channels;

    switch (*m->format) {
        case PA_SAMPLE_FLOAT32NE:
        {
            float *d, *s;

            memset(dst, 0, n * sizeof(float) * n_oc);

            for (oc = 0; oc < n_oc; oc++) {

                for (ic = 0; ic < n_ic; ic++) {
                    float vol;

                    vol = m->map_table_f[oc][ic];

                    if (vol <= 0.0)
                        continue;

                    d = (float *)dst + oc;
                    s = (float *)src + ic;

                    if (vol >= 1.0) {
                        for (i = n; i > 0; i--, s += n_ic, d += n_oc)
                            *d += *s;
                    } else {
                        for (i = n; i > 0; i--, s += n_ic, d += n_oc)
                            *d += *s * vol;
                    }
                }
            }

            break;
        }
        case PA_SAMPLE_S16NE:
        {
            int16_t *d, *s;

            memset(dst, 0, n * sizeof(int16_t) * n_oc);

            for (oc = 0; oc < n_oc; oc++) {

                for (ic = 0; ic < n_ic; ic++) {
                    int32_t vol;

                    vol = m->map_table_i[oc][ic];

                    if (vol <= 0)
                        continue;

                    d = (int16_t *)dst + oc;
                    s = (int16_t *)src + ic;

                    if (vol >= 0x10000) {
                        for (i = n; i > 0; i--, s += n_ic, d += n_oc)
                            *d += *s;
                    } else {
                        for (i = n; i > 0; i--, s += n_ic, d += n_oc)
                            *d += (int16_t) (((int32_t)*s * vol) >> 16);
                    }
                }
            }
            break;
        }
        default:
            pa_assert_not_reached();
    }
}

/* set the function that will execute the remapping based on the matrices */
static void init_remap_c(pa_remap_t *m) {
    unsigned n_oc, n_ic;

    n_oc = m->o_ss->channels;
    n_ic = m->i_ss->channels;

    /* find some common channel remappings, fall back to full matrix operation. */
    if (n_ic == 1 && n_oc == 2 &&
            m->map_table_f[0][0] >= 1.0 && m->map_table_f[1][0] >= 1.0) {
        m->do_remap = (pa_do_remap_func_t) remap_mono_to_stereo_c;
        pa_log_info("Using mono to stereo remapping");
    } else if (n_ic == 2 && n_oc == 1 && m->map_table_i[0][0] == 0x8000 && m->map_table_i[0][1] == 0x8000) {
        m->do_remap = (pa_do_remap_func_t) remap_stereo_to_mono_c;
        pa_log_info("Using stereo to mono remapping");
    } else if (n_ic == 2) {
        m->do_remap = (pa_do_remap_func_t) remap_stereo_to_nchannels_c;
        pa_log_info("Using stereo remapping");
    } else if (n_ic == 6 && n_oc == 2 &&
            m->map_table_i[0][0] == m->map_table_i[1][1] && m->map_table_i[0][2] == m->map_table_i[1][3] &&
            m->map_table_i[0][1] == 0x0000 && m->map_table_i[1][0] == 0x0000 &&
            m->map_table_i[0][3] == 0x0000 && m->map_table_i[1][2] == 0x0000 &&
            m->map_table_i[0][4] == m->map_table_i[1][4] &&
            m->map_table_i[0][4] == m->map_table_i[0][5] &&
            m->map_table_i[0][4] == m->map_table_i[1][5]) {
        m->do_remap = (pa_do_remap_func_t) remap_51_to_stereo_c;
        pa_log_info("Using 5.1 surround to stereo remapping");
    } else {
        m->do_remap = (pa_do_remap_func_t) remap_channels_matrix_c;
        pa_log_info("Using generic matrix remapping");
    }
}


/* default C implementation */
static pa_init_remap_func_t remap_func = init_remap_c;

void pa_init_remap(pa_remap_t *m) {
    pa_assert(remap_func);

    m->do_remap = NULL;

    /* call the installed remap init function */
    remap_func(m);

    if (m->do_remap == NULL) {
        /* nothing was installed, fallback to C version */
        init_remap_c(m);
    }
}

pa_init_remap_func_t pa_get_init_remap_func(void) {
    return remap_func;
}

void pa_set_init_remap_func(pa_init_remap_func_t func) {
    remap_func = func;
}
