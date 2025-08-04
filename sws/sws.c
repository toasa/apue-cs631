#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

#include "http.h"

#define DEFAULT_PORT 9876

static void usage(void) {
    printf("Usage: sws [-p port]\n");
}

struct option {
    int port;
};

static void parse_option(int argc, char *argv[], struct option *opt) {
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

static int init_server(struct option *opt) {
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

static int create_sock_stream(int sock, FILE **in, FILE **out) {
    int sock2 = dup(sock);
    if (sock2 == -1) {
        perror("dup");
        return -1;
    }

    *in = fdopen(sock, "r");
    if (*in == NULL) {
        perror("fdopen");
        return -1;
    }

    *out = fdopen(sock2, "w");
    if (*out == NULL) {
        perror("fdopen");
        return -1;
    }

    return 0;
}

static int serve(int sock) {
    int conn = accept(sock, NULL, NULL);
    if (conn == -1) {
        perror("accept");
        return -1;
    }

    FILE *in, *out;
    if (create_sock_stream(conn, &in, &out) == -1) {
        fprintf(stderr, "create_sock_stream() failed\n");
        return -1;
    }

    if (recv_http_req(in) == -1) {
        fprintf(stderr, "recv_http_req() failed\n");
        return -1;
    }

    if (send_http_resp(out) == -1) {
        fprintf(stderr, "send_http_resp() failed\n");
        return -1;
    }

    fclose(in);
    fclose(out);

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
