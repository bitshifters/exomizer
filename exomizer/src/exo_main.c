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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include "log.h"
#include "search.h"
#include "optimal.h"
#include "output.h"
#include "sfx.h"
#include "getflag.h"

/* returns 0 if ok, 1 otherwise */
static
int
str_to_int(const char *str, int *value)
{
    int status = 0;
    do {
        char *str_end;
        long lval;

        /* base 0 is auto detect */
        int base = 0;

        if (*str == '\0')
        {
            /* no string to parse */
            status = 1;
            break;
        }

        if (*str == '$')
        {
            /* a $ prefix specifies base 16 */
            ++str;
            base = 16;
        }

        lval = strtol(str, &str_end, base);

        if(*str_end != '\0')
        {
            /* there is garbage in the string */
            status = 1;
            break;
        }

        if(value != NULL)
        {
            /* all is well, set the out parameter */
            *value = (int)lval;
        }
    } while(0);

    return status;
}

static
FILE *
open_file(char *name, int *load_addr)
{
    FILE * in;
    int relocated = 0;
    int reloc;
    int load;

    do {
        char *load_str;

        in = fopen(name, "rb");
        if (in != NULL)
        {
            /* we have succeded in opening the file */
            break;
        }

        /* hmm, let's see if the user is trying to relocate it */
        load_str = strrchr(name, ',');
        if (load_str == NULL)
        {
            /* we fail */
            break;
        }

        *load_str = '\0';
        ++load_str;
        relocated = 1;

        /* relocation was requested */
        if (str_to_int(load_str, &reloc) != 0)
        {
            /* we fail */
            LOG(LOG_FATAL,
                (" can't parse load address from \"%s\"\n", load_str));
            exit(-1);
        }

        in = fopen(name, "rb");

    } while (0);
    if (in == NULL)
    {
        LOG(LOG_FATAL,
            (" can't open file \"%s\" for input\n", name));
        exit(-1);
    }

    /* set the load address */
    load = fgetc(in);
    load |= fgetc(in) << 8;

    if(load_addr != NULL)
    {
        *load_addr = load;
        if (relocated)
        {
            *load_addr = reloc;
        }
    }
    return in;
}

static int find_sys(const unsigned char *buf)
{
    int outstart = -1;
    int state = 1;
    int i = 0;
    /* skip línk and line number */
    buf += 4;
    /* exit loop at line end */
    while(i < 1000 && buf[i] != '\0')
    {
        unsigned char *sys_end;
        int c = buf[i];

        switch(state)
        {
            /* look for and consume sys token */
        case 1:
            if(c == 0x9e) state = 2;
            break;
            /* skip spaces and left parenthesis, if any */
        case 2:
            if(strchr(" (", c) != NULL) break;
            state = 3;
            /* convert string number to int */
        case 3:
            outstart = strtol((char*)(buf + i), (void*)&sys_end, 10);
            if((buf + i) == sys_end)
            {
                /* we got nothing */
                outstart = -1;
            }
            state = 4;
            break;
        case 4:
            break;
        }
        ++i;
    }

    LOG(LOG_DEBUG, ("state when leaving: %d.\n", state));
    return outstart;
}

static
void
do_load(int filec, char *filev[], unsigned char *buf, int *load, int *len)
{
    int min_load = -1;
    int max_end = -1;
    int i;
    FILE *in;

    for (i = 0; i < filec; ++i)
    {
        int tmp_load, tmp_end;

        in = open_file(filev[i], &tmp_load);

        tmp_end =
            tmp_load + fread(buf + tmp_load, 1, 65536 - tmp_load, in);

        fclose(in);

        LOG(LOG_NORMAL,
            (" filename: \"%s\", loading from $%04X to $%04X\n",
             filev[i], tmp_load, tmp_end));

        if (min_load == -1 || tmp_load < min_load)
        {
            min_load = tmp_load;
        }
        if (max_end == -1 || tmp_end > max_end)
        {
            max_end = tmp_end;
        }
    }
    /* move memory to beginning of buffer */
    LOG(LOG_NORMAL, (" "));
    LOG(LOG_BRIEF, ("crunching from $%04X to $%04X ", min_load, max_end));
    LOG(LOG_NORMAL, ("(~%d '.')\n", (max_end - min_load) >> 8));
    *load = min_load;
    *len = max_end - min_load;
}

