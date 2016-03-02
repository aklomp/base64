// This code makes some assumptions on the implementation of 
// base64_stream_encode_init(), base64_stream_encode() and base64_stream_decode().
// Basically these assumptions boil down to that when breaking the src into
// parts, out parts can be written without side effects.
// This is met when:
// 1) base64_stream_encode() and base64_stream_decode() don't use globals;
// 2) the shared variables src and out are not read or written outside of the
//    bounds of their parts, i.e.  when base64_stream_encode() reads a multiple
//    of 3 bytes, it must write no more then a multiple of 4 bytes, not even
//    temporarily;
// 3) the state flag can be discarded after base64_stream_encode() and
//    base64_stream_decode() on the parts.

// Due to the overhead of initializing Openmp and creating a team of threads,
// we require the data length to be larger than a threshold:
#define OMP_THRESHOLD 20000

void
base64_encode
	( const char	*src
	, size_t	 srclen
	, char		*out
	, size_t	*outlen
	, int		 flags
	)
{
	size_t s;
	size_t t;
	size_t sum = 0, len, last_len;
	struct base64_state state, initial_state;   
	int num_threads;

	if (srclen > OMP_THRESHOLD) {

		// Request a number of threads but not necessarily get them:
		#pragma omp parallel
		{
			// Get the number of threads used from one thread only,
			// as num_threads is a shared var:
			#pragma omp single
			{
				num_threads = omp_get_num_threads();

				// Split the input string into num_threads
				// parts, each part a multiple of 3 bytes.
				// The remaining bytes will be done later:
				len = srclen / (num_threads * 3);
				len *= 3;
				last_len = srclen - num_threads * len;

				// Init the stream reader:
				base64_stream_encode_init(&state, flags);
				initial_state = state;
			}

			// Single has an implicit barrier for all threads to
			// wait here for the above to complete:
			#pragma omp for firstprivate(state) private(s) reduction(+:sum) schedule(static,1)
			for (int i = 0; i < num_threads; i++)
			{
				// Feed each part of the string to the stream reader:
				base64_stream_encode(&state, src + i * len, len, out + i * len * 4 / 3, &s);
				sum += s;
			}
		}

		// As encoding should never fail and we encode an exact
		// multiple of 3 bytes, we can discard state:
		state = initial_state;

		// Encode the remaining bytes:
		base64_stream_encode(&state, src + num_threads * len, last_len, out + num_threads * len * 4 / 3, &s);

		// Finalize the stream by writing trailer if any:
		base64_stream_encode_final(&state, out + num_threads * len * 4 / 3 + s, &t);

		// Final output length is stream length plus tail:
	} else {
		base64_stream_encode_init(&state, flags);
		base64_stream_encode(&state, src, srclen, out, &s);
		base64_stream_encode_final(&state, out + s, &t);
	}
	sum += s + t;
	*outlen = sum;
}

int
base64_decode
	( const char	*src
	, size_t	 srclen
	, char		*out
	, size_t	*outlen
	, int		 flags
	)
{
	int num_threads, result = 0;
	size_t sum = 0, len, last_len, s;
	struct base64_state state, initial_state;

	if (srclen > OMP_THRESHOLD)
	{
		// Request a number of threads but not necessarily get them:
		#pragma omp parallel
		{
			// Get the number of threads used from one thread only,
			// as num_threads is a shared var:
			#pragma omp single
			{
				num_threads = omp_get_num_threads();

				// Split the input string into num_threads
				// parts, each part a multiple of 4 bytes.
				// The remaining bytes will be done later:
				len = srclen / (num_threads * 4);
				len *= 4;
				last_len = srclen - num_threads * len;

				// Init the stream reader:
				base64_stream_decode_init(&state, flags);

				initial_state = state;
			}

			// Single has an implicit barrier to wait here for the
			// above to complete:
			#pragma omp for firstprivate(state) private(s) reduction(+:sum, result) schedule(static,1)
			for (int i = 0; i < num_threads; i++)
			{
				int this_result;

				// Feed each part of the string to the stream reader:
				this_result = base64_stream_decode(&state, src + i * len, len, out + i * len * 3 / 4, &s);
				sum += s;
				result += this_result;
			}
		}
		state = initial_state;
		result += base64_stream_decode(&state, src + num_threads * len, last_len, out + num_threads * len * 3 / 4, &s);
		sum += s;
		*outlen = sum;

		if (result > 0)
			result = 1;

		if (result < 0)
			result = -1;

	} else {
		base64_stream_decode_init(&state, flags);
		result = base64_stream_decode(&state, src, srclen, out, outlen);
	}

	return result;
}
