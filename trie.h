#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <ctype.h>

#define TRIE_N 36
#define TRIE_LEN 12

struct node {
    char *id;
    char *value;
    struct node *next[TRIE_N];
};

void trie_print(struct node *head[]);

void trie_free(struct node *head[]);

int trie_index(const char *id, int n);

bool trie_insert(struct node *head[], const char *id, const char *value);

char* trie_search(struct node *head[], const char *id);

inline void trie_print(struct node *head[]) {
    struct node *org;
    for (int i = 0; i < TRIE_N; i++) {
        org = head[i];
        if (org == NULL) {
            continue;
        }
        if (org->id == NULL) {
            trie_print(org->next);
            continue;
        }
        printf("%s:%s ", org->id, org->value);
    }
}

inline void trie_free(struct node *head[]) {
    struct node *org;
    for (int i = 0; i < TRIE_N; i++) {
        org = head[i];
        if (org == NULL) {
            continue;
        }
        if (org->id == NULL) {
            trie_free(org->next);
            free(org);
            continue;
        }
        free(org->id);
        free(org->value);
        free(org);
    }
}

inline int trie_index(const char *id, int n) {
    int s = tolower((u_char)id[n]);
    if (s >= '0' && s <= '9') {
        return s - '0';
    }
    return s - 'a' + 10;
}

inline bool trie_insert(struct node *head[], const char *id, const char *value) {
    u_long value_len = strlen(value);
    char *tmp_value = (char*)malloc((value_len + 1) * sizeof(char));
    strcpy(tmp_value, value);

    u_long id_len = strlen(id);
    char *tmp_id = (char*)malloc((id_len + 1) * sizeof(char));
    strcpy(tmp_id, id);
    struct node **org = &head[trie_index(id, 0)];

    // 插入
    for (int i = 0; i < id_len; i++) {
        // empty node
        if ((*org) == NULL) {
            (*org) = (struct node *)malloc(sizeof(struct node));
            (*org)->value = tmp_value;
            (*org)->id = tmp_id;
            return true;
        }
        // next node
        if ((*org)->id == NULL) {
            org = &((*org)->next[trie_index(id, i+1)]);
            continue;
        }
        // node is here!
        char *org_id = (*org)->id;
        char *org_value = (*org)->value;
        for (int j = i + 1; j < id_len; j++) {
            (*org)->next[trie_index(id, j)] = (struct node *)malloc(sizeof(struct node));
            (*org)->id = NULL;
            if (org_id[j] == id[j]) { // 下一位相等
                org = &((*org)->next[trie_index(id, j)]);
                continue;
            }
            (*org)->next[trie_index(id, j)]->value = tmp_value;
            (*org)->next[trie_index(id, j)]->id = tmp_id;
            (*org)->next[trie_index(org_id, j)] = (struct node *)malloc(sizeof(struct node));
            (*org)->next[trie_index(org_id, j)]->value = org_value;
            (*org)->next[trie_index(org_id, j)]->id = org_id;
            return true;
        }
    }
    return false;
}

inline char* trie_search(struct node *head[], const char *id){
    struct node *org = head[trie_index(id, 0)];
    for (int i = 0; i < TRIE_LEN; i++) {
        if (org==NULL){
            return NULL;
        }
        if (org->id==NULL){
            org = org->next[trie_index(id, i+1)];
            continue;
        }
        return org->value;
    }
    return NULL;
}