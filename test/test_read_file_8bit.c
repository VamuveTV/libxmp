#include "test.h"

TEST(test_read_file_8bit)
{
	xmp_file f;
	int x;

	f = xmp_fopen("data/test.mmcmp", "rb");
	fail_unless(f != NULL, "can't open data file");

	x = read8(f);
	fail_unless(x == 0x0000007a, "read error");

	xmp_fclose(f);
}
END_TEST
