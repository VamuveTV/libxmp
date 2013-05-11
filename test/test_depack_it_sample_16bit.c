#include "test.h"
#include "../src/loaders/loader.h"

int itsex_decompress16(xmp_file module, void *dst, int len, char it215);


TEST(test_depack_it_sample_16bit)
{
	xmp_file f, fo;
	int ret;
	char dest[10000];

	f = xmp_fopen("data/it-sample-16bit.raw", "rb");
	fail_unless(f != NULL, "can't open data file");

	fo = xmp_fopen(TMP_FILE, "wb");
	fail_unless(fo != NULL, "can't open output file");

	ret = itsex_decompress16(f, dest, 4646, 0);
	fail_unless(ret == 0, "decompression fail");

	if (is_big_endian()) {
		convert_endian((unsigned char *)dest, 4646);
	}

	xmp_fwrite(dest, 1, 9292, fo);

	xmp_fclose(fo);
	xmp_fclose(f);

	ret = check_md5(TMP_FILE, "1e2395653f9bd7838006572d8fcdb646");
	fail_unless(ret == 0, "MD5 error");
}
END_TEST
