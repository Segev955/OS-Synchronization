/*
** server.c -- a stream socket server demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <pthread.h>
#include <malloc.h>
#include "synchronization.h"
#include "myMalloc.c"



#define PORT "3490"  // the port users will be connecting to

#define BACKLOG 10     // how many pending connections queue will hold

#define NUM_CLIENTS 10 // max of clients

pStack head = NULL; // Stack
pthread_mutex_t mutex;

void sigchld_handler(int s) {
    (void) s; // quiet unused variable warning

    // waitpid() might overwrite errno, so we save and restore it:
    int saved_errno = errno;

    while (waitpid(-1, NULL, WNOHANG) > 0);

    errno = saved_errno;
}


// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in *) sa)->sin_addr);
    }

    return &(((struct sockaddr_in6 *) sa)->sin6_addr);
}

int count = 0;

void push(char *str, pStack *head) {
    pthread_mutex_lock(&mutex);
    if (count == 1024) {
        printf("ERROR: Stack full\n");
        return;
    }
//    pStack node = (pStack)(malloc(sizeof(Stack)));
    pStack node = (pStack)(_malloc(sizeof(Stack)));
    if (node == NULL) {
        perror("Malloc failed");
        exit(0);
    }
    bzero(node->stack, 1024);
    strcpy(node->stack, str); //input data
    node->next = *head;
    *head = node;
    printf("'%s' pushed to stack\n", str);
    count++;
    pthread_mutex_unlock(&mutex);
}

void pop(pStack *head) {
    pthread_mutex_lock(&mutex);
    if (count == 0) {
        printf("ERROR: Stack empty\n");
        return;
    }
    pStack tmp = *head;
    *head = (*head)->next;
    printf("'%s' poped\n", tmp->stack);
//    free(tmp);
    _free(tmp);
    count--;
    pthread_mutex_unlock(&mutex);
}

void top(pStack *head) {
    pthread_mutex_lock(&mutex);
    if (count == 0) {
        printf("ERROR: Stack empty\n");
        return;
    }
    printf("OUTPUT: ");
    printf("%s\n", (*head)->stack);
    pthread_mutex_unlock(&mutex);
}

int checkSUB(char e[], char s[]) {
    if (strlen(s) < strlen(e))
        return 0;
    for (int i = 0; i < strlen(e); ++i) {
        if (s[i] != e[i])
            return 0;
    }
    return 1;
}

void *myThreadFun(void *arg) {
/*    sleep(5);
    if (send(new_fd, "Hello, world!", 13, 0) == -1)
        perror("send");
    close(new_fd);
    printf("server: end connection\n");*/
    int new_fd = *(int *) arg;
    char text[1024];
    while (1) {
        char str[1024];
        bzero(str, 1024);
        bzero(text, 1024);
        int msglen;
/*        for (int i = 0; i < 1024; i++) {
            if (text[i - 1] == '\n') {
                text[i - 1] = '\0';
                break;
            }
            scanf("%c", &text[i]);
        }*/
        if ((msglen = recv(new_fd, text, 1024 - 1, 0)) == -1) {
            perror("recv error");
            exit(1);
        }
        if (!msglen) {
            printf("Client disconnect\n");
            close(new_fd);
            return NULL;
        }
        text[msglen] = '\0';
        printf("Received: '%s'\n", text);
        if (checkSUB("STOP", text)) {
            printf("See Ya");
            close(new_fd);
            break;
        } else if (checkSUB("PUSH ", text)) {
            for (int i = 5; i < strlen(text); ++i) {
                str[i - 5] = text[i];
            }
            push(str, &head);
        } //POP
        else if (checkSUB("POP", text)) {
            pop(&head);
        } //TOP
        else if (checkSUB("TOP", text)) {
            top(&head);
        } //STOP
    }
}


int main(void) {
    int sockfd, new_fd;  // listen on sock_fd, new connection on new_fd
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage their_addr; // connector's address information
    socklen_t sin_size;
    struct sigaction sa;
    int yes = 1;
    char s[INET6_ADDRSTRLEN];
    int rv;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // use my IP

    if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and bind to the first we can
    for (p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                             p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }

        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
                       sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("server: bind");
            continue;
        }

        break;
    }

    freeaddrinfo(servinfo); // all done with this structure

    if (p == NULL) {
        fprintf(stderr, "server: failed to bind\n");
        exit(1);
    }

    if (listen(sockfd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }

    sa.sa_handler = sigchld_handler; // reap all dead processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }

    printf("server: waiting for connections...\n");
    int n = 0;
    pthread_t thread[NUM_CLIENTS];
    while (1) {  // main accept() loop
        sin_size = sizeof their_addr;
        new_fd = accept(sockfd, (struct sockaddr *) &their_addr, &sin_size);
        if (new_fd == -1) {
            perror("accept");
            continue;
        }

        inet_ntop(their_addr.ss_family,
                  get_in_addr((struct sockaddr *) &their_addr),
                  s, sizeof s);
        printf("server: got connection\n");
        pthread_mutex_init(&mutex, NULL);
        if (pthread_create(&thread[n++], NULL, &myThreadFun, &new_fd) != 0)
            printf("Thread error\n");
//        n++;
//        printf("Thread start\n");
        if (n >= NUM_CLIENTS) {
            n = 0;
            while (n < NUM_CLIENTS) {
                pthread_join(thread[n++], NULL); // equivalent of wait()
            }
            n = 0;
        }
        pthread_mutex_destroy(&mutex);
    }

    return 0;
}
