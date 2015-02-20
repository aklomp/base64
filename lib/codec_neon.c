#include <stddef.h>
#ifdef __ARM_NEON__
#include <arm_neon.h>
#endif

#include "../include/libbase64.h"

extern const char base64_table_enc[];
extern const unsigned char base64_table_dec[];

#ifdef __ARM_NEON__
/* With this transposed encoding table, we can use
 * two 32-byte lookups to do the encoding. Read the
 * tables top to bottom, left to right. */
static const char *base64_table_enc_transposed[2] =
{
	"AIQY"
	"BJRZ"
	"CKSa"
	"DLTb"
	"EMUc"
	"FNVd"
	"GOWe"
	"HPXf"
,
	"gow4"
	"hpx5"
	"iqy6"
	"jrz7"
	"ks08"
	"lt19"
	"mu2+"
	"nv3/"
};
#endif

void
base64_stream_encode_neon (struct base64_state *state, const char *const src, size_t srclen, char *const out, size_t *const outlen)
{
#ifdef __ARM_NEON__
	uint8x8x4_t tbl_enc_lo = vld4_u8((uint8_t *)base64_table_enc_transposed[0]);
	uint8x8x4_t tbl_enc_hi = vld4_u8((uint8_t *)base64_table_enc_transposed[1]);

	#include "enc/head.c"
	#include "enc/neon.c"
	#include "enc/tail.c"
#else
	(void)state;
	(void)src;
	(void)srclen;
	(void)out;
	(void)outlen;
#endif
}

int
base64_stream_decode_neon (struct base64_state *state, const char *const src, size_t srclen, char *const out, size_t *const outlen)
{
#ifdef __ARM_NEON__
	#include "dec/head.c"
	#include "dec/neon.c"
	#include "dec/tail.c"
#else
	(void)state;
	(void)src;
	(void)srclen;
	(void)out;
	(void)outlen;

	return 0;
#endif
}
