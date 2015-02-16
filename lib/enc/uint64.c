/* If we have 64-bit ints, pick off 6 bytes at a
 * time for as long as we can: */
while (srclen >= 6)
{
	/* Mask to pass through only the lower 6 bits of one byte: */
	uint64_t mask = 0x3F00000000000000;

	/* Load string: */
	uint64_t str = *(uint64_t *)c;

	/* Reorder to 64-bit big-endian.
	 * The workset must be in big-endian, otherwise the shifted bits
	 * do not carry over properly among adjacent bytes: */
	str = __builtin_bswap64(str);

	/* Shift bits by 2, each time choosing just one byte to include.
	 * For each byte, lookup its character in the Base64 encoding table: */
	*o++ = base64_table_enc[((str & (mask <<  2)) >> 58)];
	*o++ = base64_table_enc[((str & (mask >>  4)) >> 52)];
	*o++ = base64_table_enc[((str & (mask >> 10)) >> 46)];
	*o++ = base64_table_enc[((str & (mask >> 16)) >> 40)];
	*o++ = base64_table_enc[((str & (mask >> 22)) >> 34)];
	*o++ = base64_table_enc[((str & (mask >> 28)) >> 28)];
	*o++ = base64_table_enc[((str & (mask >> 34)) >> 22)];
	*o++ = base64_table_enc[((str & (mask >> 40)) >> 16)];

	c += 6;		/* 6 bytes of input  */
	outl += 8;	/* 8 bytes of output */
	srclen -= 6;
}
