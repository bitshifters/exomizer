/*
 * Copyright (c) 2003 Magnus Lind.
 *
 * This software is provided 'as-is', without any express or implied warranty.
 * In no event will the authors be held liable for any damages arising from
 * the use of this software.
 *
 * Permission is granted to anyone to use this software, alter it and re-
 * distribute it freely for any non-commercial, non-profit purpose subject to
 * the following restrictions:
 *
 *   1. The origin of this software must not be misrepresented; you must not
 *   claim that you wrote the original software. If you use this software in a
 *   product, an acknowledgment in the product documentation would be
 *   appreciated but is not required.
 *
 *   2. Altered source versions must be plainly marked as such, and must not
 *   be misrepresented as being the original software.
 *
 *   3. This notice may not be removed or altered from any distribution.
 *
 *   4. The names of this software and/or it's copyright holders may not be
 *   used to endorse or promote products derived from this software without
 *   specific prior written permission.
 *
 */

#include "vec.h"
#include <stdlib.h>

void vec_init(struct vec *p, size_t elsize)
{
    p->elsize = elsize;
    membuf_init(&p->buf);
}

void vec_clear(struct vec *p, cb_free * f)
{
    struct vec_iterator i;
    void *d;

    vec_get_iterator(p, &i);

    if (f != NULL)
    {
        while ((d = vec_iterator_next(&i)) != NULL)
        {
            f(d);
        }
    }
    membuf_clear(&p->buf);
}

void vec_free(struct vec *p, cb_free * f)
{
    vec_clear(p, f);
    membuf_free(&p->buf);
}

int vec_count(struct vec *p)
{
    int count;
    count = membuf_memlen(&p->buf) / p->elsize;
    return count;
}

void *vec_get(struct vec *p, int index)
{
    char *buf;

    buf = (char *) membuf_get(&p->buf);
    buf += index * p->elsize;

    return (void *)buf;
}

void *vec_push(struct vec *p, const void *in)
{
    void *out;
    out = membuf_append(&p->buf, in, p->elsize);

    return out;
}

void *vec_find(struct vec *p, cb_cmp * f, const void *in)
{
    int count;
    void *out;

    count = vec_count(p);
    out = bsearch(in, membuf_get(&p->buf), count, p->elsize, f);

    return out;
 }

int vec_insert_uniq(struct vec *p, cb_cmp * f, const void *in, void **outp)
{
    int val = 0;
    void *out;

    if((out = vec_find(p, f, in)) == NULL)
    {
        vec_push(p, in);
        qsort(membuf_get(&p->buf), vec_count(p), p->elsize, f);

        out = vec_find(p, f, in);
        val = 1;
    }
    if(outp != NULL)
    {
        *outp = out;
    }

    return val;
}

void vec_get_iterator(struct vec *p, struct vec_iterator *i)
{
    i->vec = p;
    i->pos = 0;
}

void *vec_iterator_next(struct vec_iterator *i)
{
    void *out;
    int count = vec_count(i->vec);
    if (i->pos >= count)
    {
        return NULL;
    }
    out = vec_get(i->vec, i->pos);
    i->pos += 1;
    return out;
}
