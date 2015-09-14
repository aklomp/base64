#include <windows.h>
#include <wincrypt.h>

static int
get_random_data(struct buffers *b, char **errmsg)
{
	HCRYPTPROV hProvider = 0;

	if (!CryptAcquireContext(&hProvider, 0, 0, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT | CRYPT_SILENT)) {
		return 0;
	}
	if (!CryptGenRandom(hProvider, b->regsz, b->reg)) {
		CryptReleaseContext(hProvider, 0);
		return 0;
	}
	if (!CryptReleaseContext(hProvider, 0)) {
		return 0;
	}

	return 1;
}