static
int
generate_output(match_ctx ctx,
                search_nodep snp,
                struct sfx_decruncher *decr,
                encode_match_f * f,
                encode_match_data emd,
                int load, int len, int start, FILE * of)
{
    int pos;
    int pos_diff;
    int max_diff;
    int diff;
    static output_ctx out;
    output_ctxp old;

    output_ctx_init(out);

    old = emd->out;
    emd->out = out;

    if (start >= 0 && start < 0x10000)
    {
        /* -s given */
    } else if (start >= 0x10000 && start < 0x20000)
    {
        /* -l given */
        output_word(out, (unsigned short int) (start - 0x10000));
    } else if (start >= 0x20000 && start < 0x30000)
    {
        /* -l auto given */
        output_word(out, (unsigned short int)0);
    } else if (start == -2)
    {
        /* -r given */
        output_ctx_set_reverse(out);
    }

    pos = output_get_pos(out);

    pos_diff = pos;
    max_diff = 0;

    LOG(LOG_DUMP, ("pos $%04X\n", out->pos));
    output_gamma_code(out, 16);
    output_bits(out, 1, 0); /* 1 bit out */

    diff = output_get_pos(out) - pos_diff;
    if(diff > max_diff)
    {
        max_diff = diff;
    }

    LOG(LOG_DUMP, ("pos $%04X\n", out->pos));
    LOG(LOG_DUMP, ("------------\n"));

    while (snp != NULL)
    {
        const_matchp mp;

        mp = snp->match;
        if (mp != NULL && mp->len > 0)
        {
            if (mp->offset == 0)
            {
                /* literal */
                output_byte(out, ctx->buf[snp->index]);
                output_bits(out, 1, 1);
            } else
            {
                f(mp, emd);
                output_bits(out, 1, 0);
            }

            pos_diff += mp->len;
            diff = output_get_pos(out) - pos_diff;
            if(diff > max_diff)
            {
                max_diff = diff;
            }
        }
        LOG(LOG_DUMP, ("------------\n"));
        snp = snp->prev;
    }

    LOG(LOG_DUMP, ("pos $%04X\n", out->pos));
    /* output header here */
    optimal_out(out, emd);
    LOG(LOG_DUMP, ("pos $%04X\n", out->pos));

    output_bits_flush(out);

    output_word(out, (unsigned short int) (load + len));
    LOG(LOG_DUMP, ("pos $%04X\n", out->pos));

    LOG(LOG_NORMAL,
        ("  length of compressed data (including header): %d\n",
         output_get_pos(out) - pos));

    if (start >= 0 && start < 0x10000)
    {
        /* -s given */
        len = output_get_pos(out);
        decr->load(out, (unsigned short int) load - max_diff);
        output_copy_bytes(out, 0, len);

        /* second stage of decruncher */
        decr->stages(out, (unsigned short int) start);
    } else if (start >= 0x20000 && start < 0x30000)
    {
        /* update auto load address */
        load = start - 0x20000 - max_diff;
        LOG(LOG_NORMAL,
            ("  final load address determined by auto setting to $%04X\n",
             load));

        pos = output_get_pos(out);
        output_set_pos(out, 0);
        output_word(out, (unsigned short int)load);
        output_set_pos(out, pos);
    }

    len = output_ctx_close(out, of);
    emd->out = old;

    return len;
}

