#ifndef _BASE64_H
#define _BASE64_H

struct base64_state {
	int eof;
	int bytes;
	unsigned char carry;
};

/* Wrapper function to encode a plain string of given length. Output is written
 * to *out without trailing zero. Output length in bytes is written to *outlen.
 * The buffer in `out` has been allocated by the caller and is at least 4/3 the
 * size of the input: */
void base64_encode (const char *const src, size_t srclen, char *const out, size_t *const outlen);

/* Call this before calling base64_stream_encode() to init the state: */
void base64_stream_encode_init (struct base64_state *);

/* Encodes the block of data of given length at `src`, into the buffer at
 * `out`. Caller is responsible for allocating a large enough out-buffer; it
 * must be at least 4/3 the size of the in-buffer, but take some margin. Places
 * the number of new bytes written into `outlen` (which is set to zero when the
 * function starts). Does not zero-terminate or finalize the output. */
void base64_stream_encode (struct base64_state *, const char *const src, size_t srclen, char *const out, size_t *const outlen);

/* Finalizes the output begun by previous calls to `base64_stream_encode()`.
 * Adds the required end-of-stream markers if appropriate. `outlen` is modified
 * and will contain the number of new bytes written at `out` (which will quite
 * often be zero). */
void base64_stream_encode_final (struct base64_state *, char *const out, size_t *outlen);

/* Wrapper function to decode a plain string of given length. Output is written
 * to *out without trailing zero. Output length in bytes is written to *outlen.
 * The buffer in `out` has been allocated by the caller and is at least 3/4 the
 * size of the input. */
int base64_decode (const char *const src, size_t srclen, char *const out, size_t *const outlen);

/* Call this before calling base64_stream_decode() to init the state: */
void base64_stream_decode_init (struct base64_state *);

/* Decodes the block of data of given length at `src`, into the buffer at
 * `out`. Caller is responsible for allocating a large enough out-buffer; it
 * must be at least 3/4 the size of the in-buffer, but take some margin. Places
 * the number of new bytes written into `outlen` (which is set to zero when the
 * function starts). Does not zero-terminate the output. Returns 1 if all is
 * well, and 0 if a decoding error was found, such as an invalid character. */
int base64_stream_decode (struct base64_state *, const char *const src, size_t srclen, char *const out, size_t *const outlen);

#endif /* _BASE64_H */
