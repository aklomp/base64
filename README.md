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

The stream encoder/decoders made reentrant with BASE64_STATE.

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

## License

This repository is licensed under the
[BSD 2-clause License](http://opensource.org/licenses/BSD-2-Clause). See the
LICENSE file.
