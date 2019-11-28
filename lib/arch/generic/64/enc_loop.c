static inline void
enc_loop_generic_64_inner (const uint8_t **s, uint8_t **o)
{
	uint64_t src;

	// Load input:
	memcpy(&src, *s, sizeof (src));

	// Reorder to 64-bit big-endian, if not already in that format. The
	// workset must be in big-endian, otherwise the shifted bits do not
	// carry over properly among adjacent bytes:
	src = BASE64_HTOBE64(src);

	// Shift input by 6 bytes each round and mask in only the lower 6 bits;
	// look up the character in the Base64 encoding table and write it to
	// the output location:
	*(*o)++ = base64_table_enc[(src >> 58) & 0x3F];
	*(*o)++ = base64_table_enc[(src >> 52) & 0x3F];
	*(*o)++ = base64_table_enc[(src >> 46) & 0x3F];
	*(*o)++ = base64_table_enc[(src >> 40) & 0x3F];
	*(*o)++ = base64_table_enc[(src >> 34) & 0x3F];
	*(*o)++ = base64_table_enc[(src >> 28) & 0x3F];
	*(*o)++ = base64_table_enc[(src >> 22) & 0x3F];
	*(*o)++ = base64_table_enc[(src >> 16) & 0x3F];

	*s += 6;
}

static inline void
enc_loop_generic_64 (const uint8_t **s, size_t *slen, uint8_t **o, size_t *olen)
{
	if (*slen < 8) {
		return;
	}

	// Process blocks of 6 bytes at a time. Because blocks are loaded 8
	// bytes at a time, ensure that there will be at least 2 remaining
	// bytes after the last round, so that the final read will not pass
	// beyond the bounds of the input buffer:
	size_t rounds = (*slen - 2) / 6;

	*slen -= rounds * 6;	// 6 bytes consumed per round
	*olen += rounds * 8;	// 8 bytes produced per round

	do {
		if (rounds >= 8) {
			enc_loop_generic_64_inner(s, o);
			enc_loop_generic_64_inner(s, o);
			enc_loop_generic_64_inner(s, o);
			enc_loop_generic_64_inner(s, o);
			enc_loop_generic_64_inner(s, o);
			enc_loop_generic_64_inner(s, o);
			enc_loop_generic_64_inner(s, o);
			enc_loop_generic_64_inner(s, o);
			rounds -= 8;
			continue;
		}
		if (rounds >= 4) {
			enc_loop_generic_64_inner(s, o);
			enc_loop_generic_64_inner(s, o);
			enc_loop_generic_64_inner(s, o);
			enc_loop_generic_64_inner(s, o);
			rounds -= 4;
			continue;
		}
		if (rounds >= 2) {
			enc_loop_generic_64_inner(s, o);
			enc_loop_generic_64_inner(s, o);
			rounds -= 2;
			continue;
		}
		enc_loop_generic_64_inner(s, o);
		break;

	} while (rounds > 0);
}
