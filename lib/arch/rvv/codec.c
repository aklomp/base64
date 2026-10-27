#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "../../../include/libbase64.h"
#include "../../tables/tables.h"
#include "../../codecs.h"
#include "config.h"
#include "../../env.h"

#if HAVE_RVV
#include <riscv_vector.h>

#include "../generic/32/dec_loop.c"
#include "../generic/64/enc_loop.c"
#include "dec_loop.c"
#include "enc_loop.c"

#endif	// HAVE_RVV

// The RVV loops are vector-length agnostic: each iteration processes as
// many 3-byte (encode) or 4-byte (decode) groups as fit in the vector
// registers, so they handle the whole input down to the last complete
// group. The generic uint64 codec is still included to keep the codec
// structure identical to the other SIMD codecs; it only runs on the few
// bytes the vector loop leaves behind.

void
base64_stream_encode_rvv BASE64_ENC_PARAMS
{
#if HAVE_RVV
	#include "../generic/enc_head.c"
	enc_loop_rvv(&s, &slen, &o, &olen);
	enc_loop_generic_64(&s, &slen, &o, &olen);
	#include "../generic/enc_tail.c"
#else
	base64_enc_stub(state, src, srclen, out, outlen);
#endif
}

int
base64_stream_decode_rvv BASE64_DEC_PARAMS
{
#if HAVE_RVV
	#include "../generic/dec_head.c"
	dec_loop_rvv(&s, &slen, &o, &olen);
	dec_loop_generic_32(&s, &slen, &o, &olen);
	#include "../generic/dec_tail.c"
#else
	return base64_dec_stub(state, src, srclen, out, outlen);
#endif
}
