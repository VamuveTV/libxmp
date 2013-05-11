#include "test.h"

TEST(test_read_file_32bit_little_endian)
{
	xmp_file f;
	int x;

	f = xmp_fopen("data/test.mmcmp", "rb");
	fail_unless(f != NULL, "can't open data file");

	x = read32l(f);
	fail_unless(x == 0x4352697a, "read error");

	xmp_fclose(f);
}
END_TEST
