#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <x86intrin.h>

int main(void) {
    uint64_t start, end;
    int i;

    _mm_lfence();
    start = __rdtsc();
    _mm_lfence();

    for (i = 0; i < 100; i++) {
        getppid();
    }

    _mm_lfence();
    end = __rdtsc();
    _mm_lfence();

    printf("Total TSC ticks for 100 getppid() calls: %llu\n", (unsigned long long)(end - start));

    printf("Average TSC ticks per call: %.2f\n", (double)(end - start) / 100);

    return 0;
}