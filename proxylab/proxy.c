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

/* Task args */
struct task {
	int fd;
	struct sockaddr_in sockaddr;
};

/* Mutex */
static sem_t open_clientfd_mutex;
static sem_t log_mutex;

/* Log file */
FILE *pLog;

/*
 * Function prototypes
 */
void doit(int fd, struct sockaddr_in sockaddr);
void read_hdrs(rio_t *rp, char *headers, int *length, int *chunked);
int parse_uri(char *uri, char *target_addr, char *path, int *port);
int parse_chunked_header(char *chunked_header);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);
void format_log_entry(char *logstring, struct sockaddr_in *sockaddr, char *uri, int size);
void *thread(void *vargp);
void Rio_writen_w(int fd, void *usrbuf, size_t n);
ssize_t Rio_readnb_w(rio_t *rp, void *usrbuf, size_t n);
ssize_t Rio_readlineb_w(rio_t *rp, void *usrbuf, size_t maxlen);
int open_clientfd_ts(char *hostname, int port);

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

    /* Initial mutex */
    Sem_init(&open_clientfd_mutex, 0, 1);
    Sem_init(&log_mutex, 0, 1);

    /* Ignore SIGPIPE signals */
    Signal(SIGPIPE, SIG_IGN);

    /* Open log file */
    pLog = fopen("proxy.log", "a");

    /* Listen */
    listenfd = Open_listenfd(port);
    printf("Proxy is running...\n");
    while (1) {
        clientlen = sizeof(clientaddr);
        connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
        /* Pass args */
        struct task *vargp= (struct task *) Malloc(sizeof(struct task));
        vargp->fd = connfd;
        vargp->sockaddr = clientaddr;
        /* Create thread */
        Pthread_create(&tid, NULL, thread, vargp);
    }
    exit(0);
}

void *thread(void *vargp)
{
	Pthread_detach(pthread_self());
	struct task *thread_task = (struct task *) vargp;
	doit(thread_task->fd, thread_task->sockaddr);
	close(thread_task->fd);
    Free(vargp);
	return NULL;
}

/*
 * doit - handles one HTTP transaction
 */
void doit(int fd, struct sockaddr_in sockaddr)
{
    int serverfd, port, content_length, chunked_encode, chunked_length, size = 0;
    char hostname[MAXLINE], pathname[MAXLINE], buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE], logstring[MAXLINE];
    char headers[MAXBUF], request[MAXBUF], response[MAXBUF];
    rio_t rio_client, rio_server;

    /* Read request line and headers */
    Rio_readinitb(&rio_client, fd);
    if (Rio_readlineb_w(&rio_client, buf, MAXLINE) <= 0)
    	return;

    /* Get request type*/
    sscanf(buf, "%s %s %s", method, uri, version);
    if (strcmp(method, "POST") && strcmp(method, "GET")) {
        clienterror(fd, uri, "502", "Proxy error", "Proxy doesn't implement this method");
        return;
    }

    /* Get full headers */
    read_hdrs(&rio_client, headers, &content_length, &chunked_encode);

    /* Parse URI from request */
    if (parse_uri(uri, hostname, pathname, &port) == -1) {
        clienterror(fd, uri, "502", "Proxy error", "Proxy doesn't implement this uri");
        return;
    }

    /* Build HTTP request */
    sprintf(request, "%s /%s %s\r\n%s", method, pathname, version, headers);

    /* Send HTTP resquest to the web server */
    if ((serverfd = open_clientfd_ts(hostname, port)) == -1)
    	return;
    Rio_writen_w(serverfd, request, strlen(request));
    if (strcmp(method, "POST") == 0) {	/* POST request */
    	Rio_readnb_w(&rio_client, buf, content_length);
    	Rio_writen_w(serverfd, buf, content_length);
    }

    /* Get response header */
    Rio_readinitb(&rio_server, serverfd);
    read_hdrs(&rio_server, response, &content_length, &chunked_encode);

    /* Send HTTP response to the client */
    Rio_writen_w(fd, response, strlen(response));

    /* Send response content to the client */
    if (chunked_encode) {	                      /* Encode with chunk */
    	Rio_readlineb_w(&rio_server, buf, MAXLINE);
    	Rio_writen_w(fd, buf, strlen(buf));
    	while ((chunked_length = parse_chunked_header(buf)) > 0) {
            size += chunked_length;
    		Rio_readnb_w(&rio_server, buf, chunked_length);
    		Rio_writen_w(fd, buf, chunked_length);
    		Rio_readlineb_w(&rio_server, buf, MAXLINE);
    		Rio_writen_w(fd, buf, strlen(buf));
    		Rio_readlineb_w(&rio_server, buf, MAXLINE);
    		Rio_writen_w(fd, buf, strlen(buf));
    	}
    	Rio_readlineb_w(&rio_server, buf, MAXLINE);
    	Rio_writen_w(fd, buf, strlen(buf));
    } else if (content_length > 0) {				/* Define length with Content-length */
        size += content_length;
        int left_length = content_length;
        int handle_length = 0;
    	while (left_length > 0) {
            handle_length = left_length > MAXBUF ? MAXBUF : left_length;
            left_length -= handle_length;
            Rio_readnb_w(&rio_server, buf, handle_length);
            Rio_writen_w(fd, buf, handle_length);
        }
    } else {                                        /* Define length with closing connection */
    	while ((chunked_length = Rio_readlineb_w(&rio_server, buf, MAXBUF)) > 0) {
            size += chunked_length;
    		Rio_writen_w(fd, buf, chunked_length);
        }
    }

    /* Write log file */
    P(&log_mutex);
    format_log_entry(logstring, &sockaddr, uri, size);
    fprintf(pLog, "%s\n", logstring);
    V(&log_mutex);

    /* Close connection to server */
    close(serverfd);
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
    Rio_writen_w(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n");
    Rio_writen_w(fd, buf, strlen(buf));
    sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
    Rio_writen_w(fd, buf, strlen(buf));
    Rio_writen_w(fd, body, strlen(body));
 }

