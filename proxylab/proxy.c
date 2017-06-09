#include "csapp.h"
#include <assert.h>
#include <limits.h>

#define MAX_CACHE_OBJECT 100000
#define MAX_CACHE_COUNT 10
#define MAX_CACHE_HASH 1001

#define DEBUG
#define VERBOSE

#define MAX(a,b) ((a)>(b)?(a):(b))
#define MIN(a,b) ((a)<(b)?(a):(b))

enum method {
    METHOD_GET = 0,
    METHOD_POST = 1,
    METHOD_CONNECT = 2,
    METHOD_UNKOWN = 3
};

enum version {
    HTTP_1_1,
    HTTP_UNKOWN
};

struct task {
    int client_fd;
    struct sockaddr_in client_addr;
};

sem_t cache_lock;

struct {
    int next;
    int hash[MAX_CACHE_COUNT];
    size_t len[MAX_CACHE_OBJECT];
    char url[MAX_CACHE_COUNT][MAXLINE];
    char data[MAX_CACHE_COUNT][MAX_CACHE_OBJECT];
} cache;

int hash_code(char *url)
{
    int value = 0;
    for (char *ptr = url; *ptr; ptr++) {
        value *= CHAR_MAX;
        value += *ptr;
        value %= MAX_CACHE_HASH;
    }
    // printf("%s = %d\n", url, value);
    return value;
}

void cache_init()
{
    cache.next = 0;
    for (int i = 0; i < MAX_CACHE_COUNT; i++)
        cache.hash[i] = -1;
    // Init sem
    Sem_init(&cache_lock, 0, 1);
}

void cache_put(char *url, char *buf, size_t len)
{
    int hash = hash_code(url);
    cache.hash[cache.next] = hash;
    cache.len[cache.next] = len;
    strcpy(cache.data[cache.next], buf);
    strcpy(cache.url[cache.next], url);
    cache.next ++;
    cache.next %= MAX_CACHE_COUNT;
}

char *cache_get(char *url, size_t *len)
{
    int hash = hash_code(url);
    for (int i = 0; i < MAX_CACHE_COUNT; i++)
        if (cache.hash[i] == hash && strcmp(url, cache.url[i]) == 0) {
            *len = cache.len[i];
            return cache.data[i];
        }
    return NULL;
}

const char CONNECT_OK[] = "HTTP/1.1 200 Connection established\r\n\r\n";
const char *METHOD_NAME[] = {"GET", "POST"};

void *dispatcher(void *arg);
void tunnel(int client_fd, char *host, char *port);
void forward(int client_fd, enum method met, char *host, char *port, char *reurl, int *keep, char *cachep, int *sizep);
void read_header(int client_fd, char *nheader, int *length, int *proxy_keep, int *chunked);
void parse_first_line(char *line, enum method *met, char *url, enum version *ver);
void parse_host_port(char *url, char *host, char *port);
void parse_url(char *url, char *host, char *port, char *relative_url);
int parse_chunked_header(char *chunked_header);
void proxy_error(int fd, char *cause, char *errornum, char *shortmsg, char *longmsg);
int Read_line(int fd, char *buf, int limit);

int main(int argc, char **argv) {
    if (argc != 2) {
        printf("Usage: %s <port>", argv[0]);
        return 0;
    }
    // Init cache
    cache_init();
    // Listen
    int listen_fd = Open_listenfd(argv[1]);
    printf("Proxy is running...\n");
    while (1) {
        // Accept
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_fd = Accept(listen_fd, (SA *)&client_addr, &client_len);
        // Build arguments
        struct task *arg = (struct task *) Malloc(sizeof(struct task));
        arg->client_fd = client_fd;
        arg->client_addr = client_addr;
        // Dispatch
        pthread_t tid;
        Pthread_create(&tid, NULL, dispatcher, arg);
    }
}

