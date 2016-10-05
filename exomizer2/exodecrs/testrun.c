/*
 * Copyright (c) 2002 - 2008 Magnus Lind.
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

#include "../src/log.h"
#include "../src/exo_util.h"
#include "../src/6502emu.h"

#include <stdlib.h>

static void mem_access_write(struct mem_access *this, u16 address, u8 value)
{
    u8 *mem = this->ctx;
    mem[address] = value;
}

static u8 mem_access_read(struct mem_access *this, u16 address)
{
    u8 *mem = this->ctx;
    return mem[address];
}

int main(int argc, char *argv[])
{
    struct cpu_ctx r;
    struct load_info info[1];
    static unsigned char mem[65536];
    static unsigned char mem2[65536];
    int p;
    int pending;
    int fail;
    const char *appl;

    /* init logging */
    LOG_INIT_CONSOLE(LOG_BRIEF);

    appl = fixup_appl(argv[0]);

    info->basic_txt_start = 0x0801;
    load_located(argv[1], mem, info);

    mem[0x02a7] = 0x60;

    /* no start address from load*/
    if(info->run == -1)
    {
        /* look for sys line */
        info->run = find_sys(mem + info->basic_txt_start, 0x9e);
    }
    if(info->run == -1)
    {
        LOG(LOG_ERROR, ("Error, can't find entry point.\n"));
        exit(-1);
    }

    LOG(LOG_DEBUG, ("run %04x\n", info->run));

    r.cycles = 0;
    r.mem.ctx = mem;
    r.mem.read = mem_access_read;
    r.mem.write = mem_access_write;
    r.pc = info->run;
    r.sp = '\xff';
    r.flags = 0;

    /* setting up decrunch */
    while(r.sp >= 0x10)
    {
        /* redirect chrout to a safe rts */
        if(r.pc == 0xffd2)
        {
            r.pc = 0x02a7;
        }
        next_inst(&r);
    }

    load_located(argv[2], mem2, info);
    LOG(LOG_BRIEF, ("comparing $%04X - $%04X\n", info->start, info->end));
    p = info->start;
    fail = 0;
    pending = -1;
    while(p < info->end)
    {
        if(mem[p] != mem2[p])
        {
            if(pending < 0)
            {
                pending = p;
                fail = 1;
            }
        }
        else
        {
            if(pending >= 0)
            {
                LOG(LOG_ERROR, ("%s: interval mismatch $%04X - $%04X, "
                                "starts with $%02X, expected $%02X.\n",
                                appl, pending, p - 1,
                                mem[pending],
                                mem2[pending]));
                pending = -1;
            }
        }
        ++p;
    }
    if(pending >= 0)
    {
        LOG(LOG_ERROR, ("%s: interval mismatch $%04X - $%04X, "
                        "starts with $%02X, expected $%02X.\n",
                        appl, pending, p - 1,
                        mem[pending],
                        mem2[pending]));
    }

    return fail;
}
