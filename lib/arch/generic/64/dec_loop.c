while (srclen > 4)
{
	union {
		uint32_t asint;
		uint8_t  aschar[4];
	} x;

	x.asint = base64_table_dec_d0[c[0]]
	        | base64_table_dec_d1[c[1]]
	        | base64_table_dec_d2[c[2]]
	        | base64_table_dec_d3[c[3]];

#if BASE64_LITTLE_ENDIAN
	if (x.asint & 0x8000000U) break;
#else
	if (x.asint & 1U) break;
#endif

#if HAVE_FAST_UNALIGNED_ACCESS
	*(uint32_t*)o = x.asint;
#else
	o[0] = x.aschar[0];
	o[1] = x.aschar[1];
	o[2] = x.aschar[2];
#endif

	c += 4;
	o += 3;
	outl += 3;
	srclen -= 4;
}
