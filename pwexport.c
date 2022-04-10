#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <zip.h>

#ifdef _WIN32
#include <direct.h>
#define slash "\\"
#else
#define slash "/"
#endif

// #define PW_TEST
#ifdef PW_TEST
#define test_printf printf
#else
// #define test_printf (void *)
void test_printf(char *str, ...) { ; };
#endif


#include "memvfs.c"
#include "trie.h"

struct node *folder_id[TRIE_N] = {NULL};

int call_folder(void *data, int argc, char *argv[], char *col[]);

int call_article(void *data, int argc, char *argv[], char *col[]);

int main(int argc, char *argv[]) {

    // cmd
    if (argc < 2) {
        fprintf(stderr, "usage: %s pure_writer_file_name\n"
                        "please delete existing folders before\n",
                argv[0]);
        return 1;
    }
    const char *zip_name = argv[1];
    test_printf("zip_name: %s\n", zip_name);

    // make db_name
    u_long zip_name_len = strlen(zip_name);

    char *db_name = malloc((zip_name_len - 1) * sizeof(zip_name[0]));
    strncpy(db_name, zip_name, zip_name_len - 3);
    strcat(db_name, "db");
    printf("db_name: %s\n", db_name);

    // open zip
    int zip_error;
    zip_t *archive = zip_open(zip_name, ZIP_RDONLY, &zip_error);
    if (archive == NULL) {
        printf("open zip error: %d\n", zip_error);
        return -1;
    }
    test_printf("achieve ptr at %p\n", archive);

    // stat db file in zip
    zip_stat_t sb;
    zip_stat(archive, db_name, ZIP_STAT_SIZE, &sb);
    u_long db_size = sb.size;
    test_printf("stat db un comp size: %lu\n", db_size);

    // open db file in zip
    zip_file_t *zip_db = zip_fopen(archive, db_name, ZIP_FL_UNCHANGED);
    if (zip_db == NULL) {
        zip_error_t *err = zip_get_error(archive);
        printf("open db file in zip failed%s\n", err->str);
        zip_close(archive);
        return -1;
    }

    // read db file in zip to buffer
    void *buffer = malloc(db_size);
    zip_int64_t db_read_len = zip_fread(zip_db, buffer, db_size);
    test_printf("read size: %lu\n", db_read_len);

    // sqlite error :(
    char *error = 0;

    // load sqlite extension: memvfs
    // code from is from the below link
    // https://stackoverflow.com/questions/53448401/sqlite-how-embed-the-memvfs-extension-to-amalgamation
    sqlite3 *test_db;
    // Open an in-memory database to use as a handle for loading the memvfs extension
    if (sqlite3_open_v2(":memory:", &test_db, SQLITE_OPEN_READWRITE, NULL) != SQLITE_OK) {
        fprintf(stderr, "open :memory: %s\n", sqlite3_errmsg(test_db));
        return EXIT_FAILURE;
    }
    sqlite3_enable_load_extension(test_db, 1);
    if (sqlite3_memvfs_init(test_db, &error, 0) != SQLITE_OK_LOAD_PERMANENTLY) {
        fprintf(stderr, "load extension: %s\n", error);
        return EXIT_FAILURE;
    }
    // Done with this database
    sqlite3_close(test_db);
    // load sqlite extension done!

    // real sqlite works
    sqlite3 *db;
    char zUrl[100];

    sprintf(zUrl, "file:/whatever?ptr=%p&sz=%ld&max=%lu",
            buffer, db_read_len, db_size);
    test_printf("zUrl: %s\n", zUrl);

    if (sqlite3_open_v2(zUrl, &db, SQLITE_OPEN_READONLY | SQLITE_OPEN_URI,
                        "memvfs") != SQLITE_OK) {
        fprintf(stderr, "Open db in buffer fail: %s\n", sqlite3_errmsg(db));
        return -1;
    } else {
        fprintf(stderr, "Open db in buffer successfully\n");
    }

    const char *sql_article = "SELECT * from Article";
    const char *sql_folder = "SELECT * from Folder";
    int *call_n_folder = 0;
    int *call_n_article = 0;

    if (sqlite3_exec(db, sql_folder, call_folder, (void *) call_n_folder, &error) != SQLITE_OK) {
        fprintf(stderr, "sql folder fail: %s\n", error);
        sqlite3_free(error);
    } else {
        fprintf(stdout, "sql folder successfully\n");
    }

    if (sqlite3_exec(db, sql_article, call_article, (void *) call_n_article, &error) != SQLITE_OK) {
        fprintf(stderr, "sql article fail: %s\n", error);
        sqlite3_free(error);
    } else {
        fprintf(stdout, "sql article successfully\n");
    }

    sqlite3_close(db);
    trie_free(folder_id);
    free(buffer);
    zip_close(archive);
    return 0;
}

int call_article(void *data, int argc, char *argv[], char *col[]) {

    // 检查魔数
    if (strcmp(col[1], "title") != 0 && strcmp(col[2], "content") != 0 &&
        strcmp(col[5], "extension") != 0 && strcmp(col[10], "folderID") != 0) {
        printf("magic_err\n");
        return -1;
    }

    // 跳过回收站
    if (strcmp(argv[10], "PW_Trash") == 0) {
        return 0;
    }

    // 找到对应文件夹
    char *folder = trie_search(folder_id, argv[10]);

    // 整理文件名
    char name[256];
    strcpy(name, folder);
    strcat(name, slash);
    strcat(name, argv[1] ? argv[1] : argv[0]);
    strcat(name, ".");
    strcat(name, argv[5]);

    // 写入文件
    FILE *file = fopen(name, "w");
    fprintf(file, "%s", argv[2]);
    fclose(file);

    // 自增 1 并打印
    printf("export article successfully! *%d\n", ++(*(int *) data));
    return 0;
}

int call_folder(void *data, int argc, char *argv[], char *col[]) {

    // 检查魔数
    if (strcmp(col[0], "id") != 0 && strcmp(col[1], "name") != 0) {
        printf("magic_err\n");
        return -1;
    }

    // 跳过回收站
    if (strcmp(argv[0], "PW_Trash") == 0) {
        return 0;
    }

    // 保存书籍信息
    trie_insert(folder_id, argv[0], argv[1]);

    // 创建书籍文件夹
#ifdef _WIN32
    int can_mkdir = mkdir(argv[1]);
#else
    int can_mkdir = mkdir(argv[1], S_IRWXU | S_IRWXG | S_IRWXO);
#endif
    if (can_mkdir != 0) {
        printf("make folders fail!\n"
               "please delete existing folders before\n");
        return -1;
    }

    // 打印成功并自增 1
    printf("make folders successfully! *%d\n", ++(*(int *) data));
    return 0;
}