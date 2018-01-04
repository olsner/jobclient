#include <fcntl.h>
#include <pthread.h>
#include <spawn.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/random.h>

extern char **environ;

#define MAX_JOBS 4096

static void append_makeflags(bool dryrun, int rfd, int wfd) {
    char buf[256];
    const char *makeflags = getenv("MAKEFLAGS");
    if (makeflags && makeflags[0] == 'n') {
        // Disable dryrun to avoid adding a duplicate
        dryrun = false;
    }
    snprintf(buf, sizeof(buf), "%s%s --jobserver-fds=%d,%d -j",
            dryrun ? "n" : "", makeflags ? makeflags : "", rfd, wfd);
    setenv("MAKEFLAGS", buf, 1);
}

static void strip_makeflags() {
    const char *makeflags = getenv("MAKEFLAGS");
    if (!makeflags) return;

    static const char flagname[] = "--jobserver-fds=";
    const char *p = strstr(makeflags, flagname);
    if (p) {
        const char *flagend = strchr(p, ' ');
        // If --jobserver-fd is followed by -j, strip that one too.
        // Otherwise the child would use infinite parallelism :)
        if (flagend && strncmp(flagend, " -j", 3) == 0) {
            flagend = strchr(flagend + 3, ' ');
        }

        char *buf = strdup(makeflags);
        if (flagend) {
            memmove(buf + (p - makeflags), flagend + 1, strlen(flagend));
        } else {
            buf[p - makeflags] = 0;
        }
        setenv("MAKEFLAGS", buf, 1);
        free(buf);
    }
}

struct jobserver_params {
    int jobs;
    int rfd;
    int wfd;
};

static void *jobserver_main(void *argp) {
    struct jobserver_params *params = argp;

    const int rfd = params->rfd, wfd = params->wfd;
    const int max = params->jobs;
    // The child program we launched counts for one job by itself
    int used = 1;

    for (;;) {
        if (used < max) {
            if (write(wfd, "+", 1) == 1) {
                used++;
                //fprintf(stderr, "jobserver: ++: %d\n", used);
            }
        }
        if (used == max) {
            char c;
            if (read(rfd, &c, 1) == 1) {
                used--;
                //fprintf(stderr, "jobserver: --: %d\n", used);
            }
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: jobserver [-n] [-d] [-jN] CMD [ARGS...]\n");
        return 1;
    }

    // getopt this stuff
    bool dryrun = false, hide_server = false;
    int jobs = 8; // TODO Autodetect number of CPUs
    int i = 1;
    while (argv[i][0] == '-') {
        switch (argv[i][1]) {
        case 'n':
            dryrun = true;
            break;
        case 'd':
            hide_server = true;
            break;
        case 'j':
            jobs = atoi(argv[i] + 2);
            break;
        }
        i++;
    }
    if (jobs < 1 || jobs > MAX_JOBS) {
        fprintf(stderr, "Job count out of range [got %d]\n", jobs);
        return 1;
    }
    argv += i - 1;
    argc -= i - 1;
    if (argc < 1) {
        fprintf(stderr, "jobserver: No command given\n");
        return 1;
    }

    if (hide_server) {
        strip_makeflags();
    } else {
        // Check for an existing job server and do something sensible?
    }

    posix_spawn_file_actions_t fileops;
    posix_spawn_file_actions_init(&fileops);

    pthread_t jobserver;
    if (!hide_server) {
        int p1[2], p2[2];
        if (pipe(p1) < 0 || pipe(p2) < 0) {
            perror("pipe");
            return 1;
        }

        const int client_rfd = p1[0], client_wfd = p2[1];
        const int server_wfd = p1[1], server_rfd = p2[0];

        // Child will see client_rfd and client_wfd duped to fd 3 and 4.
        // Could just give it the raw fds, but this is cleaner I guess?
        posix_spawn_file_actions_adddup2(&fileops, client_rfd, 3);
        posix_spawn_file_actions_adddup2(&fileops, client_wfd, 4);

        append_makeflags(dryrun, 3, 4);

        // Launch a thread to run the actual job server

        static struct jobserver_params params;
        params.jobs = jobs;
        params.rfd = server_rfd;
        params.wfd = server_wfd;
        pthread_create(&jobserver, NULL, jobserver_main, &params);
    }

    pid_t pid;
    int res = posix_spawnp(&pid, argv[1],
            &fileops,
            NULL, /* attrs */
            argv + 1,
            environ);
    if (res < 0) {
        perror("posix_spawnp");
        return 1;
    }

    // Since WNOHANG is not specified, the only valid returns should be pid
    // (success) or -1 (error)
    if (waitpid(pid, &res, 0) != pid) {
        perror("waitpid");
        return 1;
    } else if (WIFEXITED(res)) {
        return WEXITSTATUS(res);
    } else {
        return 1;
    }
}
