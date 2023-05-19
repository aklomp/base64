#include "../include/libbase64.h"
#include "fuzz_helpers.h"
#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
  const uint8_t *data_cursor = data;
  int codec_flag = consume_codec_flags(&data_cursor, &size);

  // Create a buffer at least 4/3 the size of the data to encode;
  char *encoded = malloc((size + 2) / 3 * 4);
  size_t encoded_length = 0;
  base64_encode((char *)data_cursor, size, encoded, &encoded_length,
                codec_flag);

  char *decoded = malloc((encoded_length + 3) / 4 * 3);
  size_t decoded_length = 0;
  switch (base64_decode(encoded, encoded_length, decoded, &decoded_length,
                        codec_flag)) {
  case 0:
    // When using automatic codec detection we should never get a decode failure
    // due to invalid inputs as we encoded the data in the first place.
    assert(codec_flag != 0);
    break;
  case 1:
    // Everything worked!
    if (decoded_length != size) {
      free(encoded);
      free(decoded);
      printf(
          "Decoded length: %zu is not the same as the origional length %zu.\n",
          decoded_length, size);
      abort();
    }
    if (memcmp(decoded, data_cursor, decoded_length) != 0) {
      free(encoded);
      free(decoded);
      printf("Roundtrip encode->decode data does not equal original.\n");
      abort();
    }
    break;
  case -1:
    // We may sometimes choose an unsupported codec so this is ok.
    break;
  default:
    // This should never happen.
    abort();
  }
  free(encoded);
  free(decoded);
  return 0;
}
