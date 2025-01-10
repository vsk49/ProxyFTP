#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <netdb.h>
#include <sys/select.h>
#include "./simpleSocketAPI.h"

#define SERVADDR "127.0.0.1"
#define SERVPORT "21"
#define LISTENLEN 1
#define MAXBUFFERLEN 1024
#define MAXHOSTLEN 64
#define MAXPORTLEN 64

int main() {
    int ecode;
    char serverAddr[MAXHOSTLEN];
    char serverPort[MAXPORTLEN];
    int descSockRDV;
    int descSockCOM;
    int descSockServer;
    struct addrinfo hints;
    struct addrinfo *res;
    struct sockaddr_storage myinfo;
    struct sockaddr_storage from;
    socklen_t len;
    char buffer[MAXBUFFERLEN];

    // Initialisation de la socket de RDV IPv4/TCP
    descSockRDV = socket(AF_INET, SOCK_STREAM, 0);
    if (descSockRDV == -1) {
        perror("Erreur création socket RDV\n");
        exit(2);
    }

    // Publication de la socket au niveau du système
    memset(&hints, 0, sizeof(hints));
    hints.ai_flags = AI_PASSIVE;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_family = AF_INET;

    ecode = getaddrinfo(SERVADDR, SERVPORT, &hints, &res);
    if (ecode) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ecode));
        exit(1);
    }

    ecode = bind(descSockRDV, res->ai_addr, res->ai_addrlen);
    if (ecode == -1) {
        perror("Erreur liaison de la socket de RDV");
        exit(3);
    }
    freeaddrinfo(res);

    len = sizeof(struct sockaddr_storage);
    ecode = getsockname(descSockRDV, (struct sockaddr *)&myinfo, &len);
    if (ecode == -1) {
        perror("SERVEUR: getsockname");
        exit(4);
    }
    ecode = getnameinfo((struct sockaddr *)&myinfo, sizeof(myinfo), serverAddr, MAXHOSTLEN, serverPort, MAXPORTLEN, NI_NUMERICHOST | NI_NUMERICSERV);
    if (ecode != 0) {
        fprintf(stderr, "error in getnameinfo: %s\n", gai_strerror(ecode));
        exit(4);
    }
    printf("L'adresse d'ecoute est: %s\n", serverAddr);
    printf("Le port d'ecoute est: %s\n", serverPort);

    ecode = listen(descSockRDV, LISTENLEN);
    if (ecode == -1) {
        perror("Erreur initialisation buffer d'écoute");
        exit(5);
    }

    len = sizeof(struct sockaddr_storage);
    descSockCOM = accept(descSockRDV, (struct sockaddr *)&from, &len);
    if (descSockCOM == -1) {
        perror("Erreur accept\n");
        exit(6);
    }

    // Connexion au serveur FTP
    ecode = connect2Server(SERVADDR, SERVPORT, &descSockServer);
    if (ecode == -1) {
        perror("Erreur connexion au serveur FTP\n");
        exit(7);
    }

    // Redirection des données entre le client et le serveur FTP
    while (true) {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(descSockCOM, &readfds);
        FD_SET(descSockServer, &readfds);
        int maxfd = (descSockCOM > descSockServer) ? descSockCOM : descSockServer;

        ecode = select(maxfd + 1, &readfds, NULL, NULL, NULL);
        if (ecode == -1) {
            perror("Erreur select\n");
            exit(8);
        }

        if (FD_ISSET(descSockCOM, &readfds)) {
            ecode = read(descSockCOM, buffer, MAXBUFFERLEN);
            if (ecode <= 0) break;
            write(descSockServer, buffer, ecode);
        }

        if (FD_ISSET(descSockServer, &readfds)) {
            ecode = read(descSockServer, buffer, MAXBUFFERLEN);
            if (ecode <= 0) break;
            write(descSockCOM, buffer, ecode);
        }
    }

    close(descSockCOM);
    close(descSockServer);
    close(descSockRDV);
    return 0;
}