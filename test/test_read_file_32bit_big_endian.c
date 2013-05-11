#include "test.h"

TEST(test_read_file_32bit_big_endian)
{
	xmp_file f;
	int x;

	f = xmp_fopen("data/test.mmcmp", "rb");
	fail_unless(f != NULL, "can't open data file");

	x = read32b(f);
	fail_unless(x == 0x7a695243, "read error");

	xmp_fclose(f);
}
END_TEST
