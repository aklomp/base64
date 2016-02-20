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
	struct base64_state state;   

	// Init the stream reader:
	base64_stream_encode_init(&state, flags);

	// Feed the whole string to the stream reader:
	base64_stream_encode(&state, src, srclen, out, &s);

	// Finalize the stream by writing trailer if any:
	base64_stream_encode_final(&state, out + s, &t);

	// Final output length is stream length plus tail:
	*outlen = s + t;
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
	struct base64_state state;
	
	// Init the stream reader:
	base64_stream_decode_init(&state, flags);

	// Feed the whole string to the stream reader:
	return base64_stream_decode(&state, src, srclen, out, outlen);
}