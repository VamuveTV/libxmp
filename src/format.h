#ifndef XMP_FORMAT_H
#define XMP_FORMAT_H

#include <stdio.h>
#include "common.h"

#define MAX_FORMATS 110

struct format_loader {
	const char *name;
	int (*const test)(xmp_file, char *, const int);
	int (*const loader)(struct module_data *, xmp_file, const int);
};

char **format_list(void);
int pw_test_format(xmp_file, char *, const int, struct xmp_test_info *);

#endif

