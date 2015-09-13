/*
 * proxy.c - CS:APP Web proxy
 *
 * TEAM MEMBERS:
 *     zhangzhenghao@zjut.edu.cn
 * 
 * IMPORTANT: Give a high level description of your code here. You
 * must also provide a header comment at the beginning of each
 * function that describes what that function does.
 */ 

#include "csapp.h"

#define MAXRAW (1<<20)

struct task {
	int fd;
	struct sockaddr_in sockaddr;
};

/*
 * Function prototypes
 */
void doit(int fd, struct sockaddr_in sockaddr);
void read_hdrs(rio_t *rp, char *headers, int *length, int *chunked);
int parse_uri(char *uri, char *target_addr, char *path, int *port);
void serve_static(int fd, char *filename, int filesize);
void get_filetype(char *filename, char *filetype);
void serve_dynamic(int fd, char *filename, char *errnum);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);
void format_log_entry(char *logstring, struct sockaddr_in *sockaddr, char *uri, int size);
void *thread(void *vargp);

/* 
 * main - Main routine for the proxy program 
 */
int main(int argc, char **argv)
{
    int listenfd, connfd, port;
    socklen_t clientlen;
    pthread_t tid;
    struct sockaddr_in clientaddr;

    /* Check arguments */
    if (argc != 2) {
    	fprintf(stderr, "Usage: %s <port number>\n", argv[0]);
    	exit(0);
    }
    port = atoi(argv[1]);

    listenfd = Open_listenfd(port);
    while (1) {
        clientlen = sizeof(clientaddr);
        connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
        struct task vargp = {connfd, clientaddr};
        Pthread_create(&tid, NULL, thread, &vargp);
    }
    exit(0);
}

void *thread(void *vargp)
{
	Pthread_detach(pthread_self());
	struct task *tk = (struct task*)vargp;
	doit(tk->fd, tk->sockaddr);
	Close(tk->fd);
	return NULL;
}

/*
 * doit - handles one HTTP transaction
 */
void doit(int fd, struct sockaddr_in sockaddr)
{
    int serverfd, port, content_length, chunked_encode;
    char hostname[MAXLINE], pathname[MAXLINE], headers[MAXBUF], buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    char request[MAXBUF], response[MAXBUF], raw[MAXRAW];
    rio_t rio_client, rio_server;

    /* Read request line and headers */
    Rio_readinitb(&rio_client, fd);
    Rio_readlineb(&rio_client, buf, MAXLINE);
    sscanf(buf, "%s %s %s", method, uri, version);
    read_hdrs(&rio_client, headers, &content_length, &chunked_encode);

    /* Parse URI from request */
    if (parse_uri(uri, hostname, pathname, &port) == -1) {
        clienterror(fd, uri, "502", "Proxy error", "The request is not a HTTP request");
        return;
    }

    /* Build HTTP request */
    sprintf(request, "%s /%s %s\r\n%s\r\n", method, pathname, version, headers);

    printf("[resquest]\n%s\n", request);

    /* Send HTTP resquest to the web server */
    serverfd = Open_clientfd(hostname, port);
    Rio_writen(serverfd, request, strlen(request));

    /* Get response header */
    Rio_readinitb(&rio_server, serverfd);
    read_hdrs(&rio_server, response, &content_length, &chunked_encode);

    /* Send HTTP response to the client */
    Rio_writen(fd, response, strlen(response));

    if (!chunked_encode) {	/* Encode with chunk */

    } else {		/* Define length with Content-length */
    	Rio_readnb(&rio_server, raw, content_length);
    	Rio_writen(fd, raw, content_length);
    }
    Close(serverfd);
  
    printf("[response]\n%s", response);
}

/*
 * clienterror - send an error message to the clinet
 */
 void clienterror(int fd, char *cause, char *errornum, char *shortmsg, char *longmsg)
 {
    char buf[MAXLINE], body[MAXBUF];

    /* Build the HTTP response body */
    sprintf(body, "<html><title>Proxy error</title>");
    sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
    sprintf(body, "%s%s: %s\r\n", body, errornum, shortmsg);
    sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
    sprintf(body, "%s<hr><em>The proxy server</em>\r\n", body);

    /* Print the HTTP response */
    sprintf(buf, "HTTP/1.0 %s %s\r\n", errornum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
    Rio_writen(fd, buf, strlen(buf));
    Rio_writen(fd, body, strlen(body));
 }

/*
 * read_all - get all content
 */
 void read_hdrs(rio_t *rp, char *content, int *length, int *chunked)
 {
    char buf[MAXLINE];
    *length = *chunked = 0;

    Rio_readlineb(rp, buf, MAXLINE);
    strcpy(content, buf);
    while (strcmp(buf, "\r\n")) {
        Rio_readlineb(rp, buf, MAXLINE);
        if (strncasecmp(buf, "Content-Length:", 15) == 0)
            *length = atoi(buf + 15);
        if (strcmp(buf, "Transfer-Encoding: chunked"))
        	*chunked = 1;
        if (strncasecmp(buf, "Proxy-Connection:", 17) == 0) {
        	strcat(content, "Connection: keep-alive");
            continue;
        }
        strcat(content, buf);
    }
 }

/*
 * parse_uri - URI parser
 * 
 * Given a URI from an HTTP proxy GET request (i.e., a URL), extract
 * the host name, path name, and port.  The memory for hostname and
 * pathname must already be allocated and should be at least MAXLINE
 * bytes. Return -1 if there are any problems.
 */
int parse_uri(char *uri, char *hostname, char *pathname, int *port)
{
    char *hostbegin;
    char *hostend;
    char *pathbegin;
    int len;

    if (strncasecmp(uri, "http://", 7) != 0) {
    	hostname[0] = '\0';
    	return -1;
    }
       
    /* Extract the host name */
    hostbegin = uri + 7;
    hostend = strpbrk(hostbegin, " :/\r\n\0");
    len = hostend - hostbegin;
    strncpy(hostname, hostbegin, len);
    hostname[len] = '\0';
    
    /* Extract the port number */
    *port = 80; /* default */
    if (*hostend == ':')   
	*port = atoi(hostend + 1);
    
    /* Extract the path */
    pathbegin = strchr(hostbegin, '/');
    if (pathbegin == NULL)
	   pathname[0] = '\0';
    else {
	   pathbegin++;	
	   strcpy(pathname, pathbegin);
    }

    return 0;
}

/*
 * format_log_entry - Create a formatted log entry in logstring. 
 * 
 * The inputs are the socket address of the requesting client
 * (sockaddr), the URI from the request (uri), and the size in bytes
 * of the response from the server (size).
 */
void format_log_entry(char *logstring, struct sockaddr_in *sockaddr, 
		      char *uri, int size)
{
    time_t now;
    char time_str[MAXLINE];
    unsigned long host;
    unsigned char a, b, c, d;

    /* Get a formatted time string */
    now = time(NULL);
    strftime(time_str, MAXLINE, "%a %d %b %Y %H:%M:%S %Z", localtime(&now));

    /* 
     * Convert the IP address in network byte order to dotted decimal
     * form. Note that we could have used inet_ntoa, but chose not to
     * because inet_ntoa is a Class 3 thread unsafe function that
     * returns a pointer to a static variable (Ch 13, CS:APP).
     */
    host = ntohl(sockaddr->sin_addr.s_addr);
    a = host >> 24;
    b = (host >> 16) & 0xff;
    c = (host >> 8) & 0xff;
    d = host & 0xff;


    /* Return the formatted log entry string */
    sprintf(logstring, "%s: %d.%d.%d.%d %s", time_str, a, b, c, d, uri);
}