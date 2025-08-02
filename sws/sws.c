#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

void usage(void) {
    printf("Usage: sws [-p port]\n");
}

int main(int argc, char *argv[]) {
    int port = 9876;

    int opt;
    while ((opt = getopt(argc, argv, "p:")) != -1) {
        switch (opt) {
            case 'p':
                port = atoi(optarg);
                break;
            case '?':
                usage();
                return 1;
        }
    }

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("socket");
        return 1;
    }

    struct sockaddr_in src = {
        .sin_port = htons(port),
        .sin_family = AF_INET,
        .sin_addr.s_addr = htonl(INADDR_ANY),
    };

    if (bind(sock, (struct sockaddr *)&src, sizeof(src)) == -1) {
        perror("bind");
        return 1;
    }

    if (listen(sock, 1) == -1) {
        perror("listen");
        return 1;
    }

    struct sockaddr_in dst;
    socklen_t dstlen = sizeof(dst);
    if (accept(sock, (struct sockaddr *)&dst, &dstlen) == -1) {
        perror("accept");
        return 1;
    }

    printf("Connected from %s\n", inet_ntoa(dst.sin_addr));

    close(sock);

    return 0;
}
