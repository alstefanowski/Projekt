#define _XOPEN_SOURCE 700
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include <time.h>
#include <linux/limits.h>
#include <syslog.h>
#include <errno.h>

#define PUBLIC      "/home/albert/Desktop/Projekt/FIFO_SERVER"
#define PRIVATE     "/home/kali/Projekt/FIFO_CLIENT"
#define HALFPIPE_BUF (PIPE_BUF/2)
#define MAXTRIES 5
#define WARNING "warning"
#define MAX_USERS 100
#define SHM_KEY 12345

typedef struct user {
    char nickname[HALFPIPE_BUF];
    int fifo_fd;
    int num_of_users;
} User;

typedef struct message {
    char message[HALFPIPE_BUF];
    char odbiorca[HALFPIPE_BUF];
    char nadawca[HALFPIPE_BUF];
} Message;