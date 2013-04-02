#include "skein/skein.h"
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <time.h>
#include <signal.h>
#include <pthread.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <unistd.h>
#endif

#define INC_BEFORE_NEW 1024
#define HASH_BEFORE_REPORT 100000
#define SECONDS_BEFORE_UPDATE 2

struct thr_info {
	double hashrate;
	pthread_t pth;
};

pthread_mutex_t best_mutex = PTHREAD_MUTEX_INITIALIZER; 
volatile int best = 1024;

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

// thx rfw
void inc_input(char *input, int length) {
    int i = length/2;
    do {
        switch (++input[i]) {
            case ('z' + 1):
                input[i] = 'A';
                break;
            case ('Z' + 1):
                input[i] = '0';
                break;
            case ('9' + 1):
                input[i] = 'a';
                break;
            default:
                break;
        }
    } while (input[i--] == 'a' && i > 0);
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

// cross-platform is diques (thx stackoverflow)
int get_num_cpus() {
	int cpus = -1;
#ifdef _WIN32
	#ifndef _SC_NPROCESSORS_ONLN
		SYSTEM_INFO info;
		GetSystemInfo(&info);
		#define sysconf(a) info.dwNumberOfProcessors
		#define _SC_NPROCESSORS_ONLN
	#endif
#endif
#ifdef _SC_NPROCESSORS_ONLN
	cpus = sysconf(_SC_NPROCESSORS_ONLN);
	if (cpus < 1) {
		fprintf(stderr, "Could not auto-detect CPU count! Only one thread will be used.");
		cpus = 1;
	}
	
	return cpus;
#else
	fprintf(stderr, "This binary was not compiled with CPU auto-detection. Only one thread will be used.");
	return 1;
#endif
}

void *hash_thread(void *infostruct) {
	struct thr_info *tinfo = infostruct;

	// skein things
	Skein1024_Ctxt_t ctx;
	uint8_t out[128];

	// randogen things
	char str[101];
	int strLen = 0;

	// timekeeping for status outputs
	struct timeval lastcomplete;
	gettimeofday( &lastcomplete, NULL );

	for (int i = 0; ; i++) {
		// Only refresh the input with a new random value every so often
		if (i % INC_BEFORE_NEW == 0) {
			strLen = mixit(str);
		} else {
			inc_input(str, strLen);
		}

		// this is slow :(
		Skein1024_Init(&ctx, 1024);
		Skein1024_Update(&ctx, (uint8_t*) str, strLen);
		Skein1024_Final(&ctx, out);

		int diff = bitdiff(out);
		if (diff < best) {
			pthread_mutex_lock(&best_mutex);

			// threading is diques.
			if (diff < best) {
				best = diff;
				printf("%u %s\n", best, str);	
			}
			
			pthread_mutex_unlock(&best_mutex);
		}

		if (i >= HASH_BEFORE_REPORT) {
			struct timeval timenow;
			gettimeofday( &timenow, NULL );

			double seconds = (timenow.tv_sec - lastcomplete.tv_sec) + 1.0e-6 * (timenow.tv_usec - lastcomplete.tv_usec);
			tinfo->hashrate = HASH_BEFORE_REPORT / seconds / 1000;

			gettimeofday( &lastcomplete, NULL );
			i = 0;
		}
	}
}

int main(int argc, char *argv[]) {
	// setup
	srand(time(NULL));
	signal(SIGINT, quit);
	setbuf(stdout, NULL);
	setbuf(stderr, NULL);
	selftest();

	// get number of threads
	int num_threads = get_num_cpus();
	if (argc > 1)
		num_threads = atoi(argv[1]);
	if (num_threads < 1)
		num_threads = 1;

	struct thr_info *td = malloc(sizeof(struct thr_info) * num_threads);

	fprintf(stderr, "hashing with %u threads.\n", num_threads);

	// start threads
	for (int i = 0; i < num_threads; i++) {
		td[i].hashrate = 0;

		if (pthread_create(&td[i].pth, NULL, hash_thread, &td[i])) {
			fprintf(stderr, "failed to create thread #%u", i);
			return -1;
		}

		sleep(1);
	}

	// measure hashrate
	for (double hashrate = 0; ; hashrate = 0) {
		sleep(SECONDS_BEFORE_UPDATE);

		for (int i = 0; i < num_threads; i++)
			hashrate += td[i].hashrate;

		fprintf(stderr, "%.2f khash/s  \r", hashrate);
	}

	return 0;
}
