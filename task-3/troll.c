#define FUSE_USE_VERSION 28
#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

const char *troll_path = "/mnt/troll";
const char *file1 = "/very_spicy_info.txt";
const char *file2 = "/upload.txt";

static int troll_getattr(const char *path, struct stat *stbuf)
{
    memset(stbuf, 0, sizeof(struct stat));
    if (strcmp(path, "/") == 0) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
    } else if (strcmp(path, file1) == 0 || strcmp(path, file2) == 0) {
        stbuf->st_mode = S_IFREG | 0644;
        stbuf->st_nlink = 1;
        stbuf->st_size = 1024;
    } else return -ENOENT;

    return 0;
}

static int troll_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi)
{
    if (strcmp(path, "/") != 0)
        return -ENOENT;

    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);
    filler(buf, file1 + 1, NULL, 0);  
    filler(buf, file2 + 1, NULL, 0);

    return 0;
}

static int troll_open(const char *path, struct fuse_file_info *fi)
{
    if (strcmp(path, file1) != 0 && strcmp(path, file2) != 0)
        return -ENOENT;

    return 0;
}

static int troll_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
    const char *content;

    if (strcmp(path, file1) == 0) {
        content = "Confidential. Do not share.\n";
    } else if (strcmp(path, file2) == 0) {
        FILE *log = fopen("/tmp/troll_log.txt", "a");
        if (log) {
            time_t now = time(NULL);
            fprintf(log, "[%s] DainTontas triggered the trap via upload.txt\n", ctime(&now));
            fclose(log);
        }
        content = "Nice try. But now you’re being watched.\n";
    } else return -ENOENT;

    size_t len = strlen(content);
    if (offset >= len) return 0;

    if (offset + size > len) size = len - offset;
    memcpy(buf, content + offset, size);

    return size;
}

static struct fuse_operations troll_oper = {
    .getattr = troll_getattr,
    .readdir = troll_readdir,
    .open    = troll_open,
    .read    = troll_read,
};

int main(int argc, char *argv[])
{
    return fuse_main(argc, argv, &troll_oper, NULL);
}
