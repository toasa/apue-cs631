#include <errno.h>
#include <libgen.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

bool is_dir(const char *filepath) {
    struct stat sb;

    if (stat(filepath, &sb) == -1) {
        perror("stat() is_dir");
        exit(EXIT_FAILURE);
    }

    return (sb.st_mode & S_IFMT) == S_IFDIR;
}

bool is_exist(const char *filepath) {
    struct stat sb;

    int ret = stat(filepath, &sb);
    if (ret == -1 && errno != ENOENT) {
        perror("stat()");
        exit(EXIT_FAILURE);
    }

    return ret == 0;
}

ino_t inode(const char *filepath) {
    struct stat sb;

    if (stat(filepath, &sb) == -1) {
        fprintf(stderr, "%s: ", filepath);
        perror("stat() inode");
        exit(EXIT_FAILURE);
    }

    return sb.st_ino;
}

int copy(FILE *src, FILE *dst) {
    char buf[BUFSIZ];

    size_t nread;
    while ((nread = fread(buf, 1, BUFSIZ, src)) > 0) {
        size_t nwrite = fwrite(buf, 1, nread, dst);
        if (nread != nwrite) {
            fprintf(stderr, "fwrite() failed\n");
            exit(EXIT_FAILURE);
        }
    }

    if (ferror(src)) {
        fprintf(stderr, "fread() failed\n");
        exit(EXIT_FAILURE);
    }

    return 0;
}

void usage(const char *arg0) { fprintf(stderr, "[Usage] %s src dst\n", arg0); }

int main(int argc, char *argv[]) {
    if (argc != 3) {
        usage(argv[0]);
        exit(EXIT_FAILURE);
    }

    if (!is_exist(argv[1])) {
        fprintf(stderr, "bbcp: '%s' No such file or directory\n", argv[1]);
        exit(EXIT_FAILURE);
    }

    if (is_dir(argv[1])) {
        fprintf(stderr, "src must be a file\n");
        exit(EXIT_FAILURE);
    }

    bool dst_exist = is_exist(argv[2]);
    char dst_path[PATH_MAX];
    if (dst_exist && is_dir(argv[2]))
        snprintf(dst_path, PATH_MAX, "%s/%s", argv[2], basename(argv[1]));
    else
        snprintf(dst_path, PATH_MAX, "%s", argv[2]);

    dst_exist = is_exist(dst_path);
    if (dst_exist) {
        if ((inode(argv[1]) == inode(dst_path))) {
            fprintf(stderr, "bbcp: '%s' and '%s' are the same file\n", argv[1],
                    argv[2]);
            exit(EXIT_FAILURE);
        }
    }

    FILE *src = fopen(argv[1], "r");
    FILE *dst = fopen(dst_path, "w");

    if (copy(src, dst) != 0) {
        fprintf(stderr, "copy() failed\n");
        exit(EXIT_FAILURE);
    }

    fclose(src);
    fclose(dst);

    exit(EXIT_SUCCESS);
}
