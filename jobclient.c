#include <spawn.h>
#include <stdio.h>
#include <sys/wait.h>

#include "jobclient.h"

extern char **environ;

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: jobclient CMD [ARGS...]\n");
        return 1;
    }

    int jobserver_fds[2];
    if (!parse_makeflags(jobserver_fds)) {
        return 1;
    }
    // NB! Normally a command run with a jobserver gets one "free" job for
    // itself, but since this is run from e.g. a shell script this won't be
    // considered.
    // Our caller needs to know that it always has one more for free...
    // Maybe this could be done with a wrapper server :)
    // One hacky workaround is to use jobforce to fake-release a token at the
    // start and fake-acquire one at the end.
    char token = 0;
    if (!acquire_token(jobserver_fds, &token)) {
        perror("acquire_token");
        return 1;
    }
    //printf("jobclient: Got token %c (%02x)\n", isprint(token) ? token : '.', token);
    pid_t pid;
    int res = posix_spawnp(&pid, argv[1],
            NULL, /* file actions */
            NULL, /* attrs */
            argv + 1,
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
    if (!release_token(jobserver_fds, token)) {
        perror("release_token");
        res = 1;
    }
    return res;
}
