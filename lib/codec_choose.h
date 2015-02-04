/* Typedefs for encoder/decoder functions, to be used in runtime feature detection: */
typedef void (* func_enc_t) (struct base64_state *state, const char *const src, size_t srclen, char *const out, size_t *const outlen);
typedef int  (* func_dec_t) (struct base64_state *state, const char *const src, size_t srclen, char *const out, size_t *const outlen);

struct codec
{
	func_enc_t enc;
	func_dec_t dec;
};

void codec_choose (struct codec *);
