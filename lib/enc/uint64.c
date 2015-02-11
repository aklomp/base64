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
	*o++ = base64_table_enc[(((str >>  2) & (mask >>  0)) >> 56)];
	*o++ = base64_table_enc[(((str >>  4) & (mask >>  8)) >> 48) & 0xFF];
	*o++ = base64_table_enc[(((str >>  6) & (mask >> 16)) >> 40) & 0xFF];
	*o++ = base64_table_enc[(((str >>  8) & (mask >> 24)) >> 32) & 0xFF];
	*o++ = base64_table_enc[(((str >> 10) & (mask >> 32)) >> 24) & 0xFF];
	*o++ = base64_table_enc[(((str >> 12) & (mask >> 40)) >> 16) & 0xFF];
	*o++ = base64_table_enc[(((str >> 14) & (mask >> 48)) >>  8) & 0xFF];
	*o++ = base64_table_enc[(((str >> 16) & (mask >> 56)) >>  0) & 0xFF];

	c += 6;		/* 6 bytes of input  */
	outl += 8;	/* 8 bytes of output */
	srclen -= 6;
}
