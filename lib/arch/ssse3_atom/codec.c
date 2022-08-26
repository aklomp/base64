#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>

#include "../../../include/libbase64.h"
#include "../../tables/tables.h"
#include "../../codecs.h"
#include "config.h"
#include "../../env.h"

#if HAVE_SSSE3
#include <tmmintrin.h>

#include "../sse2/compare_macros.h"

#include "dec_reshuffle.c"
#include "enc_reshuffle.c"
#include "enc_translate.c"

#endif	// __SSSE3__

BASE64_ENC_FUNCTION(ssse3_atom)
{
#if HAVE_SSSE3
	#include "enc_head.c"
	#include "enc_loop.c"
	#include "enc_tail.c"
#else
	BASE64_ENC_STUB
#endif
}

BASE64_DEC_FUNCTION(ssse3_atom)
{
#if HAVE_SSSE3
	#include "dec_head.c"
	#include "dec_loop.c"
	#include "dec_tail.c"
#else
	BASE64_DEC_STUB
#endif
}
