#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <time.h>
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

    if (strcmp(p, "HTTP/1.0") != 0 && strcmp(p, "HTTP/1.1") != 0) {
        fprintf(stderr, "invalid HTTP version: %s\n", p);
        return -1;
    }

    return 0;
}

int recv_http_req(FILE *in) {
    char line[BUFSIZ];
    if (fgets(line, BUFSIZ, in) == NULL) {
        fprintf(stderr, "fgets() failed\n");
        return -1;
    }

    enum HTTP_METHOD method;
    char *uri;
    if (parse_request_line(line, &method, &uri) == -1) {
        fprintf(stderr, "parse_request_line() failed\n");
        return -1;
    }

    printf("sws: METHOD: %d\n", method);
    printf("sws: URI   : %s\n", uri);

    // TODO: use URI to response HTTP

    while (fgets(line, BUFSIZ, in) != NULL) {
        if (strcmp(line, "\r\n") == 0)
            break; // End of request

        printf("ignored: %s", line);
    }

    return 0;
}

void fmt_time(char *buf, const time_t *t) {
    struct tm *tm = gmtime(t);
    strftime(buf, 100, "%a, %d %b %Y %H:%M:%S GMT", tm);
}

int send_http_resp_header(FILE *out, const char *filename) {
    time_t now = time(NULL);
    char date[100];
    fmt_time(date, &now);

    struct stat st;
    if (stat(filename, &st) == -1) {
        perror("stat()");
        return -1;
    }

    char lm[100];
    fmt_time(lm, &st.st_mtime);

    fprintf(out, "HTTP/1.0 200 OK\r\n");
    fprintf(out, "Date: %s\r\n", date);
    fprintf(out, "Server: sws\r\n");
    fprintf(out, "Last-Modified: %s\r\n", lm);
    fprintf(out, "Content-Type: text/html\r\n");
    fprintf(out, "Content-Length: %ld\r\n", st.st_size);

    return 0;
}

int send_http_resp_body(FILE *out, const char *filename) {
    FILE *fd = fopen(filename, "r");
    if (fd == NULL) {
        perror("fdopen()");
        return -1;
    }

    char buf[BUFSIZ];
    size_t nread;
    while ((nread = fread(buf, 1, BUFSIZ, fd)) > 0) {
        if (fwrite(buf, 1, nread, out) != nread) {
            perror("fwrite()");
            return -1;
        }
    }

    if (ferror(fd)) {
        perror("fread()");
        return -1;
    }

    fclose(fd);

    return 0;
}

int send_http_resp(FILE *out) {
    const char *content = "./index.html"; // TODO: handle for each URI

    send_http_resp_header(out, content);
    fprintf(out, "\r\n");
    send_http_resp_body(out, content);

    return 0;
}

int serve(int sock) {
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
