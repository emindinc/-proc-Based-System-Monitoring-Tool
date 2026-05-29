#include "monitor.h"

static void handle_sigint(int sig)
{
    (void)sig;
    g_running = 0;
}

static void usage(const char *prog)
{
    printf("Usage: %s [OPTIONS]\n"
           "  -i <sec>    Update interval (default: 2)\n"
           "  -c          Continuous mode (keep refreshing)\n"
           "  -o <file>   Save report to file after each update\n"
           "  -h          Show this help\n",
           prog);
}

int main(int argc, char *argv[])
{
    /* defaults */
    g_config.interval_sec = 2;
    g_config.continuous   = 0;
    g_config.save_report  = 0;
    g_config.report_file[0] = '\0';

    int opt;
    while ((opt = getopt(argc, argv, "i:co:h")) != -1) {
        switch (opt) {
        case 'i':
            g_config.interval_sec = atoi(optarg);
            if (g_config.interval_sec < 1) g_config.interval_sec = 1;
            break;
        case 'c':
            g_config.continuous = 1;
            break;
        case 'o':
            strncpy(g_config.report_file, optarg, MAX_PATH_LEN - 1);
            g_config.save_report = 1;
            break;
        case 'h':
            usage(argv[0]);
            return 0;
        default:
            usage(argv[0]);
            return 1;
        }
    }

    signal(SIGINT,  handle_sigint);
    signal(SIGTERM, handle_sigint);

    pthread_t tid;
    if (pthread_create(&tid, NULL, collector_thread, NULL) != 0)
        die("pthread_create");

    while (g_running) {
        pthread_mutex_lock(&g_lock);
        int dirty = g_snapshot.dirty;
        SysSnapshot local = g_snapshot;
        if (dirty) g_snapshot.dirty = 0;
        pthread_mutex_unlock(&g_lock);

        if (dirty) {
            display_snapshot(&local);
            if (g_config.save_report)
                save_report(&local, g_config.report_file);
            if (!g_config.continuous) break;  /* one-shot: done after first snapshot */
        }

        usleep(100000); /* 100 ms poll */
    }

    g_running = 0;
    pthread_join(tid, NULL);

    printf("\nMonitor stopped.\n");
    return 0;
}
