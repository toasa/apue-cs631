#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#include "http.h"

enum HTTP_METHOD {
    GET,
    INVALID,
};

static enum HTTP_METHOD parse_http_method(const char *s) {
    if (strcmp(s, "GET") == 0)
        return GET;

    return INVALID;
}

// Request-Line = Method SP Request-URI SP HTTP-Version CRLF
static int parse_request_line(char *line, enum HTTP_METHOD *method, char **uri) {
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

    char *q = strstr(p, "\r\n");
    if (q == NULL) {
        fprintf(stderr, "invalid request line; CRLF not found\n");
        return -1;
    }
    *q = '\0';

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

static void fmt_time(char *buf, const time_t *t) {
    struct tm *tm = gmtime(t);
    strftime(buf, 100, "%a, %d %b %Y %H:%M:%S GMT", tm);
}

static int send_http_resp_header(FILE *out, const char *filename) {
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

static int send_http_resp_body(FILE *out, const char *filename) {
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
