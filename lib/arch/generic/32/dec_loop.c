// Read source 4 bytes at a time
// Since we might be writing one byte more than needed,
// we need to make sure there will still be some room
// for one extra byte in o.
// This will be the case if srclen > 0 when the loop
// is exited
while (srclen > 4)
{
	const uint32_t str
		= base64_table_dec_d0[c[0]]
	        | base64_table_dec_d1[c[1]]
	        | base64_table_dec_d2[c[2]]
	        | base64_table_dec_d3[c[3]];

#if BASE64_LITTLE_ENDIAN
	// LUTs for little-endian set Most Significant Bit
	// in case of invalid character
	if (str & UINT32_C(0x80000000)) {
		break;
	}
#else
	// LUTs for big-endian set Least Significant Bit
	// in case of invalid character
	if (str & UINT32_C(1)) {
		break;
	}
#endif

	// Store:
	memcpy(o, &str, sizeof (str));

	c += 4;
	o += 3;
	outl += 3;
	srclen -= 4;
}
