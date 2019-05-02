#include <stdio.h>
#include "sysdep.h"

#define BUFLEN (64 * 1024)

static unsigned char buf[BUFLEN];

int main(int argc, char **argv)
{
    int fd;
    int i, n = 1, len;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s filename\n", argv[0]);        exit(1);
    }

    if ((fd = open(argv[1], O_RDONLY | O_BINARY)) == -1) {
        fprintf(stderr, "Open: %s, error=%d\n", argv[1], errno);
        exit(1);
    }
    printf("/* do not edit */\nstatic const unsigned char DefaultConfig[] = {\n");
    while ((len = read(fd, buf, BUFLEN)) > 0) {
        for (i = 0; i < len; i++) {
            printf("0x%02.02X", buf[i]);
            if (n++ % 10) {
                printf(", ");
            } else {
                printf(",\n");
            }
        }
    }
    close(fd);
    printf("\n};\n");
    return 0;
}
