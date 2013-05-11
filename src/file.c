/* Extended Module Player
 * Copyright (C) 2013 Jesper Hansen <jesper@jesperhansen.net>
 *
 * This file is part of the Extended Module Player and is distributed
 * under the terms of the GNU Lesser General Public License. See COPYING.LIB
 * for more information.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "xmp.h"

///////////////////////////////////////////////////////////////////////////////
// base functionality all file objects must implement.

typedef void (*close_func)(struct xmp_file_t *f);
typedef int (*read_func)(struct xmp_file_t *f, void *dst, unsigned int size);
typedef int (*write_func)(struct xmp_file_t *f, const void *src, unsigned int size);
typedef int (*seek_func)(struct xmp_file_t *f, int offset, int whence);

struct xmp_file_t
{
	int at_eof;
	close_func close;
	read_func read;
	write_func write;
	seek_func seek;
};

///////////////////////////////////////////////////////////////////////////////

typedef struct xmp_cfile_file_t
{
	struct xmp_file_t base;
	FILE *file;
} xmp_cfile_file;

void xmp_cfile_close(xmp_cfile_file *f)
{
	fclose(f->file);
	free(f);
}

int xmp_cfile_read(xmp_cfile_file *f, void *dst, unsigned int size)
{
	return fread(dst, 1, size, f->file);
}

int xmp_cfile_write(xmp_cfile_file *f, const void *src, unsigned int size)
{
	return fwrite(src, 1, size, f->file);
}

int xmp_cfile_seek(xmp_cfile_file *f, int offset, int whence)
{
	fseek(f->file, offset, whence);
	return ftell(f->file);
}

xmp_file xmp_fopen(const char *filename, const char *mode)
{
	FILE *ff = fopen(filename, mode);
	if(ff == NULL)
		return NULL;

	xmp_cfile_file *f = malloc(sizeof(*f));
	f->file = ff;
	f->base.at_eof = 0;
	f->base.close = (close_func)xmp_cfile_close;
	f->base.read = (read_func)xmp_cfile_read;
	f->base.write = (write_func)xmp_cfile_write;
	f->base.seek = (seek_func)xmp_cfile_seek;

	return (xmp_file)f;
}

///////////////////////////////////////////////////////////////////////////////

typedef struct xmp_mem_file_t
{
	struct xmp_file_t base;
	unsigned char *data;
	unsigned int size;
	unsigned int capacity; // if 0, then we don't own the data, but has borrowed it.
	unsigned int offset;
} xmp_mem_file;

void xmp_mem_close(xmp_mem_file *f)
{
	if(f->capacity > 0)
		free(f->data);
	free(f);
}

int xmp_mem_read(xmp_mem_file *f, void *dst, unsigned int size)
{
	if(f->offset >= f->size)
		return 0;

	if(size > f->size - f->offset)
		size = f->size - f->offset;

	memcpy(dst, f->data + f->offset, size);
	f->offset += size;

	return size;
}

int xmp_mem_write(xmp_mem_file *f, const void *src, unsigned int size)
{
	int new_capacity = f->capacity;
	if(f->offset + size > new_capacity)
		new_capacity = (f->offset + size + 4095)&~4095;

	if(f->capacity == 0)
	{
		// we have only borrowed the data, so need to malloc a new buffer
		void *new_data = malloc(new_capacity);
		if(new_data == NULL)
			return 0;
		memcpy(new_data, f->data, f->size);
		f->data = new_data;
		f->capacity = new_capacity;
	}
	else if(new_capacity > f->capacity)
	{
		void *new_data = realloc(f->data, new_capacity);
		if(new_data == NULL)
			return 0;
		f->data = new_data;
		f->capacity = new_capacity;
	}

	memcpy(f->data + f->offset, src, size);

	f->offset += size;

	if(f->offset > f->size)
		f->size = f->offset;

	return size;
}

int xmp_mem_seek(xmp_mem_file *f, int offset, int whence)
{
	unsigned int base;
	switch(whence)
	{
	case SEEK_SET: base = 0; break;
	case SEEK_CUR: base = f->offset; break;
	case SEEK_END: base = f->size; break;
	default: return -1;
	}

	if(offset < 0 && -offset>base)
		f->offset = 0;
	else
		f->offset = base + offset;

	return f->offset;
}

xmp_file xmp_fopen_mem(const void *data, unsigned int size)
{
	xmp_mem_file *f = malloc(sizeof(*f));
	f->data = (unsigned char*)data;
	f->size = size;
	f->capacity = 0;
	f->offset = 0;
	f->base.at_eof = 0;
	f->base.close = (close_func)xmp_mem_close;
	f->base.read = (read_func)xmp_mem_read;
	f->base.write = (write_func)xmp_mem_write;
	f->base.seek = (seek_func)xmp_mem_seek;

	return (xmp_file)f;
}

///////////////////////////////////////////////////////////////////////////////

void xmp_fclose(xmp_file f)
{
	f->close(f);
}

int xmp_fread(void *dst, unsigned int size, unsigned int nmemb, xmp_file f)
{
	int read_size = f->read(f, dst, size*nmemb);

	if(read_size < size*nmemb)
		f->at_eof = 1;

	return read_size/size; // not a 100% api match in case of partial read
}

int xmp_fwrite(const void *src, unsigned int size, unsigned int nmemb, xmp_file f)
{
	return f->write(f, src, size*nmemb)/size; // not a 100% api match
}

int xmp_fsize(xmp_file f)
{
	int cur_pos = f->seek(f, 0, SEEK_CUR);
	int file_size = f->seek(f, 0, SEEK_END);
	f->seek(f, cur_pos, SEEK_SET);
	return file_size;
}

int xmp_ftell(xmp_file f)
{
	return f->seek(f, 0, SEEK_CUR);
}

int xmp_fseek(xmp_file f, int offset, int whence)
{
	f->at_eof = 0;
	f->seek(f, offset, whence);
	return 0;
}

int xmp_feof(xmp_file f)
{
	return f->at_eof;
}

int xmp_fputc(int c, xmp_file f)
{
	if(f->write(f, &c, 1) != 1)
		return EOF;
	return c;
}

int xmp_fgetc(xmp_file f)
{
	unsigned char c;
	if(f->read(f, &c, 1) != 1)
		return -1;
	return c;
}

// cheat; just seek one char backwards
int xmp_fungetc(int c, xmp_file f)
{
	f->seek(f, -1, SEEK_CUR);
	return c;
}
