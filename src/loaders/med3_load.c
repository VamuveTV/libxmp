/* Extended Module Player
 * Copyright (C) 1996-2013 Claudio Matsuoka and Hipolito Carraro Jr
 *
 * Pattern unpacking code by Teijo Kinnunen, 1990
 *
 * This file is part of the Extended Module Player and is distributed
 * under the terms of the GNU Lesser General Public License. See COPYING.LIB
 * for more information.
 */

/*
 * MED 2.00 is in Fish disk #349 and has a couple of demo modules, get it
 * from ftp://ftp.funet.fi/pub/amiga/fish/301-400/ff349
 */

#include <assert.h>
#include "loader.h"

#define MAGIC_MED3	MAGIC4('M','E','D',3)


static int med3_test(xmp_file, char *, const int);
static int med3_load (struct module_data *, xmp_file, const int);

const struct format_loader med3_loader = {
	"MED 2.00 MED3 (MED)",
	med3_test,
	med3_load
};

static int med3_test(xmp_file f, char *t, const int start)
{
	if (read32b(f) !=  MAGIC_MED3)
		return -1;

	read_title(f, t, 0);

	return 0;
}


#define MASK		0x80000000

#define M0F_LINEMSK0F	0x01
#define M0F_LINEMSK1F	0x02
#define M0F_FXMSK0F	0x04
#define M0F_FXMSK1F	0x08
#define M0F_LINEMSK00	0x10
#define M0F_LINEMSK10	0x20
#define M0F_FXMSK00	0x40
#define M0F_FXMSK10	0x80



/*
 * From the MED 2.00 file loading/saving routines by Teijo Kinnunen, 1990
 */

static uint8 get_nibble(uint8 *mem, uint16 *nbnum)
{
	uint8 *mloc = mem + (*nbnum / 2),res;

	if(*nbnum & 0x1)
		res = *mloc & 0x0f;
	else
		res = *mloc >> 4;
	(*nbnum)++;

	return res;
}

static uint16 get_nibbles(uint8 *mem,uint16 *nbnum,uint8 nbs)
{
	uint16 res = 0;

	while (nbs--) {
		res <<= 4;
		res |= get_nibble(mem,nbnum);
	}

	return res;
}

