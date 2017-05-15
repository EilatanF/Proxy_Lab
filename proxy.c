/* Natalie Fan 
 * AndrewID: nfan
 *
 *
 * proxy is a simple, tiny proxy that handles HTTP/1.0 calls
 * between client and server.
 */


#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";
static const char *cnt_hdr = "Connection: close\r\n";
static const char *pxy_cnt_hdr = "Proxy-Connection: close\r\n\r\n";

void *thread(void *vargp);
void doit(int fd);
void read_requesthdrs(rio_t *rp);
int parse_uri(char *uri, char *host, char *port, char *path);
void clienterror(int fd, char *cause, char *errnum, 
		 char *shortmsg, char *longmsg);

int main(int argc, char **argv) 
{
    int listenfd, *connfdp;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    pthread_t tid;

    /* Check command line args */
    if (argc != 2) {
	fprintf(stderr, "usage: %s <port>\n", argv[0]);
	exit(1);
    }

    listenfd = Open_listenfd(argv[1]);
    while (1) {
        clientlen = sizeof(clientaddr);
        connfdp = Malloc(sizeof(int));
        *connfdp = Accept(listenfd, (SA *)&clientaddr, &clientlen);
        Pthread_create(&tid, NULL, thread, (void *) connfdp);
    }
}
/* $end tinymain */


/* 
 * thread - thread routine
 */
 
 /* $begin thread */
 void *thread (void *vargp){
     int connfd = *((int *)vargp);
     Pthread_detach(pthread_self());
     Free(vargp);
     doit(connfd);
     Close(connfd);
     return NULL;
 }
 /* $end thread */

/*
 * doit - handle one HTTP request/response transaction
 */
/* $begin doit */
void doit(int fd) 
{
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    char host[MAXLINE], path[MAXLINE], port[MAXLINE];
    rio_t rio;
    rio_t rios;
    int serverfd;
    char gethost[MAXLINE], hst_hdr[MAXLINE];
    size_t n;

    /* Read request line and headers */
    Rio_readinitb(&rio, fd);
    if (!Rio_readlineb(&rio, buf, MAXLINE))
        return;
    printf("%s", buf);
    sscanf(buf, "%s %s %s", method, uri, version);
    if (strcasecmp(method, "GET")) { 
        clienterror(fd, method, "501", "Not Implemented",
                    "Tiny does not implement this method");
        return;
    }
    
    /* Parse URI from GET request */
    parse_uri(uri, host, port, path);

    serverfd = Open_clientfd(host, port);
    Rio_readinitb(&rios, serverfd);
    
    sprintf(gethost, "GET %s HTTP/1.0\r\n", path);
    sprintf(hst_hdr, "Host: %s\r\n", host);
    
    Rio_writen(serverfd, (void *)gethost, strlen(gethost));
    Rio_writen(serverfd, (void *)hst_hdr, strlen(hst_hdr));
    Rio_writen(serverfd, (void *)user_agent_hdr, strlen(user_agent_hdr));
    Rio_writen(serverfd, (void *)cnt_hdr, strlen(cnt_hdr));
    Rio_writen(serverfd, (void *)pxy_cnt_hdr, strlen(pxy_cnt_hdr));
    
    while((n = Rio_readlineb(&rios, buf, MAXLINE)) != 0){
        Rio_writen(fd, buf, n);
    }
    Close(serverfd);
}
/* $end doit */


/*
 * parse_uri - parse URI into hostname, port, and path.
 * if there is no specific port, then port is set to 80 as default.
 */
/* $begin parse_uri */
int parse_uri(char *uri, char *host, char *port, char *path) 
{
    char *ptr;
    

    if (strstr(uri, "http://") != NULL) { 
        uri = uri + 7;
    }
    printf("uri: %s\n", uri);
	
    if ((ptr = strstr(uri, ":")) != NULL) {
        sscanf(uri, "%[^:]", host);
        ptr = strstr(ptr, ":") + 1;
        sscanf(ptr, "%[^/]%s", port, path);
    }else {
        sscanf(uri, "%[^/]%s", host, path);
        port = "80";
    }
    
    printf("host: %s, port: %s, path: %s\n", host, port, path);
    return 1;
    
}
/* $end parse_uri */



/*
 * clienterror - returns an error message to the client
 */
/* $begin clienterror */
void clienterror(int fd, char *cause, char *errnum, 
		 char *shortmsg, char *longmsg) 
{
    char buf[MAXLINE], body[MAXBUF];

    /* Build the HTTP response body */
    sprintf(body, "<html><title>Tiny Error</title>");
    sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
    sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
    sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
    sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body);

    /* Print the HTTP response */
    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
    Rio_writen(fd, buf, strlen(buf));
    Rio_writen(fd, body, strlen(body));
}
/* $end clienterror */

