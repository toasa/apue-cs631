#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <libgen.h>
#include <sys/stat.h>

int is_dir(const char *filepath) {
    struct stat sb;

    if (stat(filepath, &sb) == -1) {
        perror("stat");
        exit(EXIT_FAILURE);
    }

    return (sb.st_mode & S_IFMT) == S_IFDIR;
}

int is_exist(const char *filepath) {
    struct stat sb;

    if (stat(filepath, &sb) == -1) {
        perror("stat");
        exit(EXIT_FAILURE);
    }

    return (sb.st_mode & S_IFMT) == S_IFREG;
}

ino_t inode(const char *filepath) {
    struct stat sb;

    if (stat(filepath, &sb) == -1) {
        perror("stat");
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

    return 0;
}

// 1. bbcp file1 file2
// 2. bbcp file1 dir
// 3. bbcp file1 dir/file2
// 4. bbcp file1 dir/subdir/subsubdir/file2

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "[Usage] src dst\n");
        exit(EXIT_FAILURE);
    }

    if (is_dir(argv[1])) {
        fprintf(stderr, "src must be a file\n");
        exit(EXIT_FAILURE);
    }

    char dst_path[PATH_MAX];
    if (is_dir(argv[2]))
        snprintf(dst_path, PATH_MAX, "%s/%s", argv[2], basename(argv[1]));
    else
        snprintf(dst_path, PATH_MAX, "%s", argv[2]);

    if (is_exist(argv[2]) && (inode(argv[1]) == inode(argv[2]))) {
        fprintf(stderr, "bbcp: '%s' and '%s' are the same file", argv[1], argv[2]);
        exit(EXIT_FAILURE);
    }

    FILE *src = fopen(argv[1], "r");
    FILE *dst = fopen(argv[2], "w");

    if (copy(src, dst) != 0) {
        fprintf(stderr, "copy() failed\n");
        exit(EXIT_FAILURE);
    }


    exit(EXIT_SUCCESS);
}
