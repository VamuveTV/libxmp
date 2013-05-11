/* Extended Module Player
 * Copyright (C) 1996-2013 Claudio Matsuoka and Hipolito Carraro Jr
 *
 * This file is part of the Extended Module Player and is distributed
 * under the terms of the GNU Lesser General Public License. See COPYING.LIB
 * for more information.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#ifdef __native_client__
#include <sys/syslimits.h>
#else
#include <limits.h>
#endif

#if !defined(HAVE_POPEN) && defined(WIN32)
#include "../win32/ptpopen.h"
#endif

#include "format.h"
#include "list.h"
#include "md5.h"
#include "loaders/loader.h"


extern struct format_loader *format_loader[];

void load_prologue(struct context_data *);
void load_epilogue(struct context_data *);


int decrunch_arc	(xmp_file, xmp_file);
int decrunch_arcfs	(xmp_file, xmp_file);
int decrunch_sqsh	(xmp_file, xmp_file);
int decrunch_pp		(xmp_file, xmp_file);
int decrunch_mmcmp	(xmp_file, xmp_file);
int decrunch_muse	(xmp_file, xmp_file);
int decrunch_lzx	(xmp_file, xmp_file);
int decrunch_oxm	(xmp_file, xmp_file);
int decrunch_xfd	(xmp_file, xmp_file);
int decrunch_s404	(xmp_file, xmp_file);
int decrunch_zip	(xmp_file, xmp_file);
int decrunch_gzip	(xmp_file, xmp_file);
int decrunch_compress	(xmp_file, xmp_file);
int decrunch_bzip2	(xmp_file, xmp_file);
int decrunch_xz		(xmp_file, xmp_file);
int decrunch_lha	(xmp_file, xmp_file);
int decrunch_zoo	(xmp_file, xmp_file);
int test_oxm		(xmp_file);
char *test_xfd		(unsigned char *, int);

enum {
	BUILTIN_PP = 0x01,
	BUILTIN_SQSH,
	BUILTIN_MMCMP,
	BUILTIN_ARC,
	BUILTIN_ARCFS,
	BUILTIN_S404,
	BUILTIN_OXM,
	BUILTIN_XFD,
	BUILTIN_MUSE,
	BUILTIN_LZX,
	BUILTIN_ZIP,
	BUILTIN_GZIP,
	BUILTIN_COMPRESS,
	BUILTIN_BZIP2,
	BUILTIN_XZ,
	BUILTIN_LHA,
	BUILTIN_ZOO,
};


#if defined __EMX__ || defined WIN32
#define REDIR_STDERR "2>NUL"
#elif defined unix || defined __unix__
#define REDIR_STDERR "2>/dev/null"
#else
#define REDIR_STDERR
#endif

#define DECRUNCH_MAX 5 /* don't recurse more than this */

#define BUFLEN 16384

static void set_md5sum(xmp_file f, unsigned char *digest)
{
	unsigned char buf[BUFLEN];
	MD5_CTX ctx;
	int bytes_read;

	xmp_fseek(f, 0, SEEK_SET);

	MD5Init(&ctx);
	while ((bytes_read = xmp_fread(buf, 1, BUFLEN, f)) > 0) {
		MD5Update(&ctx, buf, bytes_read);
	}
	MD5Final(&ctx);

	memcpy(digest, ctx.digest, 16);
}

static int decrunch(xmp_file *f, char **s, int ttl)
{
    unsigned char b[1024];
    char *cmd;
    xmp_file t;
    int builtin, res;
    char *temp2;
    int headersize;

    cmd = NULL;
    builtin = res = 0;

    xmp_fseek(*f, 0, SEEK_SET);
    if ((headersize = xmp_fread(b, 1, 1024, *f)) < 100)	/* minimum valid file size */
	return 0;

#if defined __AMIGA__ && !defined __AROS__
    if (test_xfd(b, 1024)) {
	builtin = BUILTIN_XFD;
    } else
#endif

    if (b[0] == 'P' && b[1] == 'K' &&
	((b[2] == 3 && b[3] == 4) || (b[2] == '0' && b[3] == '0' &&
	b[4] == 'P' && b[5] == 'K' && b[6] == 3 && b[7] == 4))) {

	/* Zip */
	builtin = BUILTIN_ZIP;
    } else if (b[2] == '-' && b[3] == 'l' && b[4] == 'h') {
	/* LHa */
	builtin = BUILTIN_LHA;
    } else if (b[0] == 31 && b[1] == 139) {
	/* gzip */
	builtin = BUILTIN_GZIP;
    } else if (b[0] == 'B' && b[1] == 'Z' && b[2] == 'h') {
	/* bzip2 */
	builtin = BUILTIN_BZIP2;
    } else if (b[0] == 0xfd && b[3] == 'X' && b[4] == 'Z' && b[5] == 0x00) {
	/* xz */
	builtin = BUILTIN_XZ;
    } else if (b[0] == 'Z' && b[1] == 'O' && b[2] == 'O' && b[3] == ' ') {
	/* zoo */
	builtin = BUILTIN_ZOO;
    } else if (b[0] == 'M' && b[1] == 'O' && b[2] == '3') {
	/* MO3 */
	cmd = "unmo3 -s \"%s\" STDOUT";
    } else if (b[0] == 31 && b[1] == 157) {
	/* compress */
	builtin = BUILTIN_COMPRESS;
    } else if (memcmp(b, "PP20", 4) == 0) {
	/* PowerPack */
	builtin = BUILTIN_PP;
    } else if (memcmp(b, "XPKF", 4) == 0 && memcmp(b + 8, "SQSH", 4) == 0) {
	/* SQSH */
	builtin = BUILTIN_SQSH;
    } else if (!memcmp(b, "Archive\0", 8)) {
	/* ArcFS */
	builtin = BUILTIN_ARCFS;
    } else if (memcmp(b, "ziRCONia", 8) == 0) {
	/* MMCMP */
	builtin = BUILTIN_MMCMP;
    } else if (memcmp(b, "MUSE", 4) == 0 && readmem32b(b + 4) == 0xdeadbeaf) {
	/* J2B MUSE */
	builtin = BUILTIN_MUSE;
    } else if (memcmp(b, "MUSE", 4) == 0 && readmem32b(b + 4) == 0xdeadbabe) {
	/* MOD2J2B MUSE */
	builtin = BUILTIN_MUSE;
    } else if (memcmp(b, "LZX", 3) == 0) {
	/* LZX */
	builtin = BUILTIN_LZX;
    } else if (memcmp(b, "Rar", 3) == 0) {
	/* rar */
	cmd = "unrar p -inul -xreadme -x*.diz -x*.nfo -x*.txt "
	    "-x*.exe -x*.com \"%s\"";
    } else if (memcmp(b, "S404", 4) == 0) {
	/* Stonecracker */
	builtin = BUILTIN_S404;
    } else if (test_oxm(*f) == 0) {
	/* oggmod */
	builtin = BUILTIN_OXM;
    }

    if (builtin == 0 && cmd == NULL && b[0] == 0x1a) {
	int x = b[1] & 0x7f;
	int i, flag = 0;
	long size;
	
	/* check file name */
	for (i = 0; i < 13; i++) {
	    if (b[2 + i] == 0) {
		if (i == 0)		/* name can't be empty */
		    flag = 1;
		break;
	    }
	    if (!isprint(b[2 + i])) {	/* name must be printable */
		flag = 1;
		break;
	    }
	}

	size = readmem32l(b + 15);	/* max file size is 512KB */
	if (size < 0 || size > 512 * 1024)
		flag = 1;

        if (flag == 0) {
	    if (x >= 1 && x <= 9 && x != 7) {
		/* Arc */
		builtin = BUILTIN_ARC;
	    } else if (x == 0x7f) {
		/* !Spark */
		builtin = BUILTIN_ARC;
	    }
	}
    }

    xmp_fseek(*f, 0, SEEK_SET);

    if (builtin == 0 && cmd == NULL)
	return 0;

#if defined ANDROID || defined __native_client__
    /* Don't use external helpers in android */
    if (cmd)
	return 0;
#endif

    D_(D_WARN "Depacking file... ");

    if ((t = xmp_fopen_mem(NULL, 0)) == NULL) {
	D_(D_CRIT "failed");
	return -1;
    }

    if (cmd) {
#define BSIZE 0x4000
	int n;
	char line[1024], buf[BSIZE];
	FILE *p;

	snprintf(line, 1024, cmd, *s);

#ifdef WIN32
	/* Note: The _popen function returns an invalid file opaque, if
	 * used in a Windows program, that will cause the program to hang
	 * indefinitely. _popen works properly in a Console application.
	 * To create a Windows application that redirects input and output,
	 * read the section "Creating a Child Process with Redirected Input
	 * and Output" in the Win32 SDK. -- Mirko 
	 */
	if ((p = popen(line, "rb")) == NULL) {
#else
	/* Linux popen fails with "rb" */
	if ((p = popen(line, "r")) == NULL) {
#endif
	    D_(D_CRIT "failed");
	    xmp_fclose(t);
	    return -1;
	}
	while ((n = fread(buf, 1, BSIZE, p)) > 0)
	    xmp_fwrite(buf, 1, n, t);
	pclose (p);
    } else {
	switch (builtin) {
	case BUILTIN_PP:    
	    res = decrunch_pp(*f, t);
	    break;
	case BUILTIN_ARC:
	    res = decrunch_arc(*f, t);
	    break;
	case BUILTIN_ARCFS:
	    res = decrunch_arcfs(*f, t);
	    break;
	case BUILTIN_SQSH:    
	    res = decrunch_sqsh(*f, t);
	    break;
	case BUILTIN_MMCMP:    
	    res = decrunch_mmcmp(*f, t);
	    break;
	case BUILTIN_MUSE:    
	    res = decrunch_muse(*f, t);
	    break;
	case BUILTIN_LZX:    
	    res = decrunch_lzx(*f, t);
	    break;
	case BUILTIN_S404:
	    res = decrunch_s404(*f, t);
	    break;
	case BUILTIN_ZIP:
	    res = decrunch_zip(*f, t);
	    break;
	case BUILTIN_GZIP:
	    res = decrunch_gzip(*f, t);
	    break;
	case BUILTIN_COMPRESS:
	    res = decrunch_compress(*f, t);
	    break;
	case BUILTIN_BZIP2:
	    res = decrunch_bzip2(*f, t);
	    break;
	case BUILTIN_XZ:
	    res = decrunch_xz(*f, t);
	    break;
	case BUILTIN_LHA:
	    res = decrunch_lha(*f, t);
	    break;
	case BUILTIN_ZOO:
	    res = decrunch_zoo(*f, t);
	    break;
	case BUILTIN_OXM:
	    res = decrunch_oxm(*f, t);
	    break;
#ifdef AMIGA
	case BUILTIN_XFD:
	    res = decrunch_xfd(*f, t);
	    break;
#endif
	}
    }

    if (res < 0) {
	D_(D_CRIT "failed");
	xmp_fclose(t);
	return -1;
    }

    D_(D_INFO "done");

    *f = t;
 
    if (!--ttl) {
	    return -1;
    }
    
    res = decrunch(f, &temp2, ttl);

    return res;
}


int xmp_test_module_f(xmp_file f, struct xmp_test_info *info)
{
	int file_size;
	char buf[XMP_NAME_SIZE];
	int i;
	int ret = -XMP_ERROR_FORMAT;;
	char *path = NULL;
	xmp_file fin = f;
	
	if (decrunch(&f, &path, DECRUNCH_MAX) < 0) {
		ret = -XMP_ERROR_DEPACK;
		goto err;
	}

	file_size = xmp_fsize(f);

	if (file_size < 256) {		/* set minimum valid module size */
		ret = -XMP_ERROR_FORMAT;
		goto err;
	}

	if (info != NULL) {
		*info->name = 0;	/* reset name prior to testing */
		*info->type = 0;	/* reset type prior to testing */
	}

	for (i = 0; format_loader[i] != NULL; i++) {
		xmp_fseek(f, 0, SEEK_SET);
		if (format_loader[i]->test(f, buf, 0) == 0) {
			int is_prowizard = 0;

			if (strcmp(format_loader[i]->name, "prowizard") == 0) {
			    xmp_fseek(f, 0, SEEK_SET);
				pw_test_format(f, buf, 0, info);
				is_prowizard = 1;
			}

			if (info != NULL && !is_prowizard) {
				strncpy(info->name, buf, XMP_NAME_SIZE);
				strncpy(info->type, format_loader[i]->name,
							XMP_NAME_SIZE);
			}
			ret = 0;
		}
	}

    err:
	if (f != fin)
	    xmp_fclose(f);
	return ret;
}

int xmp_test_module(char *path, struct xmp_test_info *info) 
{
	struct stat st;

	if (stat(path, &st) < 0)
		return -XMP_ERROR_SYSTEM;

	if (S_ISDIR(st.st_mode)) {
		errno = EISDIR;
		return -XMP_ERROR_SYSTEM;
	}

	xmp_file f = xmp_fopen(path, "rb");
	if (f == NULL) {
		return -XMP_ERROR_INTERNAL;
	}

	int err = xmp_test_module_f(f, info);

	xmp_fclose(f);

	return err;
}

/* FIXME: ugly code, check allocations */
static void split_name(char *s, char **d, char **b)
{
	char *div;
	int tmp;

	D_("alloc dirname/basename");
	if ((div = strrchr(s, '/'))) {
		tmp = div - s + 1;
		*d = malloc(tmp + 1);
		memcpy(*d, s, tmp);
		(*d)[tmp] = 0;
		*b = strdup(div + 1);
	} else {
		*d = strdup("");
		*b = strdup(s);
	}
}

int xmp_load_module_f(xmp_context opaque, xmp_file f, char *path)
{
	struct context_data *ctx = (struct context_data *)opaque;
	struct module_data *m = &ctx->m;
	int i;
	int file_size;
	int test_result, load_result;
	xmp_file fin = f;

	D_(D_WARN "path = %s", path);

	D_(D_INFO "decrunch");
	if (decrunch(&f, &path, DECRUNCH_MAX) < 0) {
		if (f != fin)
			xmp_fclose(f);
		return -XMP_ERROR_DEPACK;
	}

	file_size = xmp_fsize(f);

	if (file_size < 256) {			/* get size after decrunch */
		if (f != fin)
			xmp_fclose(f);
		return -XMP_ERROR_FORMAT;
	}

	split_name(path, &m->dirname, &m->basename);
	m->filename = strdup(path);	/* For ALM, SSMT, etc */
	m->size = file_size;

	load_prologue(ctx);

	D_(D_WARN "load");
	test_result = load_result = -1;
	for (i = 0; format_loader[i] != NULL; i++) {
		xmp_fseek(f, 0, SEEK_SET);
		test_result = format_loader[i]->test(f, NULL, 0);
		if (test_result == 0) {
			xmp_fseek(f, 0, SEEK_SET);
			D_(D_WARN "load format: %s", format_loader[i]->name);
			load_result = format_loader[i]->loader(m, f, 0);
			break;
		}
	}

	set_md5sum(f, m->md5);

	if (f != fin)
		xmp_fclose(f);

	if (test_result < 0) {
		free(m->basename);
		free(m->dirname);
		free(m->filename);
		return -XMP_ERROR_FORMAT;
	}

	if (load_result < 0) {
		xmp_release_module(opaque);
		return -XMP_ERROR_LOAD;
	}

	str_adj(m->mod.name);
	if (!*m->mod.name) {
		strncpy(m->mod.name, m->basename, XMP_NAME_SIZE);
	}

	load_epilogue(ctx);

	return 0;
}

int xmp_load_module(xmp_context opaque, char *path) 
{
	struct stat st;

	if (stat(path, &st) < 0)
		return -XMP_ERROR_SYSTEM;

	if (S_ISDIR(st.st_mode)) {
		errno = EISDIR;
		return -XMP_ERROR_SYSTEM;
	}

	xmp_file f = xmp_fopen(path, "rb");
	if (f == NULL) {
		return -XMP_ERROR_INTERNAL;
	}

	int err = xmp_load_module_f(opaque, f, path);

	xmp_fclose(f);

	return err;
}

void xmp_release_module(xmp_context opaque)
{
	struct context_data *ctx = (struct context_data *)opaque;
	struct module_data *m = &ctx->m;
	struct xmp_module *mod = &m->mod;
	int i;

	D_(D_INFO "Freeing memory");

	if (m->extra) {
		free(m->extra);
	}

	if (m->med_vol_table) {
		for (i = 0; i < mod->ins; i++)
			if (m->med_vol_table[i])
				free(m->med_vol_table[i]);
		free(m->med_vol_table);
	}

	if (m->med_wav_table) {
		for (i = 0; i < mod->ins; i++)
			if (m->med_wav_table[i])
				free(m->med_wav_table[i]);
		free(m->med_wav_table);
	}

	for (i = 0; i < mod->trk; i++) {
		free(mod->xxt[i]);
	}

	for (i = 0; i < mod->pat; i++) {
		free(mod->xxp[i]);
	}

	for (i = 0; i < mod->ins; i++) {
		free(mod->xxi[i].sub);
		if (mod->xxi[i].extra)
			free(mod->xxi[i].extra);
	}

	free(mod->xxt);
	free(mod->xxp);
	if (mod->smp > 0) {
		for (i = 0; i < mod->smp; i++) {
			free_sample(&mod->xxs[i]);
		}
		free(mod->xxs);
	}
	free(mod->xxi);
	if (m->comment) {
		free(m->comment);
	}

	D_("free dirname/basename");
	free(m->dirname);
	free(m->basename);
	free(m->filename);
}

void xmp_scan_module(xmp_context opaque)
{
	struct context_data *ctx = (struct context_data *)opaque;

	scan_sequences(ctx);
}
