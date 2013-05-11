#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include "test.h"

static void load_tester(xmp_context ctx, const char *path)
{
	xmp_file f;
	void *moddata;
	size_t modsize;
	int ret;

	f = xmp_fopen(path, "rb");
	modsize = xmp_fsize(f);
	moddata = malloc(modsize);
	xmp_fread(moddata, 1, modsize, f);
	xmp_fclose(f);
	
	f = xmp_fopen_mem(moddata, modsize);
	ret = xmp_load_module_f(ctx, f, "data/unknown");
	fail_unless(ret == 0, "load file");
	xmp_fclose(f);
	xmp_release_module(ctx);
	
	free(moddata);
}

TEST(test_api_load_module_from_mem)
{
	xmp_context ctx;
	
	ctx = xmp_create_context();

	// uncompressed
	load_tester(ctx, "data/test.xm");
	// and compressed
	load_tester(ctx, "data/gzipdata");

	xmp_free_context(ctx);
}
END_TEST
