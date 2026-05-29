#include "monitor.h"

static void print_separator(char c, int width)
{
    for (int i = 0; i < width; i++) putchar(c);
    putchar('\n');
}

static void print_bar(const char *label, double used_kb, double total_kb, int bar_width)
{
    double pct = (total_kb > 0) ? (used_kb / total_kb) : 0.0;
    int filled = (int)(pct * bar_width);
    if (filled > bar_width) filled = bar_width;

    printf("%-8s [", label);
    for (int i = 0; i < bar_width; i++)
        putchar(i < filled ? '#' : '.');
    printf("] %5.1f%%  (%.0f / %.0f MB)\n",
        pct * 100.0,
        used_kb  / 1024.0,
        total_kb / 1024.0);
}

void display_snapshot(const SysSnapshot *snap)
{
    /* clear screen */
    printf("\033[2J\033[H");

    time_t now = time(NULL);
    char tbuf[64];
    strftime(tbuf, sizeof(tbuf), "%Y-%m-%d %H:%M:%S", localtime(&now));

    print_separator('=', 72);
    printf("  /proc System Monitor   %s\n", tbuf);
    print_separator('=', 72);

    /* ── CPU ── */
    printf("\n[CPU]\n");
    print_bar("Total", snap->cpu_total_percent, 100.0, 40);

    /* ── Memory ── */
    printf("\n[Memory]\n");
    long used_kb = snap->mem_total_kb - snap->mem_available_kb;
    print_bar("RAM",  (double)used_kb,                (double)snap->mem_total_kb,  40);
    print_bar("Swap", (double)(snap->swap_total_kb - snap->swap_free_kb),
                      (double)snap->swap_total_kb, 40);
    printf("  Cached: %ld MB\n", snap->mem_cached_kb / 1024);

    /* ── Top CPU processes ── */
    ProcInfo sorted[MAX_PROCS];
    int n = snap->count;
    if (n > MAX_PROCS) n = MAX_PROCS;
    memcpy(sorted, snap->procs, n * sizeof(ProcInfo));
    qsort(sorted, n, sizeof(ProcInfo), cmp_cpu);

    printf("\n[Top %d by CPU]\n", TOP_N);
    printf("  %-6s %-20s %8s %10s\n", "PID", "NAME", "CPU%", "RSS(MB)");
    print_separator('-', 50);
    for (int i = 0; i < n && i < TOP_N; i++) {
        printf("  %-6d %-20.20s %7.2f%% %9.1f\n",
            sorted[i].pid,
            sorted[i].name,
            sorted[i].cpu_percent,
            sorted[i].rss_kb / 1024.0);
    }

    /* ── Top Memory processes ── */
    memcpy(sorted, snap->procs, n * sizeof(ProcInfo));
    qsort(sorted, n, sizeof(ProcInfo), cmp_mem);

    printf("\n[Top %d by Memory]\n", TOP_N);
    printf("  %-6s %-20s %10s %8s\n", "PID", "NAME", "RSS(MB)", "CPU%");
    print_separator('-', 50);
    for (int i = 0; i < n && i < TOP_N; i++) {
        printf("  %-6d %-20.20s %9.1f %7.2f%%\n",
            sorted[i].pid,
            sorted[i].name,
            sorted[i].rss_kb / 1024.0,
            sorted[i].cpu_percent);
    }

    printf("\n  Total processes: %d\n", snap->count);
    print_separator('=', 72);
    printf("  Press Ctrl+C to stop.\n");
    fflush(stdout);
}
