#include "test.h"
#include "../src/loaders/loader.h"

#define SET(x,y,z,w) do { \
  s.len = (x); s.lps = (y); s.lpe = (z); s.flg = (w); s.data = NULL; \
} while (0)


TEST(test_sample_load_skip)
{
	struct xmp_sample s;
	xmp_file f;
	short buffer[202];
	int i;
	struct module_data m;

	memset(&m, 0, sizeof(struct module_data));

	f = xmp_fopen("data/sample-16bit.raw", "rb");
	fail_unless(f != NULL, "can't open sample file");

	/* read little-endian sample to native-endian buffer */
	for (i = 0; i < 101; i++) {
		buffer[i] = read16l(f);
	}
	for (i = 0; i < 101; i++) {
		buffer[101 + i] = buffer[101 - i - 1];
	}

	/* load sample from file */
	SET(101, 0, 102, XMP_SAMPLE_16BIT);
	xmp_fseek(f, 0, SEEK_SET);
	load_sample(&m, f, 0, &s, NULL);
	fail_unless(s.data != NULL, "didn't allocate sample data");
	fail_unless(s.lpe == 101, "didn't fix invalid loop end");
	fail_unless(memcmp(s.data, buffer, 202) == 0, "sample data error");
	free_sample(&s);

	/* disable sample load */
	SET(101, 0, 102, XMP_SAMPLE_16BIT);
	xmp_fseek(f, 0, SEEK_SET);
	m.smpctl |= XMP_SMPCTL_SKIP;
	load_sample(&m, f, 0, &s, NULL);
	fail_unless(s.data == NULL, "didn't skip sample load");
	free_sample(&s);

	xmp_fclose(f);
}
END_TEST