void *dispatcher(void *arg)
{
    Pthread_detach(Pthread_self());
    // Parse arguments
    struct task *tsk = (struct task *) arg;
    int client_fd = tsk->client_fd;
    struct sockaddr_in client_addr = tsk->client_addr;
    Free(arg);
#ifdef DEBUG
    printf("[proxy] serve %s:%d\n", inet_ntoa(client_addr.sin_addr), client_addr.sin_port);
#endif
    // Read first line
    char line[MAXLINE], url[MAXLINE], host[MAXLINE], port[MAXLINE], reurl[MAXLINE];
    enum method met;
    enum version ver;
    if (Read_line(client_fd, line, MAXLINE) <= 0)
        goto dispatcher_exit;
#ifdef DEBUG
    printf("[proxy] recv %s:%d: %s", inet_ntoa(client_addr.sin_addr), client_addr.sin_port, line);
#endif
    parse_first_line(line, &met, url, &ver);
    char cache_temp[MAX_CACHE_OBJECT] = "";
    size_t cache_size = 0;
    char *p;
    int keep = 0;
    switch (met) {
        case METHOD_CONNECT:
            parse_host_port(url, host, port);
            tunnel(client_fd, host, port);
            break;
        case METHOD_POST:
            parse_url(url, host, port, reurl);
            forward(client_fd, met, host, port, reurl, &keep, NULL, NULL);
            break;
        case METHOD_GET:
            // puts(url);
            P(&cache_lock);
            if ((p = cache_get(url, &cache_size))) {
#ifdef DEBUG
                printf("[cache] get %s %d\n", url, cache_size);
#endif
                read_header(client_fd, NULL, NULL, &keep, NULL);
                Rio_writen(client_fd, p, cache_size);
                V(&cache_lock);
            } else {
                V(&cache_lock);
                cache_size = 0;
                parse_url(url, host, port, reurl);
                forward(client_fd, met, host, port, reurl, &keep, cache_temp, &cache_size);
                if (cache_size < MAX_CACHE_OBJECT) {
#ifdef DEBUG
                    printf("[cache] put %s %d\n", url, cache_size);
#endif
                    *(cache_temp + cache_size) = 0;
                    P(&cache_lock);
                    cache_put(url, cache_temp, cache_size);
                    V(&cache_lock);
                    // puts(cache_temp);
                }
            }
            break;
        default:
            read_header(client_fd, NULL, NULL, &keep, NULL);
            proxy_error(client_fd, url, "502", "Proxy error", "Proxy doesn't implement this method");
    }
    // Keep alive
    while (keep) {
#ifdef DEBUG
        printf("[proxy] keep %s:%d\n", inet_ntoa(client_addr.sin_addr), client_addr.sin_port);
#endif
        if (Read_line(client_fd, line, MAXLINE) <= 0)
            break;
#ifdef DEBUG
        printf("[proxy] recv %s:%d: %s", inet_ntoa(client_addr.sin_addr), client_addr.sin_port, line);
#endif
        parse_first_line(line, &met, url, &ver);
        switch (met) {
            case METHOD_POST:
                parse_url(url, host, port, reurl);
                forward(client_fd, met, host, port, reurl, &keep, NULL, NULL);
                break;
            case METHOD_GET:
                // puts(url);
                P(&cache_lock);
                if ((p = cache_get(url, &cache_size))) {
#ifdef DEBUG
                    printf("[cache] get %s %d\n", url, cache_size);
#endif
                    read_header(client_fd, NULL, NULL, &keep, NULL);
                    Rio_writen(client_fd, p, cache_size);
                    V(&cache_lock);
                } else {
                    V(&cache_lock);
                    cache_size = 0;
                    parse_url(url, host, port, reurl);
                    forward(client_fd, met, host, port, reurl, &keep, cache_temp, &cache_size);
                    if (cache_size < MAX_CACHE_OBJECT) {
#ifdef DEBUG
                        printf("[cache] put %s %d\n", url, cache_size);
#endif
                        *(cache_temp + cache_size) = 0;
                        P(&cache_lock);
                        cache_put(url, cache_temp, cache_size);
                        V(&cache_lock);
                        // puts(cache_temp);
                    }
                }
                break;
            default:
                read_header(client_fd, NULL, NULL, &keep, NULL);
                proxy_error(client_fd, url, "502", "Proxy error", "Proxy doesn't implement this method");
        }
    }
    // puts("CLOSE");
dispatcher_exit:
    Close(client_fd);
#ifdef DEBUG
    printf("[proxy] close %s:%d\n", inet_ntoa(client_addr.sin_addr), client_addr.sin_port);
#endif
    return NULL;
}

void Rio_writen_c(int fd, char *buf, size_t len, char *cachep, size_t *sizep)
{
    if (cachep && sizep) {
        size_t old_size = *sizep;
        *sizep += len;
        if (*sizep < MAX_CACHE_OBJECT)
            memcpy(cachep + old_size, buf, len);
    }
    Rio_writen(fd, buf, len);
}

