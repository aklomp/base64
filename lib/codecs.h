#define BASE64_ENC_PARAMS			\
	( struct base64_state	*	state	\
	, const char		*const	src	\
	, size_t			srclen	\
	, char			*const	out	\
	, size_t		*const	outlen	\
	)

#define BASE64_DEC_PARAMS			\
	( struct base64_state	*	state	\
	, const char		*const	src	\
	, size_t			srclen	\
	, char			*const	out	\
	, size_t		*const	outlen	\
	)

#define UNUSED(x)		((void)(x))

#define BASE64_ENC_STUB				\
	UNUSED(state);				\
	UNUSED(src);				\
	UNUSED(srclen);				\
	UNUSED(out);				\
						\
	*outlen = 0;

#define BASE64_DEC_STUB				\
	UNUSED(state);				\
	UNUSED(src);				\
	UNUSED(srclen);				\
	UNUSED(out);				\
	UNUSED(outlen);				\
						\
	return -1;

/* Typedefs for encoder/decoder functions,
 * to be used in runtime feature detection: */
typedef void (* func_enc_t) BASE64_ENC_PARAMS;
typedef int  (* func_dec_t) BASE64_DEC_PARAMS;

struct codec
{
	func_enc_t enc;
	func_dec_t dec;
};

void codec_choose (struct codec *, int flags);

/* These tables are used by all codecs
 * for fallback plain encoding/decoding: */
extern const unsigned char base64_table_enc[];
extern const unsigned char base64_table_dec[];
