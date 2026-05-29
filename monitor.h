#ifndef MONITOR_H
#define MONITOR_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <signal.h>
#include <errno.h>
#include <sys/stat.h>
#include <ctype.h>

#define MAX_PROCS       512
#define MAX_NAME_LEN    256
#define MAX_PATH_LEN    512
#define TOP_N           10

typedef struct {
    unsigned long long user, nice, system, idle, iowait, irq, softirq, steal;
} CpuStat;

typedef struct {
    int    pid;
    char   name[MAX_NAME_LEN];
    long   rss_kb;
    double cpu_percent;
    unsigned long long utime;
    unsigned long long stime;
} ProcInfo;

typedef struct {
    ProcInfo procs[MAX_PROCS];
    int      count;
    double   cpu_total_percent;
    long     mem_total_kb;
    long     mem_free_kb;
    long     mem_available_kb;
    long     mem_cached_kb;
    long     swap_total_kb;
    long     swap_free_kb;
    int      dirty;
} SysSnapshot;

typedef struct {
    int    interval_sec;
    int    continuous;
    char   report_file[MAX_PATH_LEN];
    int    save_report;
} Config;

extern SysSnapshot     g_snapshot;
extern pthread_mutex_t g_lock;
extern volatile int    g_running;
extern Config          g_config;

/* proc_reader.c */
int  read_cpu_stat(CpuStat *cs);
int  read_mem_info(SysSnapshot *snap);
int  read_all_procs(SysSnapshot *snap, CpuStat *prev_cpu, CpuStat *cur_cpu,
                    const SysSnapshot *prev_snap);

/* display.c */
void display_snapshot(const SysSnapshot *snap);

/* report.c */
int  save_report(const SysSnapshot *snap, const char *path);

/* collector.c */
void *collector_thread(void *arg);

/* utils.c */
int  cmp_cpu(const void *a, const void *b);
int  cmp_mem(const void *a, const void *b);
void die(const char *msg);

#endif
