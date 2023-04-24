#define _XOPEN_SOURCE 700
#include <stdio.h>
#include <stdlib.h>
#include <error.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <memory.h>
#include <pthread.h>
#include <syslog.h>
#include <signal.h>
#include <linux/limits.h>

#define SERVER_FIFO "/tmp/addition_fifo_server"

int fd, fd_server, bytes_read;
char buf1[512], buf2[1024];
char newstring[128];
char my_fifo_name[128];
int programflag = 0;
int sygnal = 0;

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

void odbierz_sygnal(int signum)
{
    sygnal = signum;
}

void handler(int sig)
{
    unlink("addition_fifo_server");
    printf("\n");
    kill(0, SIGKILL);
}

void *send(void *pthread)
{
    chmod(my_fifo_name, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IXGRP);
    signal(SIGQUIT, handler);
    while (1)
    {

        int *myid = (int *)pthread;

        if (programflag == 0)
        {
            strcat(newstring, my_fifo_name);
            // wysylamy wiadomosc na serwer
            if ((fd_server = open(SERVER_FIFO, O_WRONLY)) == -1)
            {
                perror("open: server fifo");
                break;
            }

            if (write(fd_server, newstring, strlen(newstring)) != strlen(newstring))
            {
                perror("write");
                break;
            }

            if (close(fd_server) == -1)
            {
                perror("close");
                break;
            }
            programflag = 1;
        }

        else if (programflag == 1)
        {
            printf("\n");
            if (fgets(buf1, sizeof(buf1), stdin) == NULL)
                break;
            strcpy(buf2, my_fifo_name);
            strcat(buf2, " ");
            strcat(buf2, buf1);

            // wyslij wiadomosc na serwer
            if ((fd_server = open(SERVER_FIFO, O_WRONLY)) == -1)
            {
                perror("open: server fifo");
                break;
            }

            if (write(fd_server, buf2, strlen(buf2)) != strlen(buf2))
            {
                perror("write");
                break;
            }

            if (close(fd_server) == -1)
            {
                perror("close");
                break;
            }
        }
    }
    pthread_exit(NULL);
    return NULL;
}

void *receive(void *pthread)
{
    while (1)
    {
        char read_data[20][100];
        for (int i = 0; i < 20; i++)
        {
            strcpy(read_data[i], "");
        }

        if ((fd = open(my_fifo_name, O_RDONLY)) == -1)
            perror("open");

        memset(buf2, '\0', sizeof(buf2));

        if ((bytes_read = read(fd, buf2, sizeof(buf2))) == -1)
            perror("read");

        if (bytes_read > 0)
        {
            char *token;
            char *rest = buf2;
            int i = 0;
            while ((token = strtok_r(rest, " ", &rest)))
            {
                strcpy(read_data[i], token);
                i++;
            }

            if (strcmp("success", read_data[0]) == 0)
            {
                syslog(LOG_INFO, "Status logowania: %s \nTwoj adres to: %s\n Twoj numer portu wynosi: %s\n", read_data[0], my_fifo_name, read_data[1]);
                printf("Status logowania: %s \nTwoj adres to: %s\nTwoj numer portu wynosi: %s\n", read_data[0], my_fifo_name, read_data[1]);
            }
            else
            {
                printf("\nWiadomosc od %s: ", read_data[1]);
                for (int i = 2; i < 20; i++)
                {
                    printf("%s ", read_data[i]);
                }
            }
        }

        if (close(fd) == -1)
        {
            perror("close");
            break;
        }
    }
    pthread_exit(NULL);
    return NULL;
}

