#define FUSE_USE_VERSION 28
#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <sys/time.h>
#include <time.h>
#include <ctype.h>
#include <stdlib.h>

static const char *dirpath = "/home/ibrahim-ferel/lawakFS/source";

char katasensitif[1024] = "";
char *lawakwords[100]; 
int countsensitif;
int start_hour = -1;
int end_hour = -1;

void write_log(const char *act, const char *path) {
    FILE *logfile = fopen("/var/log/lawakfs.log", "a");
    if (!logfile) return;

    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    uid_t uid = getuid();

    char tmptime[64];
    strftime(tmptime, sizeof(tmptime), "%Y-%m-%d %H:%M:%S", t);

    fprintf(logfile, "[%s] [%d] [%s] %s\n", tmptime, uid, act, path);
    fclose(logfile);
}

void load_config(const char *lawak_conf) {
    katasensitif[0] = '\0';  
    start_hour = -1;
    end_hour = -1;
    countsensitif = 0;

    FILE *fp = fopen(lawak_conf, "r");
    if (!fp) return;

    char temp[512];
    while (fgets(temp, sizeof(temp), fp)) {
        temp[strcspn(temp, "\n")] = '\0';

        if (strncmp(temp, "SECRET_FILE_BASENAME=", 21) == 0) {
            sscanf(temp + 21, "%255s", katasensitif);
        } 
        else if (strncmp(temp, "ACCESS_START=", 13) == 0) {
            int h, m;
            if (sscanf(temp + 13, "%d:%d", &h, &m) == 2) {
                start_hour = h;
            }
        } 
        else if (strncmp(temp, "ACCESS_END=", 11) == 0) {
            int h, m;
            if (sscanf(temp + 11, "%d:%d", &h, &m) == 2) {
                end_hour = h;
            }
        } 
        else if (strncmp(temp, "FILTER_WORDS=", 13) == 0) {
            char *words = temp + 13;
            char *token = strtok(words, ",");
            while (token && countsensitif < 100) {
                lawakwords[countsensitif++] = strdup(token);
                token = strtok(NULL, ",");
            }
        }
    }

    fclose(fp);
}


int jamSecret() {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    int hour = t->tm_hour;
    return (hour >= start_hour && hour < end_hour); 
}

int cekSecret(const char *path) {
    const char *filename = strrchr(path, '/');
    if (filename == NULL) filename = path;
    else filename++; 

    char name[500];
    strcpy(name, filename);
    char *cp = strrchr(name, '.');
    if (cp) *cp = '\0'; 

    return strcmp(name, katasensitif) == 0;
}

void misteriusNama(const char *path, char *fpath) {
    if(strcmp(path, "/") == 0){ 
        sprintf(fpath, "%s", dirpath);
        return; 
    }

    const char *nama = strrchr(path, '/')+1;
    char tempnama[256];
    strcpy(tempnama, nama); 

    DIR *d = opendir(dirpath);
    struct dirent *de; 

    if(d==NULL){ 
        fpath[0] = '\0';
        return;
    }

    while((de=readdir(d))!=NULL){
        char tmp[256];
        strcpy(tmp, de->d_name); 
        char *ext = strrchr(tmp, '.');
        if(ext) *ext = '\0'; 

        if(strcmp(tempnama, tmp)==0){ 
            sprintf(fpath, "%s/%s", dirpath, de->d_name);
            closedir(d);
            return;
        }
    }

    closedir(d);
    fpath[0] = '\0'; 
}

int secretFile(const char *path) {
    if (strlen(katasensitif) == 0) {
        return 0;
    }

    const char *filename = strrchr(path, '/');
    if (filename == NULL){
        filename = path; 
    }
    else filename++;

    char name[500];
    strcpy(name, filename);
    char *cp = strrchr(name, '.');
    if (cp) *cp = '\0'; 

    return (strcmp(name, katasensitif) == 0);
}

// const char *lawak_words[] = {"ducati", "ferrari", "mu", "chelsea", "prx", "onic", "sisop"};
// const int lawak_words_count = 7;

int detectTXT(const char *path) {
    const char *ext = strrchr(path, '.');
    printf("File extension: '%s'\n", ext ? ext : "(none)");
    return ext && (strcmp(ext, ".txt") == 0);
}

void strtolower(char *tmp, const char *src) {
    int i;
    for (i = 0; src[i] && i < 255; i++) {  
        tmp[i] = tolower((unsigned char)src[i]);
    }
    tmp[i] = '\0'; 
}

void DETECT_LAWAK(char *buf, int size) {
    char *lawakspace = malloc(size * 3);
    lawakspace[0] = '\0';
    
    char *saveptr;
    char *copy = strdup(buf);
    
    char *token = strtok_r(copy, " \n\r\t", &saveptr);
    while (token) {
        // int cek = 0;
        char abclower[256];  
        strtolower(abclower, token); 
        
        for (int i = 0; i < countsensitif; i++) {
            if (strcmp(abclower, lawakwords[i]) == 0) {
                strcat(lawakspace, "lawak");
                // cek = 1;
                break;
            }
        }
        strcat(lawakspace, " ");
        token = strtok_r(NULL, " \n\r\t", &saveptr);
    }
    
    // int len = strlen(lawakspace);
    // if (strlen(lawakspace) > 0 && lawakspace[len-1] == ' ') {
    //     lawakspace[len-1] = '\0';
    // }
    
    strncpy(buf, lawakspace, size - 1);
    buf[size - 1] = '\0';
    
    free(copy);
    free(lawakspace);
}

