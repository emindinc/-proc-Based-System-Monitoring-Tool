#include "monitor.h"

int cmp_cpu(const void *a, const void *b)
{
    const ProcInfo *pa = (const ProcInfo *)a;
    const ProcInfo *pb = (const ProcInfo *)b;
    if (pb->cpu_percent > pa->cpu_percent) return  1;
    if (pb->cpu_percent < pa->cpu_percent) return -1;
    return 0;
}

int cmp_mem(const void *a, const void *b)
{
    const ProcInfo *pa = (const ProcInfo *)a;
    const ProcInfo *pb = (const ProcInfo *)b;
    if (pb->rss_kb > pa->rss_kb) return  1;
    if (pb->rss_kb < pa->rss_kb) return -1;
    return 0;
}

void die(const char *msg)
{
    perror(msg);
    exit(EXIT_FAILURE);
}
