#include "test.h"


TEST(test_depack_vorbis)
{
	FILE *f;
	struct stat st;
	int i, ret;
	int16 *buf, *pcm16;
	xmp_context c;
	struct xmp_module_info info;

	c = xmp_create_context();
	fail_unless(c != NULL, "can't create context");

	ret = xmp_load_module(c, "data/beep.oxm");
	fail_unless(ret == 0, "can't load module");

	xmp_start_player(c, 44100, 0);
	xmp_get_module_info(c, &info);

	//for(i=0; i<)

	fail_unless(info.mod->xxs[0].len == 9376, "decompressed data has unexpected size");

	stat("data/beep.raw", &st);
	fail_unless(st.st_size == 2*9376, "raw data has unexpected size");
	f = fopen("data/beep.raw", "rb");
	fail_unless(f != NULL, "can't open raw data file");

	buf = malloc(st.st_size);
	fail_unless(buf != NULL, "can't alloc raw buffer");
	fread(buf, 1, st.st_size, f);

	pcm16 = (int16 *)info.mod->xxs[0].data;

	for (i = 0; i < 9376; i++) {
		if (pcm16[i] != buf[i])
			fail_unless(abs(pcm16[i] - buf[i]) <= 1, "data error");
	}

	free(buf);
	fclose(f);

	xmp_end_player(c);
	xmp_release_module(c);
	xmp_free_context(c);
}
END_TEST
