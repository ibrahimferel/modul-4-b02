[![Review Assignment Due Date](https://classroom.github.com/assets/deadline-readme-button-22041afd0340ce965d47ae6ef1cefeee28c7c493a6346c4f15d667ab976d596c.svg)](https://classroom.github.com/a/V7fOtAk7)
|    NRP     |      Name      |
| :--------: | :------------: |
| 5025221049 | Ibrahim Ferel |
| 5025221000 | Student 2 Name |
| 5025221000 | Student 3 Name |

# Praktikum Modul 4 _(Module 4 Lab Work)_

</div>

### Daftar Soal _(Task List)_

- [Task 1 - FUSecure](/task-1/)

- [Task 2 - LawakFS++](/task-2/)

- [Task 3 - Drama Troll](/task-3/)

- [Task 4 - LilHabOS](/task-4/)

### Laporan Resmi Praktikum Modul 4 _(Module 4 Lab Work Report)_

### [Task 1 - FUSecure]

### [Task 2 - LawakFS++]
#### lawak.c
```c
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

```
#### Penjelasan lawak.c
##### Pendahuluan
1. Fungsi Main, menerima beberapa argumen dan masuk ke sub-fungsi load_config untuk menyalin isi dari lawak.conf kedalam beberapa nama file yang sudah saya buat. Yakni katasensitif, countsensitif, start_hour, end_hour.
2. Kita bikin char *dirpath yang berisikan path menuju ke direktori source yang kita bikin.
3. Masuk ke fungsi fuse, ada getattr, opendir, readdir, read, open, dan juga access.

##### *A. Menyembunyikan ekstensi dari setiap file.*
Pendahuluan Problem A : Karena kita perlu menyembunyikan ektensi dari setiap file ketika kita melakukan command "ls", maka hal yang perlu diperhatikan dan dimodifikasi adalah utamanya terkait readdir. Namun ketika kita ingin melakukan perintah seperti "cat", kita hanya boleh menuliskan nama filenya tanpa ekstensinya juga (contoh : "cat temulawak", maka nanti akan dimunculkan hasil tulisan dari temulawak.txt). Hal ini memicu saya untuk membuat suatu sub-fungsi baru yakni misteriusNama, yang berfungsi sebagai petunjuk ke path asli dari suatu file.

*Penjelasan misteriusNama :*
- Menerima path dari input
- Kalo semisal mengakses root, langsung return
- Lalu kita cari nama file setelah slash terakhir setelah dirpath dan di copy namanya kedalam nama variabel yang bernama tempnama. Dalam hal ini berarti tempnama tidak mengandung ekstensi dari file, karena ini adalah input dari user.
- Lalu kita buka direktori source, dan baca satu persatu nama filenya (dengan ekstensi) yang dimasukkan kedalam variabel tmp.
- Masih dalam source, hilangkan ekstensi tmp dan compare dengan tempnama yang tadi kita buat.
- Apabila sama antara tempnama dan tmp print dirpath/namafile ke dalam fpath yang akan dikembalikan kedalam sub-fungsi fuse kita yakni getattr, opendir, readdir, read, open, dan juga access
- Tutup direktori
  
*Penjelasan readdir :*
- Menerima fpath dari misteriusNama, kalo gaada return ENOENT
- Buka direktori source, cek nama file
- Lalu kita buat variabel "nama", yang mana menampung nama file dari source tanpa ekstensi.
- Gunakan filler untuk menampilkan nama file tanpa ekstensi pada saat user menulis command "ls"

##### *B. Akses terbatas untuk file dengan nama secret*
Pendahuluan Problem B : Pertama kita harus menentukan algoritma untuk mencari nama file yang bernama secret di dalam pathnya menggunakan solusi yang saya tawarkan yakni membuat sub-fungsi secretFile. Dan disini karena harus membatasi akses pada jam-jam tertentu, kita akan bikin sub-fungsi jamSecret untuk menentukan jam aksesnya.

*Penjelasan secretFile :*
- Pertama kita buat variabel "filename" untuk menampung nama file setelah dipisah dengan jalur pathnya
- Lalu kita buat variabel "nama" untuk menampung nama filenya tanpa ekstensi
- Lalu bandingkan apakah "nama" ini sama dengan kata "secret" dan return hasilnya
  
*Penjelasan jamSecret :*
- Dapetin waktu sekarang (local_time)
- track hour, dan set dibatasi dari jam 08.00 hingga 18.00

Lalu yang perlu kita lakukan yakni meng-Update getattr, readdir, open, access, dan read.

##### *B. Akses terbatas untuk file dengan nama secret*
Pendahuluan Problem B : Pertama kita harus menentukan algoritma untuk mencari nama file yang bernama secret di dalam pathnya menggunakan solusi yang saya tawarkan yakni membuat sub-fungsi secretFile. Dan disini karena harus membatasi akses pada jam-jam tertentu, kita akan bikin sub-fungsi jamSecret untuk menentukan jam aksesnya.

*Penjelasan secretFile :*
- Pertama kita buat variabel "filename" untuk menampung nama file setelah dipisah dengan jalur pathnya
- Lalu kita buat variabel "nama" untuk menampung nama filenya tanpa ekstensi
- Lalu bandingkan apakah "nama" ini sama dengan kata "secret" dan return hasilnya
  
*Penjelasan jamSecret :*
- Dapetin waktu sekarang (local_time)
- track hour, dan set dibatasi dari jam 08.00 hingga 18.00

Lalu yang perlu kita lakukan yakni meng-Update getattr, readdir, open, access, dan read.

##### *B. Akses terbatas untuk file dengan nama secret*
Pendahuluan Problem B : Pertama kita harus menentukan algoritma untuk mencari nama file yang bernama secret di dalam pathnya menggunakan solusi yang saya tawarkan yakni membuat sub-fungsi secretFile. Dan disini karena harus membatasi akses pada jam-jam tertentu, kita akan bikin sub-fungsi jamSecret untuk menentukan jam aksesnya.

*Penjelasan secretFile :*
- Pertama kita buat variabel "filename" untuk menampung nama file setelah dipisah dengan jalur pathnya
- Lalu kita buat variabel "nama" untuk menampung nama filenya tanpa ekstensi
- Lalu bandingkan apakah "nama" ini sama dengan kata "secret" dan return hasilnya
  
*Penjelasan jamSecret :*
- Dapetin waktu sekarang (local_time)
- track hour, dan set dibatasi dari jam 08.00 hingga 18.00

Lalu yang perlu kita lakukan yakni meng-Update getattr, readdir, open, access, dan read.

##### *B. Akses terbatas untuk file dengan nama secret*
Pendahuluan Problem B : Pertama kita harus menentukan algoritma untuk mencari nama file yang bernama secret di dalam pathnya menggunakan solusi yang saya tawarkan yakni membuat sub-fungsi secretFile. Dan disini karena harus membatasi akses pada jam-jam tertentu, kita akan bikin sub-fungsi jamSecret untuk menentukan jam aksesnya.

*Penjelasan secretFile :*
- Pertama kita buat variabel "filename" untuk menampung nama file setelah dipisah dengan jalur pathnya
- Lalu kita buat variabel "nama" untuk menampung nama filenya tanpa ekstensi
- Lalu bandingkan apakah "nama" ini sama dengan kata "secret" dan return hasilnya
  
*Penjelasan jamSecret :*
- Dapetin waktu sekarang (local_time)
- track hour, dan set dibatasi dari jam 08.00 hingga 18.00

Lalu yang perlu kita lakukan yakni meng-Update getattr, readdir, open, access, dan read.


#### Beberapa bukti SS dan/atau hasil Output
#### Kendala

#### lawak.conf
```bash
FILTER_WORDS=ducati,ferrari,mu,chelsea,prx,onic,sisop
SECRET_FILE_BASENAME=secret
ACCESS_START=08:00
ACCESS_END=18:00
```
#### Penjelasan lawak.conf
1. Baris pertama menandakan bahwa kata-kata yang dianggap "lawak" tertera pada list didepan "FILTER_WORDS"
2. Baris kedua menandakan apa saja nama file yang sensitif dan hanya bisa diakses pada jam tertentu
3. Baris ketiga dan keempat merupakan jam dimulai dan jam berakhirnya untuk access nama file sensitif yang disebutkan pada poin 2

### [Task 3 - Drama Troll]

### [Task 4 - LilHabOS]
