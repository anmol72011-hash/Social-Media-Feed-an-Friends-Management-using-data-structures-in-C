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

/* Load all users into hash table */
HashTable *load_users(void) {
    HashTable *ht = create_hash_table();
    FILE *f = fopen(USERS_FILE,"r");
    if (!f) return ht;
    char line[MAX_USERNAME + MAX_PASSWORD + 4];
    while (fgets(line, sizeof(line), f)) {
        line[strcspn(line,"\r\n")]='\0';
        char *pipe = strchr(line,'|');
        if (pipe) { *pipe='\0'; ht_insert(ht, line, pipe+1); }
    }
    fclose(f);
    return ht;
}

/* Append new user */
int save_user(const char *username, const char *password) {
    FILE *f = fopen(USERS_FILE,"a");
    if (!f) return 0;
    fprintf(f,"%s|%s\n", username, password);
    fclose(f);
    return 1;
}

/* Load all posts into queue (FIFO preserved) */
Queue *load_posts(void) {
    Queue *q = create_queue();
    FILE *f = fopen(POSTS_FILE,"r");
    if (!f) return q;
    char line[MAX_LINE];
    while (fgets(line, sizeof(line), f)) {
        line[strcspn(line,"\r\n")]='\0';
        /* Format: username|text|imagefile */
        char *p1 = strchr(line,'|');
        if (!p1) continue;
        *p1='\0';
        char *p2 = strchr(p1+1,'|');
        if (p2) {
            *p2='\0';
            enqueue(q, line, p1+1, p2+1);
        } else {
            enqueue(q, line, p1+1, "");
        }
    }
    fclose(f);
    return q;
}

/* Append post */
int save_post(const char *user, const char *text, const char *img) {
    FILE *f = fopen(POSTS_FILE,"a");
    if (!f) return 0;
    fprintf(f,"%s|%s|%s\n", user, text, img ? img : "");
    fclose(f);
    return 1;
}

/* Remove the LAST post by a given user (for undo) */
int remove_last_post_by_user(const char *username) {
    FILE *f = fopen(POSTS_FILE,"r");
    if (!f) return 0;

    static char lines[MAX_FILE_LINES][MAX_LINE];
    int count=0;
    while (count<MAX_FILE_LINES && fgets(lines[count],MAX_LINE,f))
        lines[count++][strcspn(lines[count-1],"\r\n")]='\0';
    fclose(f);

    int found=-1, i;
    for (i=count-1; i>=0; i--) {
        char tmp[MAX_LINE];
        strncpy(tmp,lines[i],MAX_LINE-1);
        char *pipe=strchr(tmp,'|');
        if (pipe) { *pipe='\0'; if (strcmp(tmp,username)==0){ found=i; break; } }
    }
    if (found<0) return 0;

    f = fopen(POSTS_FILE,"w");
    if (!f) return 0;
    for (i=0; i<count; i++)
        if (i!=found && lines[i][0]) fprintf(f,"%s\n",lines[i]);
    fclose(f);
    return 1;
}

/* Get the last post details by a user (for stack undo display) */
int get_last_post_by_user(const char *username,
                           char *text_out, char *img_out) {
    text_out[0]=img_out[0]='\0';
    FILE *f=fopen(POSTS_FILE,"r");
    if (!f) return 0;
    char line[MAX_LINE];
    int found=0;
    while (fgets(line,sizeof(line),f)) {
        line[strcspn(line,"\r\n")]='\0';
        char *p1=strchr(line,'|');
        if (!p1) continue;
        *p1='\0';
        if (strcmp(line,username)==0) {
            char *p2=strchr(p1+1,'|');
            if (p2) { *p2='\0'; strncpy(text_out,p1+1,MAX_POST_TEXT-1); strncpy(img_out,p2+1,MAX_IMAGE_NAME-1); }
            else     {           strncpy(text_out,p1+1,MAX_POST_TEXT-1); img_out[0]='\0'; }
            found=1;
        }
    }
    fclose(f);
    return found;
}

