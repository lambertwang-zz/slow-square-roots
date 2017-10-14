#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#ifdef linux
#include <sys/resource.h>
#include <unistd.h>
#endif

 
#define EXP_HIGH_BIT 0x40000000
#define MANT_HIGH_BIT 0x00400000
#define VALUE_COUNT 100000000
 
float values[VALUE_COUNT];
float hack_result[VALUE_COUNT];
float real_result[VALUE_COUNT];


inline uint64_t rdtsc() {
    uint32_t lo, hi;
    __asm__ __volatile__ (
      "xorl %%eax, %%eax\n"
      "cpuid\n"
      "rdtsc\n"
      : "=a" (lo), "=d" (hi)
      :
      : "%ebx", "%ecx");
    return (uint64_t)hi << 32 | lo;
}

void set_thread_high_pri() {
    int which = PRIO_PROCESS;
    id_t pid;
    int priority = -20;
    int ret;

    pid = getpid();
    ret = setpriority(which, pid, priority);
}

int main()
{
    #ifdef linux
    set_thread_high_pri();
    #endif

    srand(rdtsc());
    for (size_t i = 0; i < VALUE_COUNT; i++)
    {
        do
        {
            uint32_t x;
            x = rand() & 0xff;
            x |= (rand() & 0xff) << 8;
            x |= (rand() & 0xff) << 16;
            x |= (rand() & 0xff) << 24;
            uint32_t bits = (x & ~0x80000000) | EXP_HIGH_BIT;
            values[i] = *(float*)&bits;
        }
        while (isnan(values[i]));
    }
 
    unsigned long long t = rdtsc();
    for (size_t i = 0; i < VALUE_COUNT; i++)
    {
        real_result[i] = sqrt(values[i]);
    }
    unsigned long long real_sqrt = rdtsc() - t;
    printf("real: %llu ticks\n", real_sqrt);
 



    t = rdtsc();
    for (size_t i = 0; i < VALUE_COUNT; i++)
    {
        uint32_t bits = ((*(uint32_t*)&values[i] >> 1) ^ (EXP_HIGH_BIT | (EXP_HIGH_BIT >> 1))) & ~MANT_HIGH_BIT;
        hack_result[i] = *(float*)&bits;
    }
    unsigned long long shittysqrt = rdtsc() - t;
    printf("hack: %llu ticks\n", shittysqrt);
 
    printf("hack is %f%% faster\n", (float)real_sqrt * 100 / shittysqrt - 100);
 
    float error = 0;
    float min = 1;
    size_t buckets[100] = {0};
    size_t awful = 0;

    for (size_t i = 0; i < 1000000; i++)
    {
        float hack = hack_result[i] * hack_result[i];
        float real = real_result[i] * real_result[i];

        if (isnan(hack) || isnan(real)) {
            continue;
        }

        float err = (hack - real) / real;
        err = fabs(err);
        if (err < min) min = err;
        error += err;

        size_t index = (size_t)(err * 100);
        if (index > 99) {
            awful++;
            continue;
        }
        buckets[index]++;
        
    }
    error /= 1000000;
    printf("avg abs rel err: %f\n", error);
    printf("min rel err: %f\n", min * 100);

    for (size_t i = 0; i < 100; i++) {
        printf("E%%[%i-%i]: %d\n", i, i+1, buckets[i]);
    }

    printf("E%%>100%%: %i\n", awful);
}