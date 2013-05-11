#include "test.h"

int itsex_decompress8(xmp_file module, void *dst, int len, char it215);


TEST(test_depack_it_sample_8bit)
{
	xmp_file f, fo;
	int ret;
	char dest[10000];

	f = xmp_fopen("data/it-sample-8bit.raw", "rb");
	fail_unless(f != NULL, "can't open data file");

	fo = xmp_fopen(TMP_FILE, "wb");
	fail_unless(fo != NULL, "can't open output file");

	ret = itsex_decompress8(f, dest, 4879, 0);
	fail_unless(ret == 0, "decompression fail");
	xmp_fwrite(dest, 1, 4879, fo);

	xmp_fclose(fo);
	xmp_fclose(f);

	ret = check_md5(TMP_FILE, "299c9144ae2349b90b430aafde8d799a");
	fail_unless(ret == 0, "MD5 error");
}
END_TEST
