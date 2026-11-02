#include "../include/libbase64.h"

// Function parameters for encoding functions:
#define BASE64_ENC_PARAMS			\
	( struct base64_state	*state		\
	, const char		*src		\
	, size_t		 srclen		\
	, char			*out		\
	, size_t		*outlen		\
	)

// Function parameters for decoding functions:
#define BASE64_DEC_PARAMS			\
	( struct base64_state	*state		\
	, const char		*src		\
	, size_t		 srclen		\
	, char			*out		\
	, size_t		*outlen		\
	)

// This function is used as a stub when a certain encoder is not compiled in.
// It discards the inputs and returns zero output bytes.
static inline void
base64_enc_stub BASE64_ENC_PARAMS
{
	(void) state;
	(void) src;
	(void) srclen;
	(void) out;

	*outlen = 0;
}

// This function is used as a stub when a certain decoder is not compiled in.
// It discards the inputs and returns an invalid decoding result.
static inline int
base64_dec_stub BASE64_DEC_PARAMS
{
	(void) state;
	(void) src;
	(void) srclen;
	(void) out;
	(void) outlen;

	return -1;
}

typedef void (* base64_enc_fn) BASE64_ENC_PARAMS;
typedef int  (* base64_dec_fn) BASE64_DEC_PARAMS;

struct codec
{
	base64_enc_fn enc;
	base64_dec_fn dec;
};

extern void codec_choose (struct codec *, int flags);

#if (__x86_64__ || __i386__ || _M_X86 || _M_X64)
enum
{
	X86_FEATURE_LEVEL_NONE,
	X86_FEATURE_LEVEL_SSSE3,
	X86_FEATURE_LEVEL_SSE41,
	X86_FEATURE_LEVEL_SSE42,
	X86_FEATURE_LEVEL_AVX,
	X86_FEATURE_LEVEL_AVX2,
	X86_FEATURE_LEVEL_AVX512,

	X86_FEATURE_LEVEL_COUNT
};

extern int x86_get_cpu_feature_level (void);
#endif // (__x86_64__ || __i386__ || _M_X86 || _M_X64)