void transfer_response(int client_fd, int server_fd, char *cachep, int *sizep)
{
    char buf[MAXBUF];
    buf[0] = 0;
    int length = -1, chunked = 0;
    read_header(server_fd, buf, &length, NULL, &chunked);
    Rio_writen_c(client_fd, buf, strlen(buf), cachep, sizep);
#ifdef VERBOSE
    printf("%s", buf);
#endif
    if (length != -1) {     // Sized by Content-Length
        int left_length = length;
        while (left_length > 0) {
            int len = MIN(left_length, MAXBUF);
            Rio_readn(server_fd, buf, len);
            Rio_writen_c(client_fd, buf, len, cachep, sizep);
            left_length -= len;
        }
    } else if (chunked) {   // Sized by chunked
        while (length) {
            Read_line(server_fd, buf, MAXBUF);
            Rio_writen_c(client_fd, buf, strlen(buf), cachep, sizep);
            length = parse_chunked_header(buf);
            int left_length = length;
            while (left_length > 0) {
                int len = MIN(left_length, MAXBUF);
                Rio_readn(server_fd, buf, len);
                Rio_writen_c(client_fd, buf, len, cachep, sizep);
                left_length -= len;
            }
            Read_line(server_fd, buf, MAXBUF);
            Rio_writen_c(client_fd, buf, strlen(buf), cachep, sizep);
        }
    } else {                // Sized by EOF
        while ((length = Rio_readn(server_fd, buf, MAXBUF)) > 0)
            Rio_writen_c(client_fd, buf, length, cachep, sizep);
    }
//    Close(client_fd);
    Close(server_fd);
}

void forward(int client_fd, enum method met, char *host, char *port, char *reurl, int *keep, char *cachep, int *sizep)
{
    assert(met == METHOD_POST || met == METHOD_GET);
    char buf[MAXBUF];
    int length = 0;
    // Forward once
    sprintf(buf, "%s %s HTTP/1.1\r\n", METHOD_NAME[met], reurl);
    if (strcmp(port, "80"))
        sprintf(buf, "%sHost: %s:%s\r\n", buf, host, port);
    else
        sprintf(buf, "%sHost: %s\r\n", buf, host);
    read_header(client_fd, buf, &length, keep, NULL);
    // Send request
    int server_fd = Open_clientfd(host, port);
#ifdef VERBOSE
    printf("%s", buf);
#endif
    Rio_writen(server_fd, buf, strlen(buf));
    // Send pay load
    int left_length = length;
    while (left_length > 0) {
        int len = MIN(left_length, MAXBUF);
        Rio_readn(client_fd, buf, len);
        // puts(buf);
        Rio_writen(server_fd, buf, len);
        left_length -= len;
    }
    // Transfer response
    transfer_response(client_fd, server_fd, cachep, sizep);
}

// Transfer data in tunnel
void tunnel(int client_fd, char *host, char *port)
{
    read_header(client_fd, NULL, NULL, NULL, NULL);
    // Response
    Rio_writen(client_fd, CONNECT_OK, strlen(CONNECT_OK));
    // Connect to server
    int server_fd = Open_clientfd(host, port);
    // Set non block
    int val = fcntl(client_fd, F_GETFL, 0);
    fcntl(client_fd, F_SETFL, val | O_NONBLOCK);
    val = fcntl(server_fd, F_GETFL, 0);
    fcntl(server_fd, F_SETFL, val | O_NONBLOCK);
    // Build tunnel
    int n, client_open = 1, server_open = 1;
    char buf[MAXBUF];
    fd_set rest;
    FD_ZERO(&rest);
    while (client_open || server_open) {
        if (client_open)
            FD_SET(client_fd, &rest);
        if (server_open)
            FD_SET(server_fd, &rest);
        int maxfd = MAX(client_fd, server_fd) + 1;
        Select(maxfd, &rest, NULL, NULL, NULL);
        if (FD_ISSET(client_fd, &rest)) {
            if ((n = read(client_fd, buf, MAXBUF)) > 0)
                Rio_writen(server_fd, buf, n);
            else if (n == 0)
                client_open = 0;
        }
        if (FD_ISSET(server_fd, &rest)) {
            if ((n = read(server_fd, buf, MAXBUF)) > 0)
                Rio_writen(client_fd, buf, n);
            else if (n == 0)
                server_open = 0;
        }
    }
    // Close tunnel
//    Close(client_fd);
    Close(server_fd);
}

