#include "monitor.h"

/* ── /proc/stat → CPU totals ── */
int read_cpu_stat(CpuStat *cs)
{
    FILE *fp = fopen("/proc/stat", "r");
    if (!fp) return -1;

    int ok = fscanf(fp,
        "cpu %llu %llu %llu %llu %llu %llu %llu %llu",
        &cs->user, &cs->nice, &cs->system, &cs->idle,
        &cs->iowait, &cs->irq, &cs->softirq, &cs->steal);
    fclose(fp);
    return (ok == 8) ? 0 : -1;
}

/* ── /proc/meminfo ── */
int read_mem_info(SysSnapshot *snap)
{
    FILE *fp = fopen("/proc/meminfo", "r");
    if (!fp) return -1;

    char key[64];
    long val;
    char unit[16];
    int fields = 0;

    while (fscanf(fp, "%63s %ld %15s", key, &val, unit) == 3) {
        if      (strcmp(key, "MemTotal:")     == 0) { snap->mem_total_kb     = val; fields++; }
        else if (strcmp(key, "MemFree:")      == 0) { snap->mem_free_kb      = val; fields++; }
        else if (strcmp(key, "MemAvailable:") == 0) { snap->mem_available_kb = val; fields++; }
        else if (strcmp(key, "Cached:")       == 0) { snap->mem_cached_kb    = val; fields++; }
        else if (strcmp(key, "SwapTotal:")    == 0) { snap->swap_total_kb    = val; fields++; }
        else if (strcmp(key, "SwapFree:")     == 0) { snap->swap_free_kb     = val; fields++; }
        if (fields == 6) break;
    }
    fclose(fp);
    return (fields == 6) ? 0 : -1;
}

/* ── /proc/<pid>/stat → per-process times and RSS ── */
static int read_proc_stat(int pid, ProcInfo *p)
{
    char path[MAX_PATH_LEN];
    snprintf(path, sizeof(path), "/proc/%d/stat", pid);

    FILE *fp = fopen(path, "r");
    if (!fp) return -1;

    char line[4096];
    int got = (fgets(line, sizeof(line), fp) != NULL);
    fclose(fp);
    if (!got) return -1;

    /* comm field is between first '(' and last ')' — may contain spaces */
    char *comm_start = strchr(line, '(');
    char *comm_end   = strrchr(line, ')');
    if (!comm_start || !comm_end || comm_end <= comm_start) return -1;

    size_t comm_len = (size_t)(comm_end - comm_start - 1);
    if (comm_len >= MAX_NAME_LEN) comm_len = MAX_NAME_LEN - 1;
    strncpy(p->name, comm_start + 1, comm_len);
    p->name[comm_len] = '\0';
    p->pid = pid;

    /* parse remaining fields after ')' */
    char state;
    int  ppid, pgrp, session, tty, tpgid;
    unsigned int flags;
    unsigned long minflt, cminflt, majflt, cmajflt;
    unsigned long long utime, stime;
    long cutime, cstime, priority, nice, num_threads, itrealvalue;
    unsigned long long starttime;
    unsigned long vsize;
    long rss;

    int n = sscanf(comm_end + 1,
        " %c %d %d %d %d %d %u "
        "%lu %lu %lu %lu "
        "%llu %llu %ld %ld %ld %ld %ld %ld "
        "%llu %lu %ld",
        &state, &ppid, &pgrp, &session, &tty, &tpgid, &flags,
        &minflt, &cminflt, &majflt, &cmajflt,
        &utime, &stime, &cutime, &cstime, &priority, &nice,
        &num_threads, &itrealvalue,
        &starttime, &vsize, &rss);

    if (n < 22) return -1;

    p->utime  = utime;
    p->stime  = stime;
    p->rss_kb = rss * (sysconf(_SC_PAGESIZE) / 1024);

    return 0;
}

/* ── Scan /proc for all numeric directories ── */
int read_all_procs(SysSnapshot *snap, CpuStat *prev_cpu, CpuStat *cur_cpu,
                   const SysSnapshot *prev_snap)
{
    unsigned long long prev_total =
        prev_cpu->user + prev_cpu->nice + prev_cpu->system + prev_cpu->idle +
        prev_cpu->iowait + prev_cpu->irq + prev_cpu->softirq + prev_cpu->steal;
    unsigned long long cur_total =
        cur_cpu->user + cur_cpu->nice + cur_cpu->system + cur_cpu->idle +
        cur_cpu->iowait + cur_cpu->irq + cur_cpu->softirq + cur_cpu->steal;
    unsigned long long delta_total = cur_total - prev_total;
    if (delta_total == 0) delta_total = 1;

    unsigned long long prev_idle = prev_cpu->idle + prev_cpu->iowait;
    unsigned long long cur_idle  = cur_cpu->idle  + cur_cpu->iowait;
    snap->cpu_total_percent =
        100.0 * (1.0 - (double)(cur_idle - prev_idle) / (double)delta_total);

    DIR *dir = opendir("/proc");
    if (!dir) return -1;

    snap->count = 0;
    struct dirent *de;

    while ((de = readdir(dir)) != NULL && snap->count < MAX_PROCS) {
        if (!isdigit((unsigned char)de->d_name[0])) continue;

        int pid = atoi(de->d_name);
        ProcInfo p;
        memset(&p, 0, sizeof(p));

        if (read_proc_stat(pid, &p) != 0) continue;

        /* Find this PID in the previous snapshot to compute delta ticks */
        unsigned long long prev_proc_ticks = 0;
        if (prev_snap) {
            for (int i = 0; i < prev_snap->count; i++) {
                if (prev_snap->procs[i].pid == pid) {
                    prev_proc_ticks = prev_snap->procs[i].utime + prev_snap->procs[i].stime;
                    break;
                }
            }
        }

        unsigned long long cur_proc_ticks = p.utime + p.stime;
        unsigned long long delta_proc = (cur_proc_ticks >= prev_proc_ticks)
                                        ? (cur_proc_ticks - prev_proc_ticks) : 0;

        p.cpu_percent = 100.0 * (double)delta_proc / (double)delta_total;

        snap->procs[snap->count++] = p;
    }
    closedir(dir);
    return 0;
}
