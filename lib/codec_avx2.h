void base64_stream_encode_avx2 (struct base64_state *state, const char *const src, size_t srclen, char *const out, size_t *const outlen);
int  base64_stream_decode_avx2 (struct base64_state *state, const char *const src, size_t srclen, char *const out, size_t *const outlen);
