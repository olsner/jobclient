#include <fcntl.h>
#include <stdio.h>

#include "jobclient.h"

#define MAX_TOKENS 4096
static char tokens[MAX_TOKENS];

int main() {
    int jobserver_fds[2];
    if (!parse_makeflags(jobserver_fds)) {
        return 1;
    }
    fcntl(jobserver_fds[0], F_SETFL, O_NONBLOCK);

    size_t n = 0;
    while (n < MAX_TOKENS && acquire_token(jobserver_fds, &tokens[n])) {
        n++;
    }
    printf("%zu\n", n);
    while (n--) {
        release_token(jobserver_fds, tokens[n]);
    }

    return 0;
}