/* Load friends into graph */
Graph *load_friends(void) {
    Graph *g = create_graph();
    FILE *f = fopen(FRIENDS_FILE,"r");
    if (!f) return g;
    char line[MAX_LINE];
    while (fgets(line,sizeof(line),f)) {
        line[strcspn(line,"\r\n")]='\0';
        char *pipe=strchr(line,'|');
        if (pipe) { *pipe='\0'; add_friendship_graph(g,line,pipe+1); }
    }
    fclose(f);
    return g;
}


int save_friendship(const char *u1, const char *u2) {
    FILE *f=fopen(FRIENDS_FILE,"a");
    if (!f) return 0;
    fprintf(f,"%s|%s\n",u1,u2);
    fclose(f);
    return 1;
}

int friendship_in_file(const char *u1, const char *u2) {
    FILE *f=fopen(FRIENDS_FILE,"r");
    if (!f) return 0;
    char line[MAX_LINE];
    while (fgets(line,sizeof(line),f)) {
        line[strcspn(line,"\r\n")]='\0';
        char *p=strchr(line,'|');
        if (p) {
            *p='\0';
            if ((strcmp(line,u1)==0&&strcmp(p+1,u2)==0)||
                (strcmp(line,u2)==0&&strcmp(p+1,u1)==0))
            { fclose(f); return 1; }
        }
    }
    fclose(f);
    return 0;
}



void generate_token(const char *username, char *token, int maxlen) {
    static unsigned long counter = 0; counter++;
    unsigned long h=5381; const char *s=username;
    while (*s) h=((h<<5)+h)+(unsigned char)*s++;
    unsigned long t=(unsigned long)time(NULL);
    snprintf(token, maxlen, "%lx%lx%lx", h^t, t, counter);
}

void save_session(const char *token, const char *username) {
    FILE *f=fopen(SESSIONS_FILE,"a");
    if (!f) return;
    fprintf(f,"%s|%s\n",token,username);
    fclose(f);
}


int check_auth(char *username_out, char *token_out) {
    username_out[0] = '\0';
    if (token_out) token_out[0]='\0';

    const char *cookie = getenv("HTTP_COOKIE");
    if (!cookie) return 0;

    /* Find sds_session=VALUE in the cookie header */
    char token[128]={0};
    const char *p = strstr(cookie,"sds_session=");
    if (!p) return 0;
    p+=12;
    int i=0;
    while (*p && *p!=';' && *p!=' ' && i<127) token[i++]=*p++;
    token[i]='\0';
    if (!token[0]) return 0;

    /* Look up token in sessions.txt */
    FILE *f=fopen(SESSIONS_FILE,"r");
    if (!f) return 0;
    char line[256];
    int found=0;
    while (fgets(line,sizeof(line),f)) {
        line[strcspn(line,"\r\n")]='\0';
        char *pipe=strchr(line,'|');
        if (pipe) {
            *pipe='\0';
            if (strcmp(line,token)==0) {
                strncpy(username_out,pipe+1,MAX_USERNAME-1);
                username_out[MAX_USERNAME-1]='\0';
                if (token_out) strncpy(token_out,token,127);
                found=1; break;
            }
        }
    }
    fclose(f);
    return found;
}

void clear_session(const char *token) {
    FILE *f=fopen(SESSIONS_FILE,"r");
    if (!f) return;
    static char lines[MAX_SESSIONS][200];
    int count=0;
    while (count<MAX_SESSIONS && fgets(lines[count],200,f)) {
        lines[count][strcspn(lines[count],"\r\n")]='\0';
        if (lines[count][0]) count++;
    }
    fclose(f);
    f=fopen(SESSIONS_FILE,"w");
    if (!f) return;
    int i;
    for (i=0; i<count; i++) {
        char tmp[200]; strncpy(tmp,lines[i],199);
        char *pipe=strchr(tmp,'|');
        if (pipe) { *pipe='\0'; if (strcmp(tmp,token)!=0) fprintf(f,"%s|%s\n",tmp,pipe+1); }
    }
    fclose(f);
}
