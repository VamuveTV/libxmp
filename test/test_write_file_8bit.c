#include "test.h"

TEST(test_write_file_8bit)
{
	xmp_file f;
	int x;

	f = xmp_fopen("write_test", "wb");
	fail_unless(f != NULL, "can't open data file");

	write8(f, 0x66);
	xmp_fclose(f);

	f = xmp_fopen("write_test", "rb");
	x = read8(f);
	fail_unless(x == 0x66, "read error");

	xmp_fclose(f);
}
END_TEST
