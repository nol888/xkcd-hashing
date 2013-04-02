#include "skein.h"
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <time.h>
#include <signal.h>

const uint64_t oracle[16] = {
	0x8082A05F5FA94D5B, 0xC818F444DF7998FC,
	0x7D75B724A42BF1F9, 0x4F4C0DAEFBBD2BE0,
	0x04FEC50CC81793DF, 0x97F26C46739042C6,
	0xF6D2DD9959C2B806, 0x877B97CC75440D54,
	0x8F9BF123E07B75F4, 0x88B7862872D73540,
	0xF99CA716E96D8269, 0x247D34D49CC74CC9,
	0x73A590233EAA67B5, 0x4066675E8AA473A3,
	0xE7C5E19701C79CC7, 0xB65818CA53FB02F9
};
const char ALLOWED_CHARACTERS[] = "abcdefghijklmonpqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

int bitdiff(uint8_t *in) {
	uint64_t *fuck_c = (uint64_t*) in;
	int popcount = 0;

	// gcc funroll pls
	for (int i = 0; i < 16; i++)
		popcount += __builtin_popcountll(fuck_c[i] ^ oracle[i]);

	return popcount;
}

// will use, at max, 101 bytes
inline int mixit(char *tgt) {
	int len = rand() / (RAND_MAX / 50) + 50;
	int i;

	for (i = 0; i < len; i++) {
		tgt[i] = ALLOWED_CHARACTERS[(sizeof(ALLOWED_CHARACTERS) / sizeof(char) - 1) * rand() / RAND_MAX];
	}
	tgt[i] = 0;

	return len;
}

// for profiling
void quit(int q) {
	printf("Quitting.        \n");
	exit(0);
}

// because things keep breaking
void selftest() {
	Skein1024_Ctxt_t ctx;
	uint8_t out[128];

	Skein1024_Init(&ctx, 1024);
	Skein1024_Update(&ctx, (uint8_t*) "ZEgAqmkKRIerhysQkExb9U046Di11D5Q7bhnorY5Z3m6rFNC9bE9V", 53);
	Skein1024_Final(&ctx, out);

	int diff = bitdiff(out);
	if (diff != 438) {
		printf("self test failed! got=%d, expected=438\n", diff);
		exit(1);
	}
}

int main() {
	// setup
	srand(time(NULL));
	signal(SIGINT, quit);
	setbuf(stdout, NULL);
	selftest();

	// skein things
	Skein1024_Ctxt_t ctx;
	uint8_t out[128];
	
	// randogen things
	char str[101];
	int strLen = 0;

	// bookkeeping
	int best = 1024;
	
#ifdef SHOW_STATUS
	// timekeeping for status outputs
	struct timeval lastcomplete;
	gettimeofday( &lastcomplete, NULL );

	for (int i = 0; ; i++)
#else
	for (;;)
#endif
	{
		strLen = mixit(str);

		// this is slow :(
		Skein1024_Init(&ctx, 1024);
		Skein1024_Update(&ctx, (uint8_t*) str, strLen);
		Skein1024_Final(&ctx, out);
		
		int diff = bitdiff(out);
		if (diff < best) {
			best = diff;
			printf("%u %s\n", best, str);
		}

#ifdef SHOW_STATUS
		if (i >= 500000) {
			struct timeval timenow;
			gettimeofday( &timenow, NULL );

			double seconds = (timenow.tv_sec - lastcomplete.tv_sec) + 1.0e-6 * (timenow.tv_usec - lastcomplete.tv_usec);

			printf("%.1f hash/s\r", 500000 / seconds);

			gettimeofday( &lastcomplete, NULL );
			i = 0;
		}
#endif
	}

	return 0;
}