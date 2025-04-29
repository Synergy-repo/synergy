#include <stdint.h>
#include <stdio.h>

#include <x86intrin.h>
#include "fake_work.h"

__thread uint64_t concord_start_time;
__thread uint64_t concord_preempt_after_cycle = 1000;

void concord_rdtsc_func() {
    puts("handler");
}


int main(void) {
    concord_start_time = __rdtsc();
    fake_work_ns_rdtsc(1000);
    return 0;
}