int Read_line(int fd, char *buf, int limit)
{
    int count = 0;
    while (count < limit) {
        if (Rio_readn(fd, buf, 1) == 0)
            break;
        if (*(buf++) == '\n')
            break;
        count ++;
    }
    *buf = 0;
    return count;
}

void read_header(int client_fd, char *nheader, int *length, int *proxy_keep, int *chunked)
{
    if (proxy_keep)
        *proxy_keep = 0;
    char line[MAXLINE];
    Read_line(client_fd, line, MAXLINE);
    while (strcmp(line, "\r\n")) {
        if (strncasecmp(line, "Host: ", 6) == 0) {
            // Do nothing
        } else if (strncasecmp(line, "Proxy-Connection: ", 18) == 0) {
            if (proxy_keep && strncasecmp(line+18, "keep-alive", 10) == 0)
                *proxy_keep = 1;
        } else if (strncasecmp(line, "Content-Length: ", 16) == 0) {
            if (length)
                *length = atoi(line + 16);
            sprintf(nheader, "%s%s", nheader, line);
        } else if (strncasecmp(line, "Transfer-Encoding: chunked", 26) == 0) {
            if (chunked)
                *chunked = 1;
            sprintf(nheader, "%s%s", nheader, line);
        } else if (nheader) {
            sprintf(nheader, "%s%s", nheader, line);
        }
        Read_line(client_fd, line, MAXLINE);
    }
    if (nheader)
        sprintf(nheader, "%s\r\n", nheader);
}

void proxy_error(int fd, char *cause, char *errornum, char *shortmsg, char *longmsg)
{
    char buf[MAXLINE], body[MAXBUF];
    // Build the HTTP response body
    sprintf(body, "<html><title>Proxy error</title>");
    sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
    sprintf(body, "%s%s: %s\r\n", body, errornum, shortmsg);
    sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
    sprintf(body, "%s<hr><em>The proxy server</em>\r\n", body);
    // Print the HTTP response
    sprintf(buf, "HTTP/1.0 %s %s\r\n", errornum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
    Rio_writen(fd, buf, strlen(buf));
    Rio_writen(fd, body, strlen(body));
}

void parse_first_line(char *line, enum method *met, char *url, enum version *ver)
{
    char *save_ptr;
    char *token = strtok_r(line, " ", &save_ptr);
    assert(token);
    if (strcmp(token, "GET") == 0)
        *met = METHOD_GET;
    else if (strcmp(token, "POST") == 0)
        *met = METHOD_POST;
    else if (strcmp(token, "CONNECT") == 0)
        *met = METHOD_CONNECT;
    else
        *met = METHOD_UNKOWN;
    // Parse URL
    token = strtok_r(NULL, " ", &save_ptr);
    strcpy(url, token);
    // Parse version
    token = strtok_r(NULL, " ", &save_ptr);
    if (strcmp(token, "HTTP/1.1") == 0)
        *ver = HTTP_1_1;
    else
        *ver = HTTP_UNKOWN;
}

void parse_host_port(char *url, char *host, char *port) {
    char *save_ptr;
    // Parse host
    char *token = strtok_r(url, ":", &save_ptr);
    strcpy(host, token);
    // Parse port
    token = strtok_r(NULL, ":", &save_ptr);
    if (token == NULL) {
        strcpy(port, "80");
    } else {
        *(token - 1) = ':';
        strcpy(port, token);
    }
}

void parse_url(char *url, char *host, char *port, char *relative_url)
{
    url += 7;
    // Parse host and port
    char *p = strchr(url, '/');
    *p = 0;
    parse_host_port(url, host, port);
    // Parse relative url
    *p = '/';
    strcpy(relative_url, p);
}

int parse_chunked_header(char *chunked_header)
{
    char ch;
    int i, length = 0;
    for (i = 0; (ch = chunked_header[i]) != '\r'; i++)
        if (isdigit(ch))
            length = length*16 + ch - '0';
        else if (ch >= 'A' && ch <= 'F')
            length = length*16 + ch - 'A' + 10;
        else if (ch >= 'a' && ch <= 'f')
            length = length*16 + ch - 'a' + 10;
        else
            return 0;
    return length;
}