static
search_nodep
do_compress(match_ctx ctx, encode_match_data emd, int max_passes)
{
    matchp_cache_enum mpce;
    matchp_snp_enum snpe;
    search_nodep snp;
    search_nodep best_snp;
    int pass;
    float size;
    float old_size;

    pass = 1;
    LOG(LOG_NORMAL, (" pass %d: optimizing ", pass));
    LOG(LOG_BRIEF, ("."));
    LOG(LOG_NORMAL, (".\n"));

    matchp_cache_get_enum(ctx, mpce);
    optimal_optimize(emd, matchp_cache_enum_get_next, mpce);

    /*optimal_dump(emd);*/

    best_snp = NULL;
    old_size = 1000000.0;

    for (;;)
    {
        snp = search_buffer(ctx, optimal_encode, emd);
        if (snp == NULL)
        {
            LOG(LOG_ERROR, ("error: search_buffer() returned NULL\n"));
            exit(-1);
        }

        size = snp->total_score;
        LOG(LOG_NORMAL, ("  size %0.1f bits ~%d bytes\n",
                         size, (((int) size) + 7) >> 3));

        if (size >= old_size)
        {
            search_node_free(snp);
            break;
        }

        if (best_snp != NULL)
        {
            search_node_free(best_snp);
        }
        best_snp = snp;
        old_size = size;
        ++pass;

        if(pass > max_passes)
        {
            break;
        }

        optimal_free(emd);
        optimal_init(emd);

        LOG(LOG_NORMAL, (" pass %d: optimizing ", pass));
        LOG(LOG_BRIEF, ("."));
        LOG(LOG_NORMAL, (".\n"));

        matchp_snp_get_enum(snp, snpe);
        optimal_optimize(emd, matchp_snp_enum_get_next, snpe);
    }

    /* optimal_dump(emd); */
    return best_snp;
}

static
void
do_output(match_ctx ctx,
          search_nodep snp,
          encode_match_data emd,
          struct sfx_decruncher *decr,
          const char *outname, int load, int len, int start)
{
    FILE *outfile;

    outfile = fopen(outname, "wb");
    if (outfile == NULL)
    {
        LOG(LOG_ERROR, (" can't open file \"%s\" for output\n", outname));
        exit(-1);
    }

    len = generate_output(ctx, snp, decr, optimal_encode, emd,
                          load, len, start, outfile);
    fclose(outfile);

    LOG(LOG_NORMAL, ("  filename: \"%s\",", outname));
    LOG(LOG_BRIEF, (" crunched length: %d bytes\n", len));
}

static
void print_license()
{
    LOG(LOG_BRIEF,
        ("----------------------------------------------------------------------------\n"
         "Exomizer v1.1.5, Copyright (c) 2002 - 2004 Magnus Lind. (magli143@comhem.se)\n"
         "----------------------------------------------------------------------------\n"));
    LOG(LOG_BRIEF,
        ("This software is provided 'as-is', without any express or implied warranty.\n"
         "In no event will the authors be held liable for any damages arising from\n"
         "the use of this software.\n"
         "Permission is granted to anyone to use this software, alter it and re-\n"
         "distribute it freely for any non-commercial, non-profit purpose subject to\n"
         "the following restrictions:\n\n"));
    LOG(LOG_BRIEF,
        ("   1.  The origin of this software must not be misrepresented; you must not\n"
         "   claim that you wrote the original software. If you use this software in a\n"
         "   product, an acknowledgment in the product documentation would be\n"
         "   appreciated but is not required.\n"
         "   2. Altered source versions must be plainly marked as such, and must not\n"
         "   be misrepresented as being the original software.\n"
         "   3. This notice may not be removed or altered from any distribution.\n"));
    LOG(LOG_BRIEF,
        ("   4. The names of this software and/or it's copyright holders may not be\n"
         "   used to endorse or promote products derived from this software without\n"
         "   specific prior written permission.\n"
         "----------------------------------------------------------------------------\n"
         "The files processed and/or generated by using this software are not covered\n"
         "nor infected by this license in any way.\n"));
}

static
void print_usage(const char *appl, enum log_level level)
{
    const char *applp;

    /* strip pathprefix from appl */
    applp = strrchr(appl, '\\');
    if (applp != NULL)
    {
        appl = applp + 1;
    }                           /* done */
    applp = strrchr(appl, '/');
    if (applp != NULL)
    {
        appl = applp + 1;
    }
    /* done */
    LOG(level, ("usage: %s [option]... infile[,<address>]...\n", appl));
    LOG(level,
        ("  -r           writes the outfile backwards without load address, this is\n"
         "               suitable for files that are to be decompressed directly\n"
         "               from disk, can't be combined with -l or -s\n"
         "  -l <address> adds load address to the outfile, using \"auto\" as <address>\n"
         "               will enable load address auto detection, can't be combined\n"
         "               with -r or -s, default is no load address\n"
         "  -s <address> adds basic-line and decruncher that exits with a jmp <address>,\n"));
    LOG(level,
        ("               using \"sys\" as <address> will enable basic-sys address auto\n"
         "               detection, can't be combined with -r or -l\n"
         "  -o <outname> sets the outfile name, default is \"a.prg\"\n"
         "  -q           enable quiet mode, display output is reduced to one line\n"
         "  -4           the decruncher targets the c16/+4 computers instead of the c64,\n"
         "               must be combined with -s\n"
         "  -n           turn off the decrunch effect shown by the decruncher, must be\n"
         "               combined with -s\n"));
    LOG(level,
        ("  -m <offset>  limits the maximum offset size\n"
         "  -p <passes>  limits the maximum number of optimization passes\n"
         "  --           treat all args to the right as non-options\n"
         "  -?           displays this help screen\n"
         "  -v           displays version and the usage license\n"
         " All infiles are merged into the outfile. They are loaded in the order\n"
         " they are given on the command-line, from left to right.\n"));
}

