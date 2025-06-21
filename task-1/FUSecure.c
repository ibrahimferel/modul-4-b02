#define FUSE_USE_VERSION 28
#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/types.h>
#include <pwd.h>

static const char *dirpath = "/home/shared_files";


int is_public_path(const char *path) {
    return (strncmp(path, "/public/", 8) == 0 || strcmp(path, "/public") == 0);
}

int is_authorized(const char *path) {
    uid_t uid = getuid();
    struct passwd *pw = getpwuid(uid);
    if (!pw) return 0;
    const char *username = pw->pw_name;

   
    if (strncmp(path, "/private_yuadi", 14) == 0 && strcmp(username, "yuadi") != 0)
        return 0;

    
    if (strncmp(path, "/private_irwandi", 16) == 0 && strcmp(username, "irwandi") != 0)
        return 0;

    return 1; 
}

static int xmp_getattr(const char *path, struct stat *stbuf)
{
    int res;
    char fpath[1000];
    sprintf(fpath, "%s%s", dirpath, path);
    res = lstat(fpath, stbuf);
    if (res == -1) return -errno;
    return 0;
}

static int xmp_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi)
{
    if (!is_public_path(path) && !is_authorized(path)) return -EACCES;

    char fpath[1000];
    if (strcmp(path, "/") == 0) {
        path = dirpath;
        sprintf(fpath, "%s", path);
    } else sprintf(fpath, "%s%s", dirpath, path);

    int res = 0;
    DIR *dp;
    struct dirent *de;
    (void) offset;
    (void) fi;

    dp = opendir(fpath);
    if (dp == NULL) return -errno;

    while ((de = readdir(dp)) != NULL) {
        struct stat st;
        memset(&st, 0, sizeof(st));
        st.st_ino = de->d_ino;
        st.st_mode = de->d_type << 12;
        res = (filler(buf, de->d_name, &st, 0));
        if (res != 0) break;
    }

    closedir(dp);
    return 0;
}

static int xmp_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
    if (!is_public_path(path) && !is_authorized(path)) return -EACCES;

    char fpath[1000];
    sprintf(fpath, "%s%s", dirpath, path);
    int fd = open(fpath, O_RDONLY);
    if (fd == -1) return -errno;

    int res = pread(fd, buf, size, offset);
    if (res == -1) res = -errno;

    close(fd);
    return res;
}


static int xmp_mkdir(const char *path, mode_t mode) { return -EROFS; }
static int xmp_rmdir(const char *path) { return -EROFS; }
static int xmp_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) { return -EROFS; }
static int xmp_create(const char* path, mode_t mode, struct fuse_file_info* fi) { return -EROFS; }
static int xmp_unlink(const char *path) { return -EROFS; }
static int xmp_rename(const char *from, const char *to) { return -EROFS; }
static int xmp_truncate(const char *path, off_t size) { return -EROFS; }
static int xmp_chmod(const char *path, mode_t mode) { return -EROFS; }

static struct fuse_operations xmp_oper = {
    .getattr = xmp_getattr,
    .readdir = xmp_readdir,
    .read = xmp_read,
    .mkdir = xmp_mkdir,
    .rmdir = xmp_rmdir,
    .write = xmp_write,
    .create = xmp_create,
    .unlink = xmp_unlink,
    .rename = xmp_rename,
    .truncate = xmp_truncate,
    .chmod = xmp_chmod,
};

int main(int argc, char *argv[])
{
    umask(0);
    return fuse_main(argc, argv, &xmp_oper, NULL);
}
