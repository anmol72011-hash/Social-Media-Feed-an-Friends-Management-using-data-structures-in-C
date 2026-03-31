#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#ifdef _WIN32
  #include <io.h>
  #include <fcntl.h>
  #define SET_STDIN_BINARY() _setmode(_fileno(stdin), _O_BINARY)
#else
  #define SET_STDIN_BINARY()
#endif

#define HASH_SIZE       101
#define MAX_USERNAME    64
#define MAX_PASSWORD    128
#define MAX_POST_TEXT   512
#define MAX_IMAGE_NAME  256
#define MAX_LINE        900


#define XAMPP_ROOT    "C:\\xampp"
#define DATA_DIR      XAMPP_ROOT "\\htdocs\\socialds\\data\\"
#define USERS_FILE    DATA_DIR "users.txt"
#define POSTS_FILE    DATA_DIR "posts.txt"
#define FRIENDS_FILE  DATA_DIR "friends.txt"
#define SESSIONS_FILE DATA_DIR "sessions.txt"
#define UPLOADS_DIR   XAMPP_ROOT "\\htdocs\\socialds\\uploads\\"

#define CGI_URL       "/cgi-bin/socialds/social.exe"
#define UPLOADS_WEB   "/socialds/uploads/"
#define CSS_URL       "/socialds/style.css"
#define LOGIN_URL     "/socialds/login.html"

#define MAX_FILE_LINES 5000
#define MAX_SESSIONS   500


typedef struct UserNode {
    char             username[MAX_USERNAME];
    char             password[MAX_PASSWORD];
    struct UserNode *next;
} UserNode;

typedef struct {
    UserNode *buckets[HASH_SIZE];
} HashTable;


unsigned int hash_function(const char *str) {
    unsigned long h = 5381;
    int c;
    while ((c = (unsigned char)*str++))
        h = ((h << 5) + h) + c;
    return (unsigned int)(h % HASH_SIZE);
}

HashTable *create_hash_table(void) {
    HashTable *ht = (HashTable *)malloc(sizeof(HashTable));
    if (!ht) return NULL;
    memset(ht->buckets, 0, sizeof(ht->buckets));
    return ht;
}

int ht_insert(HashTable *ht, const char *username, const char *password) {
    unsigned int idx = hash_function(username);
    UserNode *curr = ht->buckets[idx];
    while (curr) {
        if (strcmp(curr->username, username) == 0) return 0; /* duplicate */
        curr = curr->next;
    }
    UserNode *node = (UserNode *)malloc(sizeof(UserNode));
    if (!node) return -1;
    strncpy(node->username, username, MAX_USERNAME - 1);
    node->username[MAX_USERNAME - 1] = '\0';
    strncpy(node->password, password, MAX_PASSWORD - 1);
    node->password[MAX_PASSWORD - 1] = '\0';
    node->next          = ht->buckets[idx];
    ht->buckets[idx]    = node;
    return 1;
}

int ht_search(HashTable *ht, const char *username) {
    unsigned int idx = hash_function(username);
    UserNode *curr = ht->buckets[idx];
    while (curr) {
        if (strcmp(curr->username, username) == 0) return 1;
        curr = curr->next;
    }
    return 0;
}

int ht_check_password(HashTable *ht, const char *username, const char *password) {
    unsigned int idx = hash_function(username);
    UserNode *curr = ht->buckets[idx];
    while (curr) {
        if (strcmp(curr->username, username) == 0)
            return strcmp(curr->password, password) == 0;
        curr = curr->next;
    }
    return 0;
}

void free_hash_table(HashTable *ht) {
    int i;
    for (i = 0; i < HASH_SIZE; i++) {
        UserNode *curr = ht->buckets[i];
        while (curr) {
            UserNode *tmp = curr;
            curr = curr->next;
            free(tmp);
        }
    }
    free(ht);
}