int
main(int argc, char *argv[])
{
    const char *outfile = "a.prg";
    int reverse = 0;
    int outstart = -1;
    int decruncher = 0;
    int loadaddr = 0;
    int outload = -1;
    int c, infilec;
    char **infilev;
    int load;
    int len;
    int max_offset = 65536;
    int max_passes = 65536;
    static unsigned char mem[65536];
    static match_ctx ctx;
    encode_match_data emd;
    encode_match_priv optimal_priv;
    search_nodep snp;

#define DECR_TARGET_C64  0
#define DECR_TARGET_C264 1
#define DECR_TYPE_STANDARD 0
#define DECR_TYPE_NO_EFFECT 1

    int basic_start[] = {0x801, 0x1001};

    struct sfx_decruncher *decr_matrix[][2] = {
        {sfx_c64, sfx_c64ne}, {sfx_c264, sfx_c264ne}};
    int decr_target = DECR_TARGET_C64;
    int decr_type = DECR_TYPE_STANDARD;

    /* init logging */
    LOG_INIT_CONSOLE(LOG_NORMAL);

    LOG(LOG_DUMP, ("flagind %d\n", flagind));
    outload = -1;
    while ((c = getflag(argc, argv, "m:4qrnl:s:o:vp:")) != -1)
    {
        LOG(LOG_DUMP, (" flagind %d flagopt '%c'\n", flagind, c));
        switch (c)
        {
        case 'q':
            LOG_SET_LEVEL(LOG_BRIEF);
            break;
        case 'r':
            reverse = 1;
            break;
        case 'm':
            if (str_to_int(flagarg, &max_offset) != 0 ||
                max_offset < 0 || max_offset >= 65536)
            {
                LOG(LOG_ERROR,
                    ("error: invalid offset for -m option, "
                     "must be in the range of [0 - 0xffff]\n"));
                print_usage(argv[0], LOG_NORMAL);
                exit(-1);
            }
            break;
        case 'l':
            loadaddr = 1;
            if (strcmp(flagarg, "auto") != 0 && strcmp(flagarg, "AUTO") != 0 &&
                (str_to_int(flagarg, &outload) != 0 ||
                 outload < 0 || outload >= 65536))
            {
                LOG(LOG_ERROR,
                    ("error: invalid address for -l option, "
                     "must be in the range of [0 - 0xffff]\n"));
                print_usage(argv[0], LOG_NORMAL);
                exit(-1);
            }
            break;
        case 'p':
            if (str_to_int(flagarg, &max_passes) != 0 ||
                max_passes < 1 || max_passes >= 65536)
            {
                LOG(LOG_ERROR,
                    ("error: invalid value for -p option, "
                     "must be in the range of [1 - 0xffff]\n"));
                print_usage(argv[0], LOG_NORMAL);
                exit(-1);
            }
            break;
        case 's':
            decruncher = 1;
            if (strcmp(flagarg, "sys") != 0 && strcmp(flagarg, "SYS") != 0 &&
                (str_to_int(flagarg, &outstart) != 0 ||
                 outstart < 0 || outstart >= 65536))
            {
                LOG(LOG_ERROR,
                    ("error: invalid address for -s option, "
                     "must be in the range of [0 - 0xffff]\n"));
                print_usage(argv[0], LOG_NORMAL);
                exit(-1);
            }
            break;
        case 'o':
            outfile = flagarg;
            break;
        case 'n':
            decr_type = DECR_TYPE_NO_EFFECT;
            break;
        case 'v':
            print_license();
            exit(0);
        case '4':
            decr_target = DECR_TARGET_C264;
            break;
        default:
            if (flagflag != '?')
            {
                LOG(LOG_ERROR,
                    ("error, invalid option \"-%c\"", flagflag));
                if (flagarg != NULL)
                {
                    LOG(LOG_ERROR, (" with argument \"%s\"", flagarg));
                }
                LOG(LOG_ERROR, ("\n"));
            }
            print_usage(argv[0], LOG_BRIEF);
            exit(0);
        }
    }
#if 0
    LOG(LOG_DEBUG, ("flagind %d\n", flagind));
    for (c = 0; c < argc; ++c)
    {
        if (c == flagind)
        {
            LOG(LOG_DEBUG, ("-----------------------\n"));
        }
        LOG(LOG_DEBUG, ("argv[%d] = \"%s\"\n", c, argv[c]));
    }
    exit(1);
#endif

    infilev = argv + flagind;
    infilec = argc - flagind;

    if (reverse + loadaddr + decruncher > 1)
    {
        LOG(LOG_ERROR,
            ("error: the -r, -l or -s options can't be combined.\n"));
        print_usage(argv[0], LOG_NORMAL);
        exit(-1);
    }
    if (decr_target == DECR_TARGET_C264 && !decruncher)
    {
        LOG(LOG_ERROR,
            ("error: the -4 option must be combined with -s.\n"));
        print_usage(argv[0], LOG_NORMAL);
        exit(-1);
    }
    if (decr_type == DECR_TYPE_NO_EFFECT && !decruncher)
    {
        LOG(LOG_ERROR,
            ("error: the -n option must be combined with -s.\n"));
        print_usage(argv[0], LOG_NORMAL);
        exit(-1);
    }
    if (infilec == 0)
    {
        LOG(LOG_ERROR, ("error: no input files to process.\n"));
        print_usage(argv[0], LOG_NORMAL);
        exit(-1);
    }

    /* zero fill mem */
    memset(mem, 0, sizeof(mem));
    do_load(infilec, infilev, mem, &load, &len);

    LOG(LOG_NORMAL, ("Mode for target file: "));
    if (reverse)
    {
        LOG(LOG_NORMAL, ("decompression from file\n"));
        outstart = -2;
    } else if(decruncher)
    {
        LOG(LOG_NORMAL,
            ("self-decompressing %s executable",
             decr_matrix[decr_target][decr_type]->text));
        if(outstart < 0)
        {
            outstart = find_sys(mem + basic_start[decr_target]);
            if(outstart < 0)
            {
                LOG(LOG_ERROR, ("\nerror: cant find sys address.\n"));
                exit(-1);
            }
        }
        LOG(LOG_NORMAL, (", jmp address $%04X\n", outstart));
    }else
    {
        LOG(LOG_NORMAL, ("decompression from memory"));
        if (loadaddr)
        {
            if(outload < 0)
            {
                outload = load;
                LOG(LOG_NORMAL, (", loading to: $%04X - safety", outload));
                outstart = outload + 0x20000;
            } else
            {
                LOG(LOG_NORMAL, (", loading to: $%04X", outload));
                outstart = outload + 0x10000;
            }
        }
        LOG(LOG_NORMAL, ("\n"));
    }

    LOG(LOG_NORMAL,
        ("\nPhase 1: Instrumenting input file(s)"
         "\n------------------------------------\n"));

    match_ctx_init(ctx, mem + load, len, max_offset);

    LOG(LOG_NORMAL, (" Instrumenting file(s), done.\n"));

    emd->out = NULL;
    emd->priv = optimal_priv;

    optimal_init(emd);

    LOG(LOG_NORMAL,
        ("\nPhase 2: Calculating encoding"
         "\n-----------------------------\n"));
    snp = do_compress(ctx, emd, max_passes);
    LOG(LOG_NORMAL, (" Calculating encoding, done.\n"));

    LOG(LOG_NORMAL,
        ("\nPhase 3: Generating output file"
         "\n------------------------------\n"));
    do_output(ctx, snp, emd, decr_matrix[decr_target][decr_type],
              outfile, load, len, outstart);

    LOG(LOG_NORMAL, (" Generating file, done.\n"));

    optimal_free(emd);

    search_node_free(snp);
    match_ctx_free(ctx);

    LOG_FREE;

    return 0;
}
