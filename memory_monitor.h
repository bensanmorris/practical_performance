#prama once

#include <thread>

#if defined( _WIN32 )
#define NOMINMAX
#include "windows.h"
#include "psapi.h"
#elif defined(__linux__) || defined(__linux) || defined(linux) || defined(__gnu_linux__)
#include <unistd.h>
#include <sys/resource.h>
#endif

namespace ides {

struct mem_info {
    uint64_t process_pmem;
    mem_info() :
        process_pmem(0) {}
};

#if defined( _WIN32 )
inline void getMemoryInfo(mem_info& meminfo) {
    PROCESS_MEMORY_COUNTERS_EX pmc;
    GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc));
    meminfo.process_pmem = pmc.WorkingSetSize;
}
#elif defined(__linux__) || defined(__linux) || defined(linux) || defined(__gnu_linux__)
inline void getMemoryInfo(mem_info& meminfo) {
    struct rusage rusage;
    getrusage(RUSAGE_SELF, &rusage);
    meminfo.process_pmem = rusage.ru_maxrss * 1024L;
}
#endif

#define MEMORY_MONITOR_BEGIN \
    std::atomic_bool memory_monitor_stop(false); \
    ides::mem_info meminfo; \
    std::thread t([&] () { \
        while (!memory_monitor_stop) { \
            ides::mem_info tmp; \
            ides::getMemoryInfo(tmp); \
            if(tmp.process_pmem > meminfo.process_pmem) \
                meminfo.process_pmem = tmp.process_pmem; \
        } \
    });

#define MEMORY_MONITOR_END \
    memory_monitor_stop = true; \
    t.join();
}

