#include "monitor.h"

int save_report(const SysSnapshot *snap, const char *path)
{
    FILE *fp = fopen(path, "a");
    if (!fp) {
        fprintf(stderr, "report: cannot open '%s': %s\n", path, strerror(errno));
        return -1;
    }

    time_t now = time(NULL);
    char tbuf[64];
    strftime(tbuf, sizeof(tbuf), "%Y-%m-%d %H:%M:%S", localtime(&now));

    fprintf(fp, "========================================\n");
    fprintf(fp, "Report: %s\n", tbuf);
    fprintf(fp, "========================================\n");

    fprintf(fp, "\n[CPU]\n");
    fprintf(fp, "  Total usage: %.2f%%\n", snap->cpu_total_percent);

    fprintf(fp, "\n[Memory]\n");
    long used = snap->mem_total_kb - snap->mem_available_kb;
    fprintf(fp, "  Total:     %ld MB\n", snap->mem_total_kb    / 1024);
    fprintf(fp, "  Used:      %ld MB\n", used                  / 1024);
    fprintf(fp, "  Free:      %ld MB\n", snap->mem_free_kb     / 1024);
    fprintf(fp, "  Available: %ld MB\n", snap->mem_available_kb / 1024);
    fprintf(fp, "  Cached:    %ld MB\n", snap->mem_cached_kb   / 1024);
    fprintf(fp, "  SwapTotal: %ld MB\n", snap->swap_total_kb   / 1024);
    fprintf(fp, "  SwapFree:  %ld MB\n", snap->swap_free_kb    / 1024);

    /* top CPU */
    ProcInfo sorted[MAX_PROCS];
    int n = snap->count;
    if (n > MAX_PROCS) n = MAX_PROCS;
    memcpy(sorted, snap->procs, n * sizeof(ProcInfo));
    qsort(sorted, n, sizeof(ProcInfo), cmp_cpu);

    fprintf(fp, "\n[Top %d by CPU]\n", TOP_N);
    fprintf(fp, "  %-6s %-20s %8s %10s\n", "PID", "NAME", "CPU%", "RSS(MB)");
    for (int i = 0; i < n && i < TOP_N; i++) {
        fprintf(fp, "  %-6d %-20.20s %7.2f%% %9.1f\n",
            sorted[i].pid, sorted[i].name,
            sorted[i].cpu_percent, sorted[i].rss_kb / 1024.0);
    }

    /* top memory */
    memcpy(sorted, snap->procs, n * sizeof(ProcInfo));
    qsort(sorted, n, sizeof(ProcInfo), cmp_mem);

    fprintf(fp, "\n[Top %d by Memory]\n", TOP_N);
    fprintf(fp, "  %-6s %-20s %10s %8s\n", "PID", "NAME", "RSS(MB)", "CPU%");
    for (int i = 0; i < n && i < TOP_N; i++) {
        fprintf(fp, "  %-6d %-20.20s %9.1f %7.2f%%\n",
            sorted[i].pid, sorted[i].name,
            sorted[i].rss_kb / 1024.0, sorted[i].cpu_percent);
    }

    fprintf(fp, "\n  Total processes: %d\n\n", snap->count);
    fclose(fp);
    return 0;
}