/*
 * read_header - get request header
 *
 * Read headers of request. Return -1 if there is any problem.
 */
 void read_hdrs(rio_t *rp, char *content, int *length, int *chunked)
 {
    char buf[MAXLINE];
    *length = *chunked = 0;

    Rio_readlineb_w(rp, buf, MAXLINE);
    strcpy(content, buf);
    strcat(content, "Connection: close\r\n");
    while (strcmp(buf, "\r\n")) {
        Rio_readlineb_w(rp, buf, MAXLINE);
        /* Get 'Content-Length:' */
        if (strncasecmp(buf, "Content-Length:", 15) == 0)
            *length = atoi(buf + 15);
        /* Get 'Transfer-Encoding: chunked' */
        if (strncasecmp(buf, "Transfer-Encoding: chunked", 26) == 0)
        	*chunked = 1;
        /* Remove 'Connection' and 'Proxy-Connection' */
        if (strncasecmp(buf, "Proxy-Connection:", 17) == 0 || strncasecmp(buf, "Connection:", 11) == 0)
            continue;
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
    sprintf(logstring, "%s: %d.%d.%d.%d %s %d", time_str, a, b, c, d, uri, size);
}

/*
 * Parse chunked header - Get chunked length from header 
 */
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
			return -1;
	return length;
}

/*************************************
 * Robust I/O routines wrapper for web
 *************************************/
void Rio_writen_w(int fd, void *usrbuf, size_t n) 
{
    if (rio_writen(fd, usrbuf, n) != n)
        fprintf(stderr, "Rio_writen_w error: %s\n", strerror(errno));
}

ssize_t Rio_readnb_w(rio_t *rp, void *usrbuf, size_t n) 
{
    ssize_t rc;

    if ((rc = rio_readnb(rp, usrbuf, n)) < 0) {
        fprintf(stderr, "Rio_readnb_w error: %s\n", strerror(errno));
        return 0;
    }
    return rc;
}

ssize_t Rio_readlineb_w(rio_t *rp, void *usrbuf, size_t maxlen) 
{
    ssize_t rc;

    if ((rc = rio_readlineb(rp, usrbuf, maxlen)) < 0) {
        fprintf(stderr, "Rio_readlineb_w error: %s\n", strerror(errno));
        return 0;
    }
    return rc;
} 

/*
 * open_clientfd (thread safe version) - open connection to server at <hostname, port> 
 *   and return a socket descriptor ready for reading and writing.
 *   Returns -1 and sets errno on Unix error. 
 *   Returns -2 and sets h_errno on DNS (gethostbyname) error.
 */
/* $begin open_clientfd_ts */
int open_clientfd_ts(char *hostname, int port) 
{
    int clientfd;
    struct hostent *hp, *sharedp;
    struct sockaddr_in serveraddr;

    if ((clientfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    return -1; /* check errno for cause of error */

    /* Fill in the server's IP address and port */
    P(&open_clientfd_mutex);
    if ((sharedp = gethostbyname(hostname)) == NULL) {
    	V(&open_clientfd_mutex);	/* very important */
    	return -2; 					/* check h_errno for cause of error */
    }
    hp = (struct hostent*) malloc(sizeof(struct hostent));
    memcpy(hp, sharedp, sizeof(struct hostent));
    V(&open_clientfd_mutex);
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    bcopy((char *)hp->h_addr_list[0], 
      (char *)&serveraddr.sin_addr.s_addr, hp->h_length);
    serveraddr.sin_port = htons(port);
    free(hp);

    /* Establish a connection with the server */
    if (connect(clientfd, (SA *) &serveraddr, sizeof(serveraddr)) < 0)
    return -1;
    return clientfd;
}
/* $end open_clientfd */