int main(int argc, char **argv)
{
    //TODO --download opcja oraz zapisywanie do logow;
    struct sigaction akcja;
    akcja.sa_handler = odbierz_sygnal;
    akcja.sa_flags = 0;
    if (sigemptyset(&akcja.sa_mask))
    {
        exit(1);
    }
    sigset_t sygnaly;
    sigfillset(&sygnaly);
    sigdelset(&sygnaly, SIGQUIT);
    sigdelset(&sygnaly, SIGKILL);
    sigprocmask(SIG_BLOCK, &sygnaly, NULL);
    if (sigaction(SIGQUIT, &akcja, NULL) == -1)
    {
        perror("sigaction");
        exit(1);
    }

    if (argc < 2)
    {
        printf("Opcje logowania: %s [--start | --login <nazwa_uzytkownika> | --download <sciezka_do_katalogu]\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    if (!strcmp(argv[1], "--start"))
    {
        int fd, fd_klienta, bytes_read, i;
        char fifo_name_set[20][100];
        int counter = 0;
        char return_fifo[20][100];
        char buf[4096];

        printf("Server dziala...\n");
        send_to_log("Serwer zostal wlaczony");
        // tablica stringow ktora przetrzymuje adresy klientow
        for (int i = 0; i < 20; i++)
        {
            strcpy(fifo_name_set[i], "");
        }

        // tworzymy fifo dla servera
        if ((mkfifo(SERVER_FIFO, 0666) == -1) && (errno != EEXIST))
        {
            perror("mkfifo");
            exit(1);
        }
        // otwarcie
        if ((fd = open(SERVER_FIFO, O_RDONLY)) == -1)
            perror("open");

        while (1)
        {

            strcpy(return_fifo[0], "\0");

            for (int i = 0; i < 20; i++)
            {
                strcpy(return_fifo[i], "");
            }

            memset(buf, '\0', sizeof(buf));

            if ((bytes_read = read(fd, buf, sizeof(buf))) == -1)
                perror("read");

            if (bytes_read > 0)
            {
                // Podzial stringa na mniejsze porcje
                char *token;
                char *rest = buf;
                int i = 0;
                while ((token = strtok_r(rest, " ", &rest)) && strlen(token) != 0)
                {
                    strcpy(return_fifo[i], token);
                    i++;
                }

                int sender_number = 0;

                // Dodawanie nowych klientow
                if (strcasecmp(return_fifo[0], "new") == 0)
                {
                    counter++;
                    strcpy(fifo_name_set[counter], return_fifo[1]);
                    sender_number = counter;

                    char *str = return_fifo[1];
                    char success_string[100];
                    char a[5];
                    sprintf(a, "%d", sender_number);

                    strcpy(success_string, "success ");
                    strcat(success_string, a);

                    if ((fd_klienta = open(str, O_WRONLY)) == -1)
                    {
                        perror("open: fifo klienta");
                        continue;
                    }

                    if (write(fd_klienta, success_string, strlen(success_string)) != strlen(success_string))
                        perror("write");

                    if (close(fd_klienta) == -1)
                        perror("close");
                }

                // Pisanie wiadomosci do klienta
                else if (strcasecmp(return_fifo[1], "send") == 0)
                {

                    for (int i = 1; i < 20; i++)
                    {
                        if (strcasecmp(return_fifo[0], fifo_name_set[i]) == 0)
                        {
                            sender_number = i;
                        }
                    }
                    syslog(LOG_INFO, "OTRZYMANO wiadomosc od %d ", sender_number);
                    printf("\n>OTRZYMANO wiadomosc od %d: ", sender_number);
                    for (int i = 3; i < 20; i++)
                        printf("%s ", return_fifo[i]);

                    // Wysylanie wiadomosci do wszystkich
                    if (strcasecmp(return_fifo[2], "all") == 0)
                    {
                        char a[5];

                        for (int j = 1; j < 20; j++)
                        {
                            if (strlen(fifo_name_set[j]) != 0)
                            {

                                char obecnyKlient[100], adres_odbiorcy[500] = "";
                                strcpy(obecnyKlient, fifo_name_set[j]);
                                strcpy(adres_odbiorcy, obecnyKlient);
                                // szukamy indeksu naszego nadawcy z fifo_name_set i dodajemy go do adresu odbiorcy
                                for (int i = 1; i < 20; i++)
                                {
                                    if (strcasecmp(return_fifo[0], fifo_name_set[i]) == 0)
                                    {
                                        sprintf(a, "%d", i);
                                        strcat(adres_odbiorcy, " ");
                                        strcat(adres_odbiorcy, a);
                                        break;
                                    }
                                }

                                // doddajemy wiadomosc
                                for (int i = 3; i < 20; i++)
                                {
                                    if (strlen(return_fifo[i]) != 0)
                                    {
                                        strcat(adres_odbiorcy, " ");
                                        strcat(adres_odbiorcy, return_fifo[i]);
                                    }
                                }

                                if (strcasecmp(return_fifo[0], obecnyKlient) != 0)
                                {

                                    // Wyslanie wiadomosci
                                    if ((fd_klienta = open(obecnyKlient, O_WRONLY)) == -1)
                                    {
                                        perror("open: client fifo");
                                        continue;
                                    }

                                    if (write(fd_klienta, adres_odbiorcy, strlen(adres_odbiorcy)) != strlen(adres_odbiorcy))
                                        perror("write");

                                    if (close(fd_klienta) == -1)
                                        perror("close");
                                }
                                syslog(LOG_INFO, "Wyslij wiadomosc do wszystkich");
                                printf("\n>Wyslij wiadomosc do wszystkich: ");
                                if (sygnal)
                                {
                                        syslog(LOG_INFO, "Serwer zostal wylaczony");
                                }
                                else
                                {
                                    for (int i = 3; i < 20; i++)
                                    {
                                        syslog(LOG_INFO, "%s", return_fifo[i]);
                                        printf("%s ", return_fifo[i]);
                                    }
                                }
                            }
                        }
                    }

                    // Wysylanie wiadomosci do konkretnych uzytkownikow
                    else
                    {
                        char adres_odbiorcy[200] = "";

                        int val = atoi(return_fifo[2]);
                        char a[5];

                        if (strcasecmp(fifo_name_set[val], return_fifo[0]) != 0)
                        {
                            strcpy(adres_odbiorcy, fifo_name_set[val]);

                            for (int i = 1; i < 20; i++)
                            {
                                if (strcasecmp(return_fifo[0], fifo_name_set[i]) == 0)
                                {
                                    sprintf(a, "%d", i);
                                    strcat(adres_odbiorcy, " ");
                                    strcat(adres_odbiorcy, a);
                                    break;
                                }
                            }

                            for (int i = 3; i < 20; i++)
                            {
                                if (strlen(return_fifo[i]) != 0)
                                {
                                    strcat(adres_odbiorcy, " ");
                                    strcat(adres_odbiorcy, return_fifo[i]);
                                }
                            }
                            syslog(LOG_INFO, ">Wiadomosc wyslana do %s", return_fifo[2]);
                            printf("\n>Wiadomosc wyslana do %s: ", return_fifo[2]);
                            for (int i = 3; i < 20; i++)
                                printf("%s ", return_fifo[i]);
                        }


                        if ((fd_klienta = open(fifo_name_set[val], O_WRONLY)) == -1)
                        {
                            perror("open: client fifo");
                            continue;
                        }

                        if (write(fd_klienta, adres_odbiorcy, strlen(adres_odbiorcy)) != strlen(adres_odbiorcy))
                            perror("write");

                        if (close(fd_klienta) == -1)
                            perror("close");
                    }
                }
            }
        }
    }
    else if (!strcmp(argv[1], "--login"))
    {
        sprintf(my_fifo_name, "/tmp/add_client_fifo%ld", (long)getpid());

        if (mkfifo(my_fifo_name, 0664) == -1)
            perror("mkfifo");

        printf("Witam na chacie ! ");
        strcpy(newstring, "new ");

        pthread_t sending_thread, receiving_thread;

        if (pthread_create(&sending_thread, NULL, send, (void *)&sending_thread) != 0)
        {
            fprintf(stderr, "Blad w trakcie tworzenia watku dla funkcji wysylania\n");
            return 1;
        }
        if (pthread_create(&receiving_thread, NULL, receive, (void *)&receiving_thread) != 0)
        {
            fprintf(stderr, "Blad w trakcie tworzenia watku dla funkcji odbierania\n");
            return 1;
        }

        if (pthread_join(sending_thread, NULL))
            fprintf(stderr, "Blad w pthread_join sending\n");

        if (pthread_join(receiving_thread, NULL))
            fprintf(stderr, "Blad w pthread_join receiving\n");

        return 0;
    }
    // TODO --download
}
