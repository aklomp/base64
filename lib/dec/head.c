int ret = 0;
const char *c = src;
char *o = out;
unsigned char q;

/* Use local temporaries to avoid cache thrashing: */
size_t outl = 0;
struct base64_state st;
st.eof = state->eof;
st.bytes = state->bytes;
st.carry = state->carry;

/* If we previously saw an EOF or an invalid character, bail out: */
if (st.eof) {
	*outlen = 0;
	return 0;
}
/* Turn four 6-bit numbers into three bytes: */
/* out[0] = 11111122 */
/* out[1] = 22223333 */
/* out[2] = 33444444 */

/* Duff's device again: */
switch (st.bytes)
{
	for (;;)
	{
	case 0:
