#include "test.h"

TEST(test_write_file_16bit_little_endian)
{
	xmp_file f;
	int x;

	f = xmp_fopen("write_test", "wb");
	fail_unless(f != NULL, "can't open data file");

	write16l(f, 0x1234);
	xmp_fclose(f);

	f = xmp_fopen("write_test", "rb");
	x = read16l(f);
	fail_unless(x == 0x1234, "read error");

	xmp_fclose(f);
}
END_TEST
