#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <syslog.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 9000
#define DATA_FILE "/var/tmp/aesdsocketdata"
#define BUF_SIZE 1024

static int sockfd = -1;
static volatile sig_atomic_t shutdown_flag = 0;

static void sig_handler(int signo)
{
    syslog(LOG_INFO, "Caught signal, exiting");
    shutdown_flag = 1;
}

int main(int argc, char *argv[])
{
    int daemon_mode = 0;

    if(argc > 1 && strcmp(argv[1], "-d") == 0)
        daemon_mode = 1;

    openlog("aesdsocket", LOG_PID, LOG_USER);

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sig_handler;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd == -1){
        syslog(LOG_ERR, "socket: %s", strerror(errno));
        closelog();
        return -1;
    }

    int opt = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);

    if(bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) == -1){
        syslog(LOG_ERR, "bind: %s", strerror(errno));
        close(sockfd);
        closelog();
        return -1;
    }

    if(daemon_mode){
        pid_t pid = fork();
        if(pid < 0){
            syslog(LOG_ERR, "fork: %s", strerror(errno));
            close(sockfd);
            closelog();
            return -1;
        }
        if(pid > 0){
            closelog();
            exit(0);
        }
        setsid();
        int devnull = open("/dev/null", O_RDWR);
        if(devnull != -1){
            dup2(devnull, STDIN_FILENO);
            dup2(devnull, STDOUT_FILENO);
            dup2(devnull, STDERR_FILENO);
            close(devnull);
        }
    }

    if(listen(sockfd, 10) == -1){
        syslog(LOG_ERR, "listen: %s", strerror(errno));
        close(sockfd);
        closelog();
        return -1;
    }

    while(!shutdown_flag){
        struct sockaddr_in client_addr;
        socklen_t addrlen = sizeof(client_addr);

        int connfd = accept(sockfd, (struct sockaddr *)&client_addr, &addrlen);
        if(connfd == -1){
            if(shutdown_flag || errno == EINTR) break;
            syslog(LOG_ERR, "accept: %s", strerror(errno));
            continue;
        }

        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, ip, sizeof(ip));
        syslog(LOG_INFO, "Accepted connection from %s", ip);

        char *buf = NULL;
        size_t buf_len = 0;
        char recv_buf[BUF_SIZE];
        ssize_t n;

        while((n = recv(connfd, recv_buf, sizeof(recv_buf), 0)) > 0){
            char *tmp = realloc(buf, buf_len + n);
            if(!tmp){
                syslog(LOG_ERR, "realloc failed");
                break;
            }
            buf = tmp;
            memcpy(buf + buf_len, recv_buf, n);
            buf_len += n;

            size_t start = 0;
            char *nl;
            while((nl = memchr(buf + start, '\n', buf_len - start)) != NULL){
                size_t pkt_len = nl - (buf + start) + 1;

                int fd = open(DATA_FILE, O_WRONLY | O_CREAT | O_APPEND, 0644);
                if(fd != -1){
                    write(fd, buf + start, pkt_len);
                    close(fd);
                }
                start += pkt_len;

                fd = open(DATA_FILE, O_RDONLY);
                if(fd != -1){
                    char sbuf[BUF_SIZE];
                    ssize_t r;
                    while((r = read(fd, sbuf, sizeof(sbuf))) > 0)
                        send(connfd, sbuf, r, 0);
                    close(fd);
                }
            }

            if(start > 0){
                memmove(buf, buf + start, buf_len - start);
                buf_len -= start;
            }
        }

        free(buf);
        syslog(LOG_INFO, "Closed connection from %s", ip);
        close(connfd);
    }

    close(sockfd);
    remove(DATA_FILE);
    closelog();
    return 0;
}
