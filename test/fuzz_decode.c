#include "../include/libbase64.h"
#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "fuzz_helpers.h"

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
  const uint8_t *data_cursor = data;
  int codec_flag = consume_codec_flags(&data_cursor, &size);

  char *decoded = malloc((size + 3) / 4 * 3);
  size_t decoded_length = 0;
  switch (base64_decode((char*) data_cursor, size, decoded, &decoded_length,
                        codec_flag)) {
  case 0:
    // We will very likely be passing in invalid inputs so this is ok.
    break;
  case 1:
    // Everything worked!
    break;
  case -1:
    // We may sometimes choose an unsupported codec so this is ok.
    break;
  default:
    // This should never happen.
    abort();
  }
  free(decoded);
  return 0;
}
