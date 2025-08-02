#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

#define DEFAULT_PORT 9876

void usage(void) {
    printf("Usage: sws [-p port]\n");
}


struct option {
    int port;
};

void parse_option(int argc, char *argv[], struct option *opt) {
    int o;
    while ((o = getopt(argc, argv, "p:")) != -1) {
        switch (o) {
            case 'p':
                opt->port = atoi(optarg);
                break;
            case '?':
                usage();
                exit(1);
        }
    }
}

int init_server(struct option *opt) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("socket");
        return -1;
    }

    struct sockaddr_in src = {
        .sin_port = htons(opt->port),
        .sin_family = AF_INET,
        .sin_addr.s_addr = htonl(INADDR_ANY),
    };

    if (bind(sock, (struct sockaddr *)&src, sizeof(src)) == -1) {
        perror("bind");
        return -1;
    }

    if (listen(sock, 1) == -1) {
        perror("listen");
        return -1;
    }

    return sock;
}

int serve(int sock) {
    int conn = accept(sock, NULL, NULL);
    if (conn == -1) {
        perror("accept");
        return -1;
    }

    char buf[BUFSIZ];

    ssize_t nrecv;
    while ((nrecv = recv(conn, buf, BUFSIZ, 0)) > 0) {
        printf("sws: %s\n", buf);
    }

    if (nrecv == -1) {
        perror("recv");
        return -1;
    }

    close(conn);

    return 0;
}

int main(int argc, char *argv[]) {
    struct option opt = {
        .port = DEFAULT_PORT,
    };

    parse_option(argc, argv, &opt);

    int sock = init_server(&opt);
    if (sock == -1) {
        fprintf(stderr, "init_server() failed\n");
        return 1;
    }

    if (serve(sock) == -1) {
        fprintf(stderr, "serve() failed\n");
        return 1;
    }

    close(sock);

    return 0;
}
