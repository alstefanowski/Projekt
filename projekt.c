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
#include <signal.h>
#include "dane.h"
#include <sys/ipc.h>
#include <sys/shm.h>
#include <dirent.h>

#define MAX 5
int fd_r;        // deskryptor dla klienta do czytania converted_text_fd
int fd_w;        // deskryptor dla klienta do pisania raw_text_fd
int public_fifo; // deskryptor dla serwera
int fd_user;
pthread_mutex_t lock;
int registered_users[MAX_USERS];
int num_registered_users = 0;
User users_table[MAX_USERS];
int server_fd;

pid_t clientid;
User user;
int sygnal;

void odbierz_sygnal(int signum)
{
    sygnal = signum;
}

void send_to_log(char *info) // info to zmienna tekstowa do zapisywania w logach
{
    time_t t = time(NULL);
    struct tm date = *localtime(&t);
    char out[PATH_MAX];
    snprintf(out, sizeof(out), "%d-%d-%d %d:%d:%d - %s", date.tm_year + 1900, date.tm_mon + 1, date.tm_mday, date.tm_hour, date.tm_min, date.tm_sec, info);
    openlog("projekt", LOG_PID | LOG_CONS | LOG_NDELAY, LOG_LOCAL1);
    syslog(LOG_NOTICE, out, getuid());
    closelog();
}