static void unpack_block(struct module_data *m, uint16 bnum, uint8 *from)
{
	struct xmp_module *mod = &m->mod;
	struct xmp_event *event;
	uint32 linemsk0 = *((uint32 *)from), linemsk1 = *((uint32 *)from + 1);
	uint32 fxmsk0 = *((uint32 *)from + 2), fxmsk1 = *((uint32 *)from + 3);
	uint32 *lmptr = &linemsk0, *fxptr = &fxmsk0;
	uint16 fromn = 0, lmsk;
	uint8 *fromst = from + 16, bcnt, *tmpto;
	uint8 *patbuf, *to;
	int i, j, trkn = mod->chn;

	from += 16;
	patbuf = to = calloc(3, 4 * 64);
	assert(to);

	for (i = 0; i < 64; i++) {
		if (i == 32) {
			lmptr = &linemsk1;
			fxptr = &fxmsk1;
		}

		if (*lmptr & MASK) {
			lmsk = get_nibbles(fromst, &fromn, (uint8)(trkn / 4));
			lmsk <<= (16 - trkn);
			tmpto = to;

			for (bcnt = 0; bcnt < trkn; bcnt++) {
				if (lmsk & 0x8000) {
					*tmpto = (uint8)get_nibbles(fromst,
						&fromn,2);
					*(tmpto + 1) = (get_nibble(fromst,
							&fromn) << 4);
				}
				lmsk <<= 1;
				tmpto += 3;
			}
		}

		if (*fxptr & MASK) {
			lmsk = get_nibbles(fromst,&fromn,(uint8)(trkn / 4));
			lmsk <<= (16 - trkn);
			tmpto = to;

			for (bcnt = 0; bcnt < trkn; bcnt++) {
				if (lmsk & 0x8000) {
					*(tmpto+1) |= get_nibble(fromst,
							&fromn);
					*(tmpto+2) = (uint8)get_nibbles(fromst,
							&fromn,2);
				}
				lmsk <<= 1;
				tmpto += 3;
			}
		}
		to += 3 * trkn;
		*lmptr <<= 1;
		*fxptr <<= 1;
	}

	for (i = 0; i < 64; i++) {
		for (j = 0; j < 4; j++) {
			event = &EVENT(bnum, j, i);

			event->note = patbuf[i * 12 + j * 3 + 0];
			if (event->note)
				event->note += 48;
			event->ins  = patbuf[i * 12 + j * 3 + 1] >> 4;
			if (event->ins)
				event->ins++;
			event->fxt  = patbuf[i * 12 + j * 3 + 1] & 0x0f;
			event->fxp  = patbuf[i * 12 + j * 3 + 2];

			switch (event->fxt) {
			case 0x00:	/* arpeggio */
			case 0x01:	/* slide up */
			case 0x02:	/* slide down */
			case 0x03:	/* portamento */
			case 0x04:	/* vibrato? */
				break;
			case 0x0c:	/* set volume (BCD) */
				event->fxp = MSN(event->fxp) * 10 +
							LSN(event->fxp);
				break;
			case 0x0d:	/* volume slides */
				event->fxt = FX_VOLSLIDE;
				break;
			case 0x0f:	/* tempo/break */
				if (event->fxp == 0)
					event->fxt = FX_BREAK;
				if (event->fxp == 0xff) {
					event->fxp = event->fxt = 0;
					event->vol = 1;
				} else if (event->fxp == 0xfe) {
					event->fxp = event->fxt = 0;
				} else if (event->fxp == 0xf1) {
					event->fxt = FX_EXTENDED;
					event->fxp = (EX_RETRIG << 4) | 3;
				} else if (event->fxp == 0xf2) {
					event->fxt = FX_EXTENDED;
					event->fxp = (EX_CUT << 4) | 3;
				} else if (event->fxp == 0xf3) {
					event->fxt = FX_EXTENDED;
					event->fxp = (EX_DELAY << 4) | 3;
				} else if (event->fxp > 10) {
					event->fxt = FX_S3M_BPM;
					event->fxp = 125 * event->fxp / 33;
				}
				break;
			default:
				event->fxp = event->fxt = 0;
			}
		}
	}

	free(patbuf);
}


