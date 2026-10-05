#include <string.h>
#include <libbase64.h>

int
main (void)
{
	const char input[] = "hello world";
	const char expected[] = "aGVsbG8gd29ybGQ=";
	char encoded[sizeof(expected)], decoded[sizeof(input)];
	size_t encoded_size, decoded_size;

	base64_encode(input, sizeof(input) - 1, encoded, &encoded_size, 0);
	if (encoded_size != sizeof(expected) - 1 ||
	    memcmp(encoded, expected, encoded_size) != 0) {
		return 1;
	}
	if (base64_decode(encoded, encoded_size, decoded, &decoded_size, 0) != 1) {
		return 1;
	}
	return decoded_size != sizeof(input) - 1 ||
	       memcmp(decoded, input, decoded_size) != 0;
}