/*
void clean()
{
    close(public_fifo);
    close(fd_w);
    unlink(user.message_from_server);
    unlink(user.message_to_server);
}
*/
/*
void register_user()
{
    pthread_mutex_lock(&num_registered_users_mutex);
    num_registered_users++;
    pthread_mutex_unlock(&num_registered_users_mutex);
}
void unregister_user()
{
    pthread_mutex_lock(&num_registered_users_mutex);
    num_registered_users--;
    pthread_mutex_unlock(&num_registered_users_mutex);
}

void *user_threads(void *arg)
{
    char *username = (char*) arg;
    int server_fifo;
    if((server_fifo = open(PUBLIC, O_WRONLY))==-1)
    {
        perror("open");
        exit(1);
    }

}
*/
void forwardMessage(Message *message)
{
    if((write(server_fd, message, sizeof(Message))) == -1) {
        perror("write");
        exit(1);       
    }
}
void handle_sigquit(int sig)
{
    if (sig == SIGQUIT)
    {
        for (int i = 0; i < num_registered_users; i++)
        {
            Message log;
            strcpy(log.nadawca, users_table[i].nickname);
            strcpy(log.odbiorca, "Server");
            strcpy(log.message, "Uzytkownik sie wylogowal");
            forwardMessage(&log);
            close(users_table[i].fifo_fd);
        }
        exit(0);
    }
}
/*
void forward_message_serv(int nadawca, int odbiorca, char *info)
{
    char fifo_user[HALFPIPE_BUF];
    sprintf(fifo_user, "/home/kali/Projekt/user_fifo%d", odbiorca);
    int fd_user;
    if ((fd_user = open(fifo_user, O_WRONLY | O_NONBLOCK)) == -1)
    {
        syslog(LOG_ERR, "BLAD PRZY WYSYLANIU WIADOMOSCI OD %d DO %d: %s", nadawca, odbiorca, strerror(errno));
    }
    else
    {
        char msg[256];
        snprintf(msg, sizeof(msg), "From user: %d: %s", nadawca, info);
        write(fd_user, msg, strlen(msg) + 1);
        close(fd_user);
    }
}
*/
/*
void forward_message(Message *mess)
{
    int server_fd;
    send_to_log("tutaj");
    if ((server_fd = open(PUBLIC, O_WRONLY)) == -1)
    {
        perror("PUBLIC");
        exit(1);
    }
    send_to_log("witam");
    write(server_fd, mess, sizeof(Message));
    send_to_log("koniec");
    close(server_fd);
}
*/
void receive_mess(int fifo)
{
    Message message;
    ssize_t bytes_read;
    while ((bytes_read = read(fifo, (char *)&message, sizeof(Message))) > 0)
    {
        if (!strcmp(message.odbiorca, "Serwer"))
        {
            if (!strcmp(message.message, "shutdown"))
            {
                Message shutdown_message;
                strcpy(shutdown_message.nadawca, "Serwer");
                strcpy(shutdown_message.odbiorca, "Wszyscy");
                strcpy(shutdown_message.message, "Serwer off");
                for (int i = 0; i < num_registered_users; i++)
                    forwardMessage(&shutdown_message);
                break;
            }
        }
        else
        {
            send_to_log("ELSEE");
            int odbiorca_fd = -1;
            for (int i = 0; i < num_registered_users; i++)
            {
                if (!strcmp(users_table[i].nickname, message.odbiorca))
                    odbiorca_fd = users_table[i].fifo_fd;
                break;
            }
            if (odbiorca_fd != -1)
                write(odbiorca_fd, &message, sizeof(Message));
            else
                fprintf(stderr, "NOT FOUND");
        }
    }
}
/*
void sum_files()
{
    int file_count;
    DIR *dirp;
    struct dirent *entry;

    dirp = opendir("/home/albert/Desktop/Projekt");
    while (entry = readdir(dirp) != NULL) {
        if(strcmp(entry ->d_type, "F") == 0) {
            file_count++;
        }
    }
    return file_count;
}

void copy_file(const char* SPath, const char* TPath) {
    FILE *SPath = fopen(SPath, "rb");
    if (SPath==NULL) {
        perror("FILE");
        exit(1);
    }
    FILE *TPath = fopen(TPath, "wb");
    if (TPath == NULL ) {
        perror("ERROR");
        exit(1);
    }
    char bufor[1024];
    ssize_t bytes_read;
}
*/
int main(int argc, char **argv)
{

    User user;
    Message message;
    char bufor[BUFSIZ];
    if (argc < 2)
    {
        printf("Opcje logowania: %s [--start | --login <nazwa_uzytkownika> | --download <sciezka_do_katalogu]\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    if (!strcmp(argv[1], "--start"))
    {
        if (fork() == 0)
        {

            if (mkfifo(PUBLIC, 0666) < 0)
            {
                perror("mkfifo");
                exit(1);
            }
            setsid();
            close(STDIN_FILENO);
            close(STDOUT_FILENO);
            close(STDERR_FILENO);
            if ((server_fd = open(PUBLIC, O_RDONLY)) == -1)
            {
                perror("open");
                exit(1);
            }
            signal(SIGQUIT, handle_sigquit);
            while (1)
            {
                Message message;
                if (read(server_fd, &message, sizeof(Message)) > 0)
                {
                    num_registered_users++;
                    for (int i = 0; i < num_registered_users; i++)
                    {
                        if (!strcmp(users_table[i].nickname, message.odbiorca))
                        {
                            send_to_log("write");
                            write(users_table[i].fifo_fd, &message, sizeof(Message));
                            break;
                        }
                    }
                }
            }
        }
    }

    else if (!strcmp(argv[1], "--login"))
    {
        char fifo_name[HALFPIPE_BUF];
        openlog("chat_user", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
        syslog(LOG_INFO, "User %s sie zalogowal:", argv[2]);
        if (num_registered_users > MAX_USERS)
        {
            syslog(LOG_ERR, "Limit uzytkownikow", strerror(errno));
            exit(1);
        }
        int index = -1;
        send_to_log("HALO");
        for (int i = 0; i < num_registered_users; i++)
        {
            send_to_log("for");
            if (!strcmp(users_table[i].nickname, argv[2]))
            {
                send_to_log("index");
                index = i;
                break;
            }
        }
        if (index == -1)
        {
            pthread_mutex_lock(&lock);
            send_to_log("index3");
            User new_user;
            strcpy(new_user.nickname, argv[2]);
            sprintf(fifo_name, "/home/albert/Desktop/Projekt/Fifo%d", getpid());
            mkfifo(fifo_name, 0666);
            new_user.fifo_fd = open(fifo_name, O_RDWR);
            users_table[num_registered_users] = new_user;
            index = num_registered_users - 1;
        }
        else
        {
            send_to_log("index4");
            Message login;
            strcpy(login.nadawca, argv[2]);
            strcpy(login.odbiorca, "Serwer");
            strcpy(login.message, "User connected");
            forwardMessage(&login);
        }
        pid_t pid = fork();
        if (pid == -1)
        {
            perror("pid");
            exit(EXIT_FAILURE);
        }
        // dziecko otrzymuje wiadomosci z serwera
        else if (pid == 0)
        {
            receive_mess(users_table[num_registered_users - 1].fifo_fd);
            exit(EXIT_SUCCESS);
        }
        // Rodzic wysyla wiadomosci na serwer;
        else
        {
            memset(bufor, 0x0, BUFSIZ);
            Message message;
            char *username;
            char username_bufor[BUFSIZ];
            memset(username_bufor, 0x0, BUFSIZ);
            strcpy(message.nadawca, argv[2]);
            strcpy(message.odbiorca, "Serwer");
            while (1)
            {
                fgets(bufor, BUFSIZ, stdin);
                bufor[BUFSIZ - 1] = '\0';
                if (!strcmp(message.odbiorca, "quit"))
                {
                    strcpy(message.nadawca, argv[2]);
                    strcpy(message.message, "Uzytkownik wyologowany");
                    forwardMessage(&message); // message.nadawca, message.odbiorca, message.message
                    break;
                }
                else
                {
                    strcpy(message.message, bufor);
                    strcpy(message.odbiorca, username_bufor);
                    syslog(LOG_INFO, "Wiadomosc %s wyslana na serwer",message.message);
                    forwardMessage(&message);
                }
            }
            close(server_fd);
            wait(NULL);
            exit(EXIT_SUCCESS);
        }
    }
    else if(!strcmp(argv[1], "--download")) {
        const char* target_directory_path = argv[2];
        const char* source_directory_path;
        Message msg;
        char address[256];
        snprintf(address, sizeof(address), "%s/%s", target_directory_path, msg.nadawca);
    }
    closelog();
    return 0;
}