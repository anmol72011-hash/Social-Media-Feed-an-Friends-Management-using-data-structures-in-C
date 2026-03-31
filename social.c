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
typedef struct AdjNode {
    char            username[MAX_USERNAME];
    struct AdjNode *next;
} AdjNode;

typedef struct GraphNode {
    char             username[MAX_USERNAME];
    AdjNode         *adjList;
    struct GraphNode*next;
} GraphNode;

typedef struct { GraphNode *head; } Graph;

Graph *create_graph(void) {
    Graph *g = (Graph *)malloc(sizeof(Graph));
    if (!g) return NULL;
    g->head = NULL;
    return g;
}

GraphNode *find_graph_node(Graph *g, const char *u) {
    GraphNode *c = g->head;
    while (c) { if (strcmp(c->username, u)==0) return c; c=c->next; }
    return NULL;
}

GraphNode *add_graph_node(Graph *g, const char *u) {
    GraphNode *n = find_graph_node(g, u);
    if (n) return n;
    n = (GraphNode *)malloc(sizeof(GraphNode));
    if (!n) return NULL;
    strncpy(n->username, u, MAX_USERNAME-1);
    n->username[MAX_USERNAME-1] = '\0';
    n->adjList = NULL;
    n->next = g->head;
    g->head = n;
    return n;
}

static void add_adj_edge(GraphNode *node, const char *nb) {
    AdjNode *c = node->adjList;
    while (c) { if (strcmp(c->username,nb)==0) return; c=c->next; }
    AdjNode *a = (AdjNode *)malloc(sizeof(AdjNode));
    if (!a) return;
    strncpy(a->username, nb, MAX_USERNAME-1);
    a->username[MAX_USERNAME-1] = '\0';
    a->next = node->adjList;
    node->adjList = a;
}

void add_friendship_graph(Graph *g, const char *u1, const char *u2) {
    GraphNode *n1 = add_graph_node(g, u1);
    GraphNode *n2 = add_graph_node(g, u2);
    if (n1 && n2) { add_adj_edge(n1, u2); add_adj_edge(n2, u1); }
}

int are_friends(Graph *g, const char *u1, const char *u2) {
    GraphNode *n1 = find_graph_node(g, u1);
    if (!n1) return 0;
    AdjNode *a = n1->adjList;
    while (a) { if (strcmp(a->username,u2)==0) return 1; a=a->next; }
    return 0;
}

void free_graph(Graph *g) {
    GraphNode *c = g->head;
    while (c) {
        AdjNode *a = c->adjList;
        while (a) { AdjNode *t=a; a=a->next; free(t); }
        GraphNode *t = c; c=c->next; free(t);
    }
    free(g);
}



typedef struct PostNode {
    char             username[MAX_USERNAME];
    char             text[MAX_POST_TEXT];
    char             image[MAX_IMAGE_NAME];
    struct PostNode *next;
} PostNode;

typedef struct {
    PostNode *front;
    PostNode *rear;
    int       size;
} Queue;

Queue *create_queue(void) {
    Queue *q = (Queue *)malloc(sizeof(Queue));
    if (!q) return NULL;
    q->front = q->rear = NULL;
    q->size = 0;
    return q;
}

void enqueue(Queue *q, const char *user, const char *text, const char *img) {
    PostNode *n = (PostNode *)malloc(sizeof(PostNode));
    if (!n) return;
    strncpy(n->username, user, MAX_USERNAME-1); n->username[MAX_USERNAME-1]='\0';
    strncpy(n->text,     text, MAX_POST_TEXT-1); n->text[MAX_POST_TEXT-1]='\0';
    if (img) { strncpy(n->image, img, MAX_IMAGE_NAME-1); n->image[MAX_IMAGE_NAME-1]='\0'; }
    else n->image[0] = '\0';
    n->next = NULL;
    if (!q->rear) { q->front = q->rear = n; }
    else          { q->rear->next = n; q->rear = n; }
    q->size++;
}

void free_queue(Queue *q) {
    PostNode *c = q->front;
    while (c) { PostNode *t=c; c=c->next; free(t); }
    free(q);
}
