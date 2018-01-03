#include <spawn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

extern char **environ;

// https://www.gnu.org/software/make/manual/html_node/POSIX-Jobserver.html#POSIX-Jobserver

static int parse_makeflags(int *fds) {
    const char *makeflags = getenv("MAKEFLAGS");
    if (!makeflags) {
        fprintf(stderr, "job: error: MAKEFLAGS not set\n");
        return 0;
    }

    static const char flagname[] = "--jobserver-fds=";
    const char *p = strstr(makeflags, flagname);
    if (!p) {
        fprintf(stderr, "job: error: --jobserver-fds not in MAKEFLAGS (%s)\n", makeflags);
        return 0;
    }
    p += sizeof(flagname) - 1;

    const char *c = strchr(p, ',');
    if (!c) {
        fprintf(stderr, "job: error: Incorrect --jobserver-fds format\n");
        return 0;
    }
    c++;

    fds[0] = atoi(p);
    fds[1] = atoi(c);

    return 1;
}

int main(int argc, const char *argv[]) {
    int jobserver_fds[2];
    if (!parse_makeflags(jobserver_fds)) {
        return 1;
    }
    // NB! Normally a command run with a jobserver gets one "free" job for itself, but since this is run from e.g. a shell script it won't 
    char token = 0;
    {
        ssize_t res = read(jobserver_fds[0], &token, 1);
        if (res != 1) {
            perror("read from jobserver");
            return 1;
        }
    }
    pid_t pid;
    int res = posix_spawnp(&pid, argv[1],
            NULL, /* file actions */
            NULL, /* attrs */
            (char *const*)argv + 1,
            environ);
    if (res < 0) {
        perror("posix_spawnp");
        res = 1;
        goto leave;
    }
    // Since WNOHANG is not specified, the only valid returns should be pid
    // (success) or -1 (error)
    if (waitpid(pid, &res, 0) != pid) {
        perror("waitpid");
        res = 1;
    }
leave:
    write(jobserver_fds[1], &token, 1);
    return res;
}
