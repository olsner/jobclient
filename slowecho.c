#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, const char *argv[]) {
    const char *s = *++argv;
    sleep(atoi(s));
    int n = 0;
    while ((s = *++argv)) {
        if (n) putchar(' ');
        fputs(s, stdout);
        n = 1;
    }
    putchar('\n');
}
