#include "jobclient.h"

#include <ctype.h>

int main(int argc, const char *argv[]) {
    int jobserver_fds[2];
    if (!parse_makeflags(jobserver_fds)) {
        return 1;
    }

    if (argc != 2) {
        fprintf(stderr, "Usage: jobforce N\n"
"Consume (positive) or release (negative) jobs from the job server without\n"
"actually doing anything.\n"
"\n"
"Blocks until the number of jobs has been received.\n"
"Won't work well if N is larger than the number of jobs allowed.\n");
        return 1;
    }
    int d = atoi(argv[1]);
    while (d > 0) {
        char token;
        if (!acquire_token(jobserver_fds, &token)) {
            return 1;
        }
        printf("jobforce: Got token '%c' (%02x)\n", isprint(token) ? token : '.', token);
        d--;
    }
    while (d < 0) {
        // Make seems to give out token '+' all the time and not actually care
        // what is put back into the queue. You can actually see artificially
        // released tokens get re-issued to other clients too :)
        release_token(jobserver_fds, '-');
        d++;
    }
}
