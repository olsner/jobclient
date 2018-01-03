#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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

static int acquire_token(int *fds, char* token) {
    ssize_t res = read(fds[0], token, 1);
    if (res != 1) {
        perror("read from jobserver");
        return 0;
    }
    return 1;
}

static int release_token(int *fds, char token) {
    return write(fds[1], &token, 1) == 1;
}
