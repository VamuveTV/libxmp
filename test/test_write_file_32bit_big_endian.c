#include "test.h"

TEST(test_write_file_32bit_big_endian)
{
	xmp_file f;
	int x;

	f = xmp_fopen("write_test", "wb");
	fail_unless(f != NULL, "can't open data file");

	write32b(f, 0x12345678);
	xmp_fclose(f);

	f = xmp_fopen("write_test", "rb");
	x = read32b(f);
	fail_unless(x == 0x12345678, "read error");

	xmp_fclose(f);
}
END_TEST