static const char base64list[] =
    "0123456789qwertyuiopasdfghjklzxcvbnmQWERTYUIOPASDFGHJKLZXCVBNM+/";

int base64_encode(const unsigned char *input, int len, char *output) {
    int i, j;
    for (i = 0, j = 0; i < len;) {
        uint32_t octet_a = i < len ? input[i++] : 0;
        uint32_t octet_b = i < len ? input[i++] : 0;
        uint32_t octet_c = i < len ? input[i++] : 0;
        uint32_t triple = (octet_a << 16) | (octet_b << 8) | octet_c;

        output[j++] = base64list[(triple >> 18) & 0x3F];
        output[j++] = base64list[(triple >> 12) & 0x3F];
        output[j++] = (i > len + 1) ? '=' : base64list[(triple >> 6) & 0x3F];
        output[j++] = (i > len)     ? '=' : base64list[triple & 0x3F];
    }

    return j;  
}

static int xmp_getattr(const char *path, struct stat *stbuf) {
    char fpath[1000];
    misteriusNama(path, fpath);
    if (strlen(fpath) == 0){ 
        return -ENOENT;
    } 

    if (secretFile(path) && !jamSecret()) { 
        return -ENOENT;
    }

    int res = lstat(fpath, stbuf);
    if (res == -1) return -errno;
    return 0;
}

static int xmp_access(const char *path, int mask){ 
    char fpath[1000];
    misteriusNama(path, fpath);
    if(strlen(fpath)==0) return -ENOENT;

    if (secretFile(path) && !jamSecret()) return -ENOENT;

    int res = access(fpath, mask);
    if(res==-1) return -errno;
    write_log("ACCESS", path);

    return 0;
}

static int xmp_open(const char *path, struct fuse_file_info *fi){ 
    char fpath[1000];
    misteriusNama(path, fpath);
    if(strlen(fpath)==0) return -ENOENT;

    if (secretFile(path) && !jamSecret()) return -ENOENT;

    int fd = open(fpath, fi->flags);
    if(fd==-1) return -errno;

    close(fd);
    return 0;
}

static int xmp_opendir(const char *path, struct fuse_file_info *fi){
    char fpath[1000];
    misteriusNama(path, fpath);
    if(strlen(fpath)==0) return -ENOENT;

    if (secretFile(path) && !jamSecret()) return -ENOENT;

    DIR *dp = opendir(fpath);
    if(dp==NULL) return -errno;

    closedir(dp);
    return 0;
}

static int xmp_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi){
    char fpath[1000];
    misteriusNama(path, fpath);
    if(strlen(fpath)==0) return -ENOENT;

    DIR *d;
    struct dirent *de;
    d = opendir(fpath);
    if(d==NULL) return -errno;

    while((de = readdir(d)) != NULL){
        struct stat st;
        memset(&st, 0, sizeof(st));
        st.st_ino = de->d_ino;
        st.st_mode = de->d_type << 12;

        if(de->d_name[0]=='.'){
            if(strcmp(de->d_name,".")==0 || strcmp(de->d_name,"..")==0){
                filler(buf, de->d_name, &st, 0);
            }
            continue;
        }

        char nama[500];
        strcpy(nama, de->d_name);
        char *cek = strrchr(nama, '.');
        if(cek) *cek = '\0'; 
        if(strcmp(nama, "secret") == 0 && !jamSecret()){
            continue; 
        }
        if(filler(buf, nama, &st, 0)) break; 
    }

    closedir(d);
    return 0;
}

static int xmp_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi){
    char fpath[1000]; 
    misteriusNama(path, fpath);
    if(strlen(fpath)==0) return -ENOENT;
    if (secretFile(path) && !jamSecret()) return -ENOENT;

    int fd = open(fpath, O_RDONLY);
    if(fd==-1) return -errno;

    int res = pread(fd, buf, size, offset);
    if(res==-1) {
        close(fd);
        return -errno;
    }

    if (detectTXT(fpath)) {
        buf[res] = '\0'; 
        DETECT_LAWAK(buf, res + 1); 
        res = strlen(buf); 

        write_log("READ", path);
        close(fd);
        return res;
    }

    unsigned char B64[10000];
    int scanning = pread(fd, B64, sizeof(B64), offset);  
    if (scanning == -1) {
        close(fd);
        return -errno;
    }

    char tmpB64[20000];  
    int modify = base64_encode(B64, scanning, tmpB64);

    memcpy(buf, tmpB64, modify);
    write_log("READ", path);
    close(fd);
    return modify;
}


static struct fuse_operations xmp_oper = {
    .getattr = xmp_getattr,
    .opendir = xmp_opendir,
    .readdir = xmp_readdir,
    .read = xmp_read,
    .open = xmp_open,
    .access = xmp_access,
};

int main(int argc, char *argv[]){
    umask(0);
    load_config("lawak.conf");
    return fuse_main(argc, argv, &xmp_oper, NULL);
}
