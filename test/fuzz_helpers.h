#include "../include/libbase64.h"
#include "string.h"
#include <stddef.h>
#include <stdint.h>

static inline uint64_t consume_uint64(const uint8_t **data_ptr, size_t *size) {
  uint64_t result = 0;
  if (*size < sizeof(result)) {
    return 0;
  }
  memcpy(&result, *data_ptr, sizeof(result));
  *data_ptr += sizeof(result);
  *size -= sizeof(result);
  return result;
}

static inline int consume_codec_flags(const uint8_t **data_ptr, size_t *size) {
  int valid_codec_flags[] = {
      0,
      BASE64_FORCE_AVX2,
      BASE64_FORCE_NEON32,
      BASE64_FORCE_NEON64,
      BASE64_FORCE_PLAIN,
      BASE64_FORCE_SSSE3,
      BASE64_FORCE_SSE41,
      BASE64_FORCE_SSE42,
      BASE64_FORCE_AVX,
      BASE64_FORCE_AVX512,
  };
  // Pick a flag from the valid codecs using the fuzzed data.
  return valid_codec_flags[consume_uint64(data_ptr, size) %
                           (sizeof(valid_codec_flags) /
                            sizeof(valid_codec_flags[0]))];
}
