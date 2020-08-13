#include <errno.h>
#include <poll.h>
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

    static const char flagname[] = "--jobserver-auth=";
    const char *p = strstr(makeflags, flagname);
    if (!p) {
        fprintf(stderr, "job: error: --jobserver-auth not in MAKEFLAGS (%s)\n", makeflags);
        return 0;
    }
    p += sizeof(flagname) - 1;

    const char *c = strchr(p, ',');
    if (!c) {
        fprintf(stderr, "job: error: Incorrect --jobserver-auth format\n");
        return 0;
    }
    c++;

    fds[0] = atoi(p);
    fds[1] = atoi(c);

    return 1;
}

static void jobclient_do_poll(int fd, int flags) {
    struct pollfd pfd;
    pfd.fd = fd;
    pfd.events = flags;
    pfd.revents = 0;
    int res = poll(&pfd, 1, -1);
    if (res != 1) {
        perror("poll");
    }
}

static int try_acquire_token(int *fds, char* token) {
    return read(fds[0], token, 1) == 1;
}

static int acquire_token(int *fds, char* token) {
    for(;;) {
        jobclient_do_poll(fds[0], POLLIN);
        if (try_acquire_token(fds, token)) {
            return 1;
        } else if (errno != EAGAIN) {
            return 0;
        }
    }
}

static int release_token(int *fds, char token) {
    for(;;) {
        jobclient_do_poll(fds[1], POLLOUT);
        if (write(fds[1], &token, 1) == 1) {
            return 1;
        } else if (errno != EAGAIN) {
            return 0;
        }
    }
}
