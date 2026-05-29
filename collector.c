#include "monitor.h"

SysSnapshot     g_snapshot;
pthread_mutex_t g_lock = PTHREAD_MUTEX_INITIALIZER;
volatile int    g_running = 1;
Config          g_config;

void *collector_thread(void *arg)
{
    (void)arg;

    CpuStat    prev_cpu, cur_cpu;
    SysSnapshot prev_snap;
    memset(&prev_cpu,  0, sizeof(prev_cpu));
    memset(&prev_snap, 0, sizeof(prev_snap));

    while (g_running) {
        read_cpu_stat(&prev_cpu);
        sleep((unsigned int)g_config.interval_sec);
        read_cpu_stat(&cur_cpu);

        SysSnapshot local;
        memset(&local, 0, sizeof(local));

        read_mem_info(&local);
        read_all_procs(&local, &prev_cpu, &cur_cpu, &prev_snap);
        local.dirty = 1;

        pthread_mutex_lock(&g_lock);
        g_snapshot = local;
        pthread_mutex_unlock(&g_lock);

        prev_cpu  = cur_cpu;
        prev_snap = local;

        if (!g_config.continuous) break;
    }
    return NULL;
}
