# Base64 stream encoder/decoder in C89

This is an implementation of a base64 stream encoder/decoder in C89. It also
contains wrapper functions to encode/decode simple length-delimited strings.

If your processor supports SSSE3, encoding/decoding speed is about four times
higher than the competition, because this library uses SSE intrinsics to
encode/decode twelve bytes at a time. To the author's knowledge, this is the
only Base64 library that does this.

Notable features:

- Really fast on x86 systems by using SSE vector instructions;
- Reads/writes blocks of streaming data;
- Does not dynamically allocate memory;
- Valid C89 that compiles with pedantic options on;
- Re-entrant and threadsafe;
- Unit tested;
- Uses Duff's Device.

Strings are represented as a pointer and a length; they are not
zero-terminated. This was a conscious design decision. In the decoding step,
relying on zero-termination would make no sense since the output could contain
legitimate zero bytes. In the encoding step, returning the length saves the
overhead of calling strlen() on the output. If you insist on the trailing zero,
you can easily add it yourself at the given offset.

## Usage and building

To use the library routines, include `base64.c` and `base64.h` into your
project. No special compiler flags are necessary to build the files. If your
target supports it, compile with `-mssse3` or equivalent to benefit from SSSE3
acceleration.

## API reference

### Encoding

#### base64_encode

```c
void base64_encode (const char *const src, size_t srclen, char *const out, size_t *const outlen);
```

Wrapper function to encode a plain string of given length.
Output is written to `out` without trailing zero.
Output length in bytes is written to `outlen`.
The buffer in `out` has been allocated by the caller and is at least 4/3 the size of the input.

#### base64_stream_encode_init

```c
void base64_stream_encode_init (struct base64_state *);
```

Call this before calling `base64_stream_encode()` to init the state.

#### base64_stream_encode

```c
void base64_stream_encode (struct base64_state *, const char *const src, size_t srclen, char *const out, size_t *const outlen);
```

Encodes the block of data of given length at `src`, into the buffer at `out`.
Caller is responsible for allocating a large enough out-buffer; it must be at least 4/3 the size of the in-buffer, but take some margin.
Places the number of new bytes written into `outlen` (which is set to zero when the function starts).
Does not zero-terminate or finalize the output.

#### base64_stream_encode_final

```c
void base64_stream_encode_final (struct base64_state *, char *const out, size_t *outlen);
```

Finalizes the output begun by previous calls to `base64_stream_encode()`.
Adds the required end-of-stream markers if appropriate.
`outlen` is modified and will contain the number of new bytes written at `out` (which will quite often be zero).

### Decoding

#### base64_decode

```c
int base64_decode (const char *const src, size_t srclen, char *const out, size_t *const outlen);
```

Wrapper function to decode a plain string of given length.
Output is written to `out` without trailing zero. Output length in bytes is written to `outlen`.
The buffer in `out` has been allocated by the caller and is at least 3/4 the size of the input.

#### base64_stream_decode_init

```c
void base64_stream_decode_init (struct base64_state *);
```

Call this before calling `base64_stream_decode()` to init the state.

#### base64_stream_decode

```c
int base64_stream_decode (struct base64_state *, const char *const src, size_t srclen, char *const out, size_t *const outlen);
```

Decodes the block of data of given length at `src`, into the buffer at `out`.
Caller is responsible for allocating a large enough out-buffer; it must be at least 3/4 the size of the in-buffer, but take some margin.
Places the number of new bytes written into `outlen` (which is set to zero when the function starts).
Does not zero-terminate the output.
Returns 1 if all is well, and 0 if a decoding error was found, such as an invalid character.

## Examples

A simple example of encoding a static string to base64 and printing the output
to stdout:

```c
#include <stdio.h>	/* fwrite */
#include "base64.h"

int main ()
{
	char src[] = "hello world";
	char out[20];
	size_t srclen = sizeof(src) - 1;
	size_t outlen;

	base64_encode(src, srclen, out, &outlen);

	fwrite(out, outlen, 1, stdout);

	return 0;
}
```

A simple example (no error checking, etc) of stream encoding standard input to
standard output:

```c
#include <stdio.h>
#include "base64.h"

int main ()
{
	size_t nread, nout;
	char buf[12000], out[16000];
	struct base64_state state;

	base64_stream_encode_init(&state);
	while ((nread = fread(buf, 1, sizeof(buf), stdin)) > 0) {
		base64_stream_encode(&state, buf, nread, out, &nout);
		if (nout) fwrite(out, nout, 1, stdout);
		if (feof(stdin)) break;
	}
	base64_stream_encode_final(&state, out, &nout);
	if (nout) fwrite(out, nout, 1, stdout);

	return 0;
}
```

Also see `main.c` for a simple re-implementation of the `base64` utility. A
file or standard input is fed through the encoder/decoder, and the output is
written to standard output.

## Tests

See `tests/` for a small test suite. Testing is automated with [Travis CI](https://travis-ci.org/aklomp/base64):

[![Build Status](https://travis-ci.org/aklomp/base64.png?branch=master)](https://travis-ci.org/aklomp/base64)

## License

This repository is licensed under the
[BSD 2-clause License](http://opensource.org/licenses/BSD-2-Clause). See the
LICENSE file.
