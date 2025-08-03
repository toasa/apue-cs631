#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

int create_sock_stream(int sock, FILE **in, FILE **out) {
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

enum HTTP_METHOD {
    GET,
    INVALID,
};

enum HTTP_METHOD parse_http_method(const char *s) {
    if (strcmp(s, "GET") == 0)
        return GET;

    return INVALID;
}

// Request-Line = Method SP Request-URI SP HTTP-Version CRLF
int parse_request_line(char *line, enum HTTP_METHOD *method, char **uri) {
    char *p = strchr(line, ' ');
    if (p == NULL) {
        fprintf(stderr, "invalid request line; space not found\n");
        return -1;
    }
    *p = '\0';

    *method = parse_http_method(line);
    if (*method == INVALID) {
        fprintf(stderr, "parse_http_method() failed\n");
        return -1;
    }

    p++;
    *uri = p;

    p = strchr(p, ' ');
    if (p == NULL) {
        fprintf(stderr, "invalid request line; space not found\n");
        return -1;
    }
    *p = '\0';

    p++;

    char *end = strstr(p, "\r\n");
    if (end == NULL) {
        fprintf(stderr, "invalid request line; CRLF not found\n");
        return -1;
    }
    *end = '\0';

    if (strncmp(p, "HTTP/1.0", sizeof("HTTP/1.0")) != 0) {
        fprintf(stderr, "invalid HTTP version: %s\n", p);
        return -1;
    }

    return 0;
}

int handle_http_req(int conn) {
    FILE *in, *out;
    if (create_sock_stream(conn, &in, &out) == -1) {
        fprintf(stderr, "create_sock_stream() failed\n");
        return -1;
    }

    char buf[BUFSIZ];
    if (fgets(buf, BUFSIZ, in) == NULL) {
        fprintf(stderr, "fgets() failed\n");
        return -1;
    }

    enum HTTP_METHOD method;
    char *uri;
    if (parse_request_line(buf, &method, &uri) == -1) {
        fprintf(stderr, "parse_request_line() failed\n");
        return -1;
    }

    printf("sws: METHOD: %d\n", method);
    printf("sws: URL   : %s\n", uri);

    fclose(in);
    fclose(out);

    return 0;
}


int serve(int sock) {
    int conn = accept(sock, NULL, NULL);
    if (conn == -1) {
        perror("accept");
        return -1;
    }

    if (handle_http_req(conn) == -1) {
        fprintf(stderr, "handle_http_req() failed\n");
        return -1;
    }

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
