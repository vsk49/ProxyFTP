#define _POSIX_C_SOURCE 200809L
#include  <stdio.h>
#include  <stdlib.h>
#include  <sys/socket.h>
#include  <sys/types.h>
#include  <string.h>
#include  <unistd.h>
#include  <stdbool.h>
#include  <netdb.h>
#include "./simpleSocketAPI.h"
#include <sys/select.h>

#define SERVADDR "127.0.0.1"        // Définition de l'adresse IP d'écoute
#define SERVPORT "0"                // Définition du port d'écoute, si 0 port choisi dynamiquement
#define LISTENLEN 1                 // Taille de la file des demandes de connexion
#define MAXBUFFERLEN 1024           // Taille du tampon pour les échanges de données
#define MAXHOSTLEN 64               // Taille d'un nom de machine
#define MAXPORTLEN 64               // Taille d'un numéro de port

int main() {
    int ecode;                       // Code retour des fonctions
    char serverAddr[MAXHOSTLEN];     // Adresse du serveur
    char serverPort[MAXPORTLEN];     // Port du server
    int descSockRDV;                 // Descripteur de socket de rendez-vous
    int descSockCOM;                 // Descripteur de socket de communication
    int descSockServer;              // Descripteur de socket pour le serveur FTP
    struct addrinfo hints;           // Contrôle la fonction getaddrinfo
    struct addrinfo *res;            // Contient le résultat de la fonction getaddrinfo
    struct sockaddr_storage myinfo;  // Informations sur la connexion de RDV
    struct sockaddr_storage from;    // Informations sur le client connecté
    socklen_t len;                   // Variable utilisée pour stocker les longueurs des structures de socket
    char buffer[MAXBUFFERLEN];       // Tampon de communication entre le client et le serveur

    // Initialisation de la socket de RDV IPv4/TCP
    descSockRDV = socket(AF_INET, SOCK_STREAM, 0);
    if (descSockRDV == -1) {
         perror("Erreur création socket RDV\n");
         exit(2);
    }

    // Publication de la socket au niveau du système
    memset(&hints, 0, sizeof(hints));
    hints.ai_flags = AI_PASSIVE;      // mode serveur, nous allons utiliser la fonction bind
    hints.ai_socktype = SOCK_STREAM;  // TCP
    hints.ai_family = AF_INET;        // seules les adresses IPv4 seront présentées par la fonction getaddrinfo

    // Récupération des informations du serveur
    ecode = getaddrinfo(SERVADDR, SERVPORT, &hints, &res);
    if (ecode) {
        fprintf(stderr,"getaddrinfo: %s\n", gai_strerror(ecode));
        exit(1);
    }

    // Publication de la socket
    ecode = bind(descSockRDV, res->ai_addr, res->ai_addrlen);
    if (ecode == -1) {
        perror("Erreur liaison de la socket de RDV");
        exit(3);
    }
    freeaddrinfo(res);

    // Récuppération du nom de la machine et du numéro de port pour affichage à l'écran
    len = sizeof(struct sockaddr_storage);
    ecode = getsockname(descSockRDV, (struct sockaddr *) &myinfo, &len);
    if (ecode == -1) {
        perror("SERVEUR: getsockname");
        exit(4);
    }
    ecode = getnameinfo((struct sockaddr*)&myinfo, sizeof(myinfo), serverAddr, MAXHOSTLEN, serverPort, MAXPORTLEN, NI_NUMERICHOST | NI_NUMERICSERV);
    if (ecode != 0) {
        fprintf(stderr, "error in getnameinfo: %s\n", gai_strerror(ecode));
        exit(4);
    }
    printf("L'adresse d'ecoute est: %s\n", serverAddr);
    printf("Le port d'ecoute est: %s\n", serverPort);

    // Definition de la taille du tampon contenant les demandes de connexion
    ecode = listen(descSockRDV, LISTENLEN);
    if (ecode == -1) {
        perror("Erreur initialisation buffer d'écoute");
        exit(5);
    }

    while (true) {
        // Attente connexion du client
        len = sizeof(struct sockaddr_storage);
        descSockCOM = accept(descSockRDV, (struct sockaddr *) &from, &len);
        if (descSockCOM == -1){
            perror("Erreur accept\n");
            continue;
        }

        // Lire la commande USER du client
        ecode = read(descSockCOM, buffer, MAXBUFFERLEN);
        if (ecode <= 0) {
            perror("Erreur lecture USER\n");
            close(descSockCOM);
            continue;
        }
        buffer[ecode] = '\0';

        // Extraire le nom d'utilisateur et l'adresse du serveur à partir de la commande USER
        char username[MAXHOSTLEN];
        char hostname[MAXHOSTLEN];
        if (sscanf(buffer, "USER %[^@]@%[^\n]", username, hostname) != 2) {
            fprintf(stderr, "Invalid USER command format\n");
            close(descSockCOM);
            continue;
        }

        // Se connecter au serveur FTP
        ecode = connect2Server(hostname, "21", &descSockServer);
        if (ecode == -1) {
            perror("Erreur connexion au serveur FTP\n");
            close(descSockCOM);
            continue;
        }

        // Envoyer la commande USER au serveur FTP
        snprintf(buffer, MAXBUFFERLEN, "USER %s\r\n", username);
        write(descSockServer, buffer, strlen(buffer));

        // Relayer les commandes et les réponses entre le client et le serveur
        bool isLoggedIn = false;
        while (true) {
            fd_set readfds;
            FD_ZERO(&readfds);
            FD_SET(descSockCOM, &readfds);
            FD_SET(descSockServer, &readfds);
            int maxfd = (descSockCOM > descSockServer) ? descSockCOM : descSockServer;

            ecode = select(maxfd + 1, &readfds, NULL, NULL, NULL);
            if (ecode == -1) {
                perror("Erreur select\n");
                break;
            }

            if (FD_ISSET(descSockCOM, &readfds)) {
                ecode = read(descSockCOM, buffer, MAXBUFFERLEN);
                if (ecode <= 0) break;

                // Sauter l'envoi de la commande USER si l'utilisateur est déjà connecté
                if (strncmp(buffer, "USER", 4) == 0 && isLoggedIn) {
                    continue;
                }

                write(descSockServer, buffer, ecode);

                // Vérifier si l'utilisateur est connecté
                if (strncmp(buffer, "PASS", 4) == 0) {
                    isLoggedIn = true;
                }
            }

            if (FD_ISSET(descSockServer, &readfds)) {
                ecode = read(descSockServer, buffer, MAXBUFFERLEN);
                if (ecode <= 0) break;
                write(descSockCOM, buffer, ecode);
            }
        }

        close(descSockCOM);
        close(descSockServer);
    }

    close(descSockRDV);
    return 0;
}