/* If we have 32-bit ints, pick off 3 bytes at a
 * time for as long as we can: */
while (srclen >= 3)
{
	/* Mask to pass through only the lower 6 bits of one byte: */
	uint32_t mask = 0x3F000000;

	/* Load string: */
	uint32_t str = *(uint32_t *)c;

	/* Reorder to 32-bit big-endian.
	 * The workset must be in big-endian, otherwise the shifted bits
	 * do not carry over properly among adjacent bytes: */
	str = __builtin_bswap32(str);

	/* Shift bits by 2, each time choosing just one byte to include.
	 * For each byte, lookup its character in the Base64 encoding table: */
	*o++ = base64_table_enc[((str & (mask <<  2)) >> 26)];
	*o++ = base64_table_enc[((str & (mask >>  4)) >> 20)];
	*o++ = base64_table_enc[((str & (mask >> 10)) >> 14)];
	*o++ = base64_table_enc[((str & (mask >> 16)) >>  8)];

	c += 3;		/* 3 bytes of input  */
	outl += 4;	/* 4 bytes of output */
	srclen -= 3;
}
