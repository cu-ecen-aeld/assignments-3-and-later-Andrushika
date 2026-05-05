#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <syslog.h>
#include <errno.h>

int main(int argc, char *argv[])
{
    openlog("writer", LOG_PID, LOG_USER);

    if (argc != 3) {
        syslog(LOG_ERR, "Usage: writer <writefile> <writestr>");
        fprintf(stderr, "Usage: writer <writefile> <writestr>\n");
        closelog();
        return 1;
    }

    const char *writefile = argv[1];
    const char *writestr = argv[2];

    syslog(LOG_DEBUG, "Writing %s to %s", writestr, writefile);

    int fd = open(writefile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        syslog(LOG_ERR, "Failed to open %s: %s", writefile, strerror(errno));
        closelog();
        return 1;
    }

    ssize_t bytes_written = write(fd, writestr, strlen(writestr));
    if (bytes_written == -1) {
        syslog(LOG_ERR, "Failed to write to %s: %s", writefile, strerror(errno));
        close(fd);
        closelog();
        return 1;
    }

    close(fd);
    closelog();
    return 0;
}
