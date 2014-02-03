# Base64 stream encoder/decoder in C89

This is an implementation of a base64 stream encoder/decoder in C89. It also
contains wrapper functions to encode/decode simple length-delimited strings.

Notable features:

- Reads/writes blocks of streaming data;
- Does not dynamically allocate memory;
- Valid C89 that compiles with pedantic options on;
- Unit tested;
- Fairly fast;
- Uses Duff's Device.

The stream encoder/decoders are not reentrant, they keep a few bytes of state
locked up within themselves. This was a conscious decision, because it makes
the API simpler. If we wanted to make the functions reentrant, the caller would
need to allocate a struct containing the state variables, at the expense of
cluttering the API.

Another conscious decision is to not zero-terminate the output. Instead,
strings are represented as a pointer and a length. In the decoding step,
relying on zero-termination would make no sense since the output could contain
legitimate zero bytes. In the encoding step, returning the length saves the
overhead of calling strlen() on the output. If you insist on the trailing zero,
you can easily add it yourself at the given offset.

The API itself is documented in `base64.h`.

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

	base64_stream_encode_init();
	while ((nread = fread(buf, 1, sizeof(buf), stdin)) > 0) {
		base64_stream_encode(buf, nread, out, &nout);
		if (nout) fwrite(out, nout, 1, stdout);
		if (feof(stdin)) break;
	}
	base64_stream_encode_final(out, &nout);
	if (nout) fwrite(out, nout, 1, stdout);

	return 0;
}
```

Also see `main.c` for a simple re-implementation of the `base64` utility. A
file or standard input is fed through the encoder/decoder, and the output is
written to standard output.

## Tests

See `tests/` for a small test suite.

## License

This repository is licensed under the
[BSD 2-clause License](http://opensource.org/licenses/BSD-2-Clause). See the
LICENSE file.
