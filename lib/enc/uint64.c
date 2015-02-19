/* If we have 64-bit ints, pick off 6 bytes at a
 * time for as long as we can: */
while (srclen >= 6)
{
	/* Load string: */
	uint64_t str = *(uint64_t *)c;

	/* Reorder to 64-bit big-endian.
	 * The workset must be in big-endian, otherwise the shifted bits
	 * do not carry over properly among adjacent bytes: */
	str = __builtin_bswap64(str);

	/* Shift input by 6 bytes each round and mask in only the
	 * lower 6 bits; look up the character in the Base64 encoding
	 * table and write it to the output location: */
	*o++ = base64_table_enc[(str >> 58) & 0x3F];
	*o++ = base64_table_enc[(str >> 52) & 0x3F];
	*o++ = base64_table_enc[(str >> 46) & 0x3F];
	*o++ = base64_table_enc[(str >> 40) & 0x3F];
	*o++ = base64_table_enc[(str >> 34) & 0x3F];
	*o++ = base64_table_enc[(str >> 28) & 0x3F];
	*o++ = base64_table_enc[(str >> 22) & 0x3F];
	*o++ = base64_table_enc[(str >> 16) & 0x3F];

	c += 6;		/* 6 bytes of input  */
	outl += 8;	/* 8 bytes of output */
	srclen -= 6;
}
