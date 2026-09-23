#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>

// Inline assembly function to read the Time Stamp Counter (x86/x86_64 architectures)
static inline unsigned long long read_tsc() {
    unsigned int lo, hi;
    
    // The "rdtsc" assembly instruction reads the 64-bit counter.
    // It stores the lower 32 bits in the 'eax' register (lo)
    // and the upper 32 bits in the 'edx' register (hi).
    // The __volatile__ keyword ensures the compiler doesn't optimize this away or reorder it.
    __asm__ __volatile__ ("rdtsc" : "=a" (lo), "=d" (hi));
    
    // Shift the high bits to the left and combine them with the low bits
    return ((unsigned long long)hi << 32) | lo;
}

double get_cpu_freq_mhz() {
    FILE *fp = fopen("/proc/cpuinfo", "r");
    if (!fp) return 0.0;

    char line[256];
    double mhz = 0.0;

    while (fgets(line, sizeof(line), fp)) {
        if (strncmp(line, "cpu MHz", 7) == 0) {
            char *colon = strchr(line, ':');
            if (colon) {
                mhz = atof(colon + 1);
                break;
            }
        }
    }
    fclose(fp);
    return mhz;
}

int main() {
    unsigned long long start_cycles, end_cycles, total_cycles;
    pid_t pid;

    double cpu_mhz = get_cpu_freq_mhz();
    if (cpu_mhz <= 0.0) {
        fprintf(stderr, "Could not determine CPU frequency\n");
        return 1;
    }

    // 1. "Warm-up" the cache
    // We call getpid() once before starting the timer. 
    // This ensures that the memory pages and kernel instructions are loaded 
    // into the CPU cache, giving us an accurate measurement of the system call 
    // itself, rather than the time taken to fetch instructions from RAM.
    getpid();

    // 2. Take the start timestamp
    start_cycles = read_tsc();

    // 3. Execute the system call
    pid = getpid();

    // 4. Take the end timestamp immediately after
    end_cycles = read_tsc();

    // 5. Calculate the difference
    total_cycles = end_cycles - start_cycles;
    double time_ns = ((double)total_cycles * 1000.0) / cpu_mhz;

    printf("Process ID (PID): %d\n", pid);
    printf("Time taken for getpid(): %llu CPU cycles\n", total_cycles);
    printf("Time taken for getpid(): %.2f ns\n", time_ns);

    return 0;
}