static int med3_load(struct module_data *m, xmp_file f, const int start)
{
	struct xmp_module *mod = &m->mod;
	int i, j;
	uint32 mask;
	int transp, sliding;

	LOAD_INIT();

	read32b(f);

	set_type(m, "MED 2.00 MED3");

	mod->ins = mod->smp = 32;
	INSTRUMENT_INIT();

	/* read instrument names */
	for (i = 0; i < 32; i++) {
		uint8 c, buf[40];
		for (j = 0; j < 40; j++) {
			c = read8(f);
			buf[j] = c;
			if (c == 0)
				break;
		}
		copy_adjust(mod->xxi[i].name, buf, 32);
		mod->xxi[i].sub = calloc(sizeof (struct xmp_subinstrument), 1);
	}

	/* read instrument volumes */
	mask = read32b(f);
	for (i = 0; i < 32; i++, mask <<= 1) {
		mod->xxi[i].sub[0].vol = mask & MASK ? read8(f) : 0;
		mod->xxi[i].sub[0].pan = 0x80;
		mod->xxi[i].sub[0].fin = 0;
		mod->xxi[i].sub[0].sid = i;
	}

	/* read instrument loops */
	mask = read32b(f);
	for (i = 0; i < 32; i++, mask <<= 1) {
		mod->xxs[i].lps = mask & MASK ? read16b(f) : 0;
	}

	/* read instrument loop length */
	mask = read32b(f);
	for (i = 0; i < 32; i++, mask <<= 1) {
		uint32 lsiz = mask & MASK ? read16b(f) : 0;
		mod->xxs[i].len = mod->xxs[i].lps + lsiz;
		mod->xxs[i].lpe = mod->xxs[i].lps + lsiz;
		mod->xxs[i].flg = lsiz > 1 ? XMP_SAMPLE_LOOP : 0;
	}

	mod->chn = 4;
	mod->pat = read16b(f);
	mod->trk = mod->chn * mod->pat;

	mod->len = read16b(f);
	xmp_fread(mod->xxo, 1, mod->len, f);
	mod->spd = read16b(f);
	if (mod->spd > 10) {
		mod->bpm = 125 * mod->spd / 33;
		mod->spd = 6;
	}
	transp = read8s(f);
	read8(f);			/* flags */
	sliding = read16b(f);		/* sliding */
	read32b(f);			/* jumping mask */
	xmp_fseek(f, 16, SEEK_CUR);		/* rgb */

	/* read midi channels */
	mask = read32b(f);
	for (i = 0; i < 32; i++, mask <<= 1) {
		if (mask & MASK)
			read8(f);
	}

	/* read midi programs */
	mask = read32b(f);
	for (i = 0; i < 32; i++, mask <<= 1) {
		if (mask & MASK)
			read8(f);
	}
	
	MODULE_INFO();

	D_(D_INFO "Sliding: %d", sliding);
	D_(D_INFO "Play transpose: %d", transp);

	if (sliding == 6)
		m->quirk |= QUIRK_VSALL | QUIRK_PBALL;

	for (i = 0; i < 32; i++)
		mod->xxi[i].sub[0].xpo = transp;

	PATTERN_INIT();

	/* Load and convert patterns */
	D_(D_INFO "Stored patterns: %d", mod->pat);

	for (i = 0; i < mod->pat; i++) {
		uint32 *conv;
		uint8 b, tracks;
		uint16 convsz;

		PATTERN_ALLOC(i);
		mod->xxp[i]->rows = 64;
		TRACK_ALLOC(i);

		tracks = read8(f);

		b = read8(f);
		convsz = read16b(f);
		conv = calloc(1, convsz + 16);
		assert(conv);

                if (b & M0F_LINEMSK00)
			*conv = 0L;
                else if (b & M0F_LINEMSK0F)
			*conv = 0xffffffff;
                else
			*conv = read32b(f);

                if (b & M0F_LINEMSK10)
			*(conv + 1) = 0L;
                else if (b & M0F_LINEMSK1F)
			*(conv + 1) = 0xffffffff;
                else
			*(conv + 1) = read32b(f);

                if (b & M0F_FXMSK00)
			*(conv + 2) = 0L;
                else if (b & M0F_FXMSK0F)
			*(conv + 2) = 0xffffffff;
                else
			*(conv + 2) = read32b(f);

                if (b & M0F_FXMSK10)
			*(conv + 3) = 0L;
                else if (b & M0F_FXMSK1F)
			*(conv + 3) = 0xffffffff;
                else
			*(conv + 3) = read32b(f);

		xmp_fread(conv + 4, 1, convsz, f);

                unpack_block(m, i, (uint8 *)conv);

		free(conv);
	}

	/* Load samples */

	D_(D_INFO "Instruments: %d", mod->ins);

	mask = read32b(f);
	for (i = 0; i < 32; i++, mask <<= 1) {
		if (~mask & MASK)
			continue;

		mod->xxs[i].len = read32b(f);
		if (read16b(f))		/* type */
			continue;

		mod->xxi[i].nsm = !!(mod->xxs[i].len);

		D_(D_INFO "[%2X] %-32.32s %04x %04x %04x %c V%02x ",
			i, mod->xxi[i].name, mod->xxs[i].len, mod->xxs[i].lps,
			mod->xxs[i].lpe,
			mod->xxs[i].flg & XMP_SAMPLE_LOOP ? 'L' : ' ',
			mod->xxi[i].sub[0].vol);

		load_sample(m, f, 0, &mod->xxs[mod->xxi[i].sub[0].sid], NULL);
	}

	return 0;
}
