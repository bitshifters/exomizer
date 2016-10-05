/*
 * Copyright (c) 2002, 2003 Magnus Lind.
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

#include "membuf.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void membuf_init(struct membuf *sb)
{
    sb->buf = NULL;
    sb->len = 0;
    sb->size = 0;
}
void membuf_clear(struct membuf *sb)
{
    sb->len = 0;
}
void membuf_free(struct membuf *sb)
{
    if (sb->buf != NULL)
    {
        free(sb->buf);
        sb->buf = NULL;
    }
    sb->len = 0;
    sb->size = 0;
}

void membuf_new(struct membuf **sbp)
{
    struct membuf *sb;

    sb = malloc(sizeof(struct membuf));
    if (sb == NULL)
    {
        fprintf(stderr, "error, can't allocate memory\n");
        exit(1);
    }
    sb->buf = NULL;
    sb->len = 0;
    sb->size = 0;

    *sbp = sb;
}

void membuf_delete(struct membuf **sbp)
{
    struct membuf *sb;

    sb = *sbp;
    membuf_free(sb);
    free(sb);
    sb = NULL;
    *sbp = sb;
}

int membuf_memlen(struct membuf *sb)
{
    return sb->len;
}

void *membuf_memcpy(struct membuf *sb, const void *mem, int memlen)
{
    membuf_atleast(sb, memlen);
    memcpy(sb->buf, mem, memlen);
    return sb->buf;
}
void *membuf_append(struct membuf *sb, const void *mem, int memlen)
{
    int len;
    void *p;
    len = sb->len + memlen;
    membuf_atleast(sb, len);
    p = (char *) sb->buf + sb->len;
    memcpy(p, mem, memlen);
    sb->len = len;
    return p;
}

void membuf_atleast(struct membuf *sb, int len)
{
    int size;

    size = sb->size;
    if (size == 0)
        size = 1;
    while (size < len)
    {
        size <<= 1;
    }
    if (size > sb->size)
    {
        sb->buf = realloc(sb->buf, size);
        if (sb->buf == NULL)
        {
            fprintf(stderr, "error, can't reallocate memory\n");
            exit(1);
        }
        sb->size = size;
    }
}

void membuf_atmost(struct membuf *sb, int len)
{
    int size;

    size = sb->size;
    while (size > len)
    {
        size >>= 1;
    }
    if (size < sb->size)
    {
        sb->buf = realloc(sb->buf, size);
        if (sb->buf == NULL)
        {
            fprintf(stderr, "error, can't reallocate memory\n");
            exit(1);
        }
        sb->size = size;
        sb->len = size;
    }
}

int membuf_get_size(struct membuf *sb)
{
    return sb->size;
}
void *membuf_get(struct membuf *sb)
{
    return sb->buf;
}
