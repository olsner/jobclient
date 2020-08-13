#include <fcntl.h>
#include <stdio.h>

#include "jobclient.h"

#define MAX_TOKENS 4096
static char tokens[MAX_TOKENS];


int main() {
    // Ugly fix for compile warning because jobclient.h defines some functions
    // we don't use anymore. Should probably make it an actual "library" :)
    (void)acquire_token;

    int jobserver_fds[2];
    if (!parse_makeflags(jobserver_fds)) {
        return 1;
    }
    const int flags = fcntl(jobserver_fds[0], F_GETFL);
    fcntl(jobserver_fds[0], F_SETFL, flags | O_NONBLOCK);

    size_t n = 0;
    while (n < MAX_TOKENS && try_acquire_token(jobserver_fds, &tokens[n])) {
        n++;
    }
    fcntl(jobserver_fds[0], F_SETFL, flags);

    printf("%zu\n", n);
    while (n--) {
        release_token(jobserver_fds, tokens[n]);
    }

    return 0;
}
