#define _POSIX_C_SOURCE 200809L
#include  <stdio.h>
#include  <stdlib.h>
#include  <sys/socket.h>
#include  <sys/types.h>
#include  <string.h>
#include  <unistd.h>
#include  <stdbool.h>
#include  <netdb.h>
#include <sys/wait.h>
#include "./simpleSocketAPI.h"


#define SERVADDR "127.0.0.1"        // Définition de l'adresse IP d'écoute
#define SERVPORT "0"                // Définition du port d'écoute, si 0 port choisi dynamiquement
#define LISTENLEN 1                 // Taille de la file des demandes de connexion
#define MAXBUFFERLEN 1024           // Taille du tampon pour les échanges de données
#define MAXHOSTLEN 64               // Taille d'un nom de machine
#define MAXPORTLEN 64               // Taille d'un numéro de port


int main(){
    int ecode;                       // Code retour des fonctions
    char serverAddr[MAXHOSTLEN];     // Adresse du serveur
    char serverPort[MAXPORTLEN];     // Port du server
    int descSockRDV;
    int descSockConnectServeur;                 // Descripteur de socket de rendez-vous
    int descSockCOM;
    int descDonneesClient;
    int descDonneesServeur;               // Descripteur de socket de communication
    struct addrinfo hints;           // Contrôle la fonction getaddrinfo
    struct addrinfo *res;            // Contient le résultat de la fonction getaddrinfo
    struct sockaddr_storage myinfo;  // Informations sur la connexion de RDV
    struct sockaddr_storage from;    // Informations sur le client connecté
    socklen_t len;                   // Variable utilisée pour stocker les 
				                     // longueurs des structures de socket
    char buffer[MAXBUFFERLEN];       // Tampon de communication entre le client et le serveur
    
    // Initialisation de la socket de RDV IPv4/TCP
    descSockRDV = socket(AF_INET, SOCK_STREAM, 0);
    if (descSockRDV == -1) {
         perror("Erreur création socket RDV\n");
         exit(2);
    }
    // Publication de la socket au niveau du système
    // Assignation d'une adresse IP et un numéro de port
    // Mise à zéro de hints
    memset(&hints, 0, sizeof(hints));
    // Initialisation de hints
    hints.ai_flags = AI_PASSIVE;      // mode serveur, nous allons utiliser la fonction bind
    hints.ai_socktype = SOCK_STREAM;  // TCP
    hints.ai_family = AF_INET;        // seules les adresses IPv4 seront présentées par 
				                      // la fonction getaddrinfo

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
    // Nous n'avons plus besoin de cette liste chainée addrinfo
    freeaddrinfo(res);

    // Récuppération du nom de la machine et du numéro de port pour affichage à l'écran
    len=sizeof(struct sockaddr_storage);
    ecode=getsockname(descSockRDV, (struct sockaddr *) &myinfo, &len);
    if (ecode == -1) {
        perror("SERVEUR: getsockname");
        exit(4);
    }
    ecode = getnameinfo((struct sockaddr*)&myinfo, sizeof(myinfo), serverAddr,MAXHOSTLEN, 
                         serverPort, MAXPORTLEN, NI_NUMERICHOST | NI_NUMERICSERV);
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
        len = sizeof(struct sockaddr_storage);
        // Attente connexion du client
        // Lorsque demande de connexion, creation d'une socket de communication avec le client
        descSockCOM = accept(descSockRDV, (struct sockaddr *) &from, &len);
        if (descSockCOM == -1){
            perror("Erreur accept\n");
            exit(6);
        }

        pid_t pid = fork();
        if (pid == -1) {
            perror("Erreur fork\n");
            continue;
        } else if (pid == 0) {
            // Echange de données avec le client connecté
            close(descSockRDV);

            /*****
             * Debut de communication avec 220 BIENVENUE AU CANAL DE COMMUNICATION FTP
             * **/
            strcpy(buffer, "220 BIENVENUE AU CANAL DE COMMUNICATION FTP\n");
            write(descSockCOM, buffer, strlen(buffer));

            // Lecture des informations de connexion username@hostname
            memset(buffer, 0, MAXBUFFERLEN);
            int bytesRead = read(descSockCOM, buffer, MAXBUFFERLEN - 1);
            if (bytesRead < 0) {
                perror("Erreur lecture des informations de connexion");
                close(descSockCOM);
                close(descSockRDV);
                exit(7);
            }
            printf("%s", buffer);

            // Séparation du nom d'utilisateur et du nom de l'hôte
            char username[MAXBUFFERLEN];
            char hostname[MAXBUFFERLEN];
            sscanf(buffer, "%[^@]@%[^\n]", username, hostname);
            printf("Username: %s\n", username);
            printf("Hostname: %s\n", hostname);
            // enlever le \r\n
            hostname[strlen(hostname) - 1] = '\0';
            printf("Username: '%s'\n", hostname);

            if (connect2Server(hostname, "21", &descSockConnectServeur) < 0) {
                perror("Erreur connexion au serveur FTP");
                close(descSockCOM);
                close(descSockRDV);
                exit(8);
            }
            read(descSockConnectServeur, buffer, MAXBUFFERLEN - 1);
            printf("%s", buffer);
            // envoyer les info de conexion au serveur
            printf("USER %s\n", username);
            if (strlen(username) + 2 >= MAXBUFFERLEN) { // Adjusted check to include null terminator
                fprintf(stderr, "Username too long\n");
                exit(1);
            }
            snprintf(buffer, MAXBUFFERLEN, "%s\r\n", username);
            printf("'%s'\n", username);
            write(descSockConnectServeur, buffer, strlen(buffer));
            printf("ok\n");
            memset(buffer, 0, MAXBUFFERLEN);
            read(descSockConnectServeur, buffer, MAXBUFFERLEN - 1);
            printf("%s", buffer);
            write(descSockCOM, buffer, strlen(buffer));

            // recuperer le mot de passe
            memset(buffer, 0, MAXBUFFERLEN);
            bytesRead = read(descSockCOM, buffer, MAXBUFFERLEN - 1);
            if (bytesRead < 0) {
                perror("Erreur lecture du mot de passe");
                close(descSockCOM);
                close(descSockRDV);
                close(descSockConnectServeur);
                exit(9);
            }
            printf("%s", buffer);

            // envoyer le mot de passe au serveur
            char motDePasse[MAXBUFFERLEN];
            sscanf(buffer, "%[^\n]", motDePasse);
            printf("Mot de passe: %s\n", motDePasse);
            // enlever le \r\n
            motDePasse[strlen(motDePasse) - 1] = '\0';
            printf("Mot de passe: %s\n", buffer);
            write(descSockConnectServeur, buffer, strlen(buffer));
            memset(buffer, 0, MAXBUFFERLEN);
            read(descSockConnectServeur, buffer, MAXBUFFERLEN - 1);
            printf("%s", buffer);
            write(descSockCOM, buffer, strlen(buffer));

            // connexion reussie, maintenant on fait une boucle pour lire les commandes
            while (strncmp(buffer, "QUIT", 4) != 0) {
                // recuperer la commande
                memset(buffer, 0, MAXBUFFERLEN);
                bytesRead = read(descSockCOM, buffer, MAXBUFFERLEN - 1);
                if (bytesRead < 0) {
                    perror("Erreur lecture de la commande");
                    close(descSockCOM);
                    close(descSockRDV);
                    close(descSockConnectServeur);
                    exit(10);
                }
                printf("%s", buffer);

                // verifier si la commande est PORT
                if (strncmp(buffer, "PORT", 4) == 0) {
                    // separaration de la reponse PASV pour recuperer l'adresse IP et le port
                    int ip1, ip2, ip3, ip4, port1, port2;
                    sscanf(buffer, "PORT %d,%d,%d,%d,%d,%d", &ip1, &ip2, &ip3, &ip4, &port1, &port2);
                    char dataIPClient[16];
                    snprintf(dataIPClient, sizeof(dataIPClient), "%d.%d.%d.%d", ip1, ip2, ip3, ip4);
                    printf("IP: %s\n", dataIPClient);
                    char dataPortClient[16];
                    snprintf(dataPortClient, sizeof(dataPortClient), "%d", port1 * 256 + port2);
                    printf("Port: %s\n", dataPortClient);
                    
                    // envoyer la commande PASV au serveur
                    char pasvCommand[] = "PASV\r\n";
                    write(descSockConnectServeur, pasvCommand, strlen(pasvCommand));
                    memset(buffer, 0, MAXBUFFERLEN);
                    read(descSockConnectServeur, buffer, MAXBUFFERLEN - 1);
                    printf("%s", buffer);

                    // separaration de la reponse PASV pour recuperer l'adresse IP et le port
                    sscanf(buffer, "227 Entering Passive Mode (%d,%d,%d,%d,%d,%d)", &ip1, &ip2, &ip3, &ip4, &port1, &port2);
                    char dataIPServeur[16];
                    snprintf(dataIPServeur, sizeof(dataIPServeur), "%d.%d.%d.%d", ip1, ip2, ip3, ip4);
                    printf("IP: %s\n", dataIPServeur);
                    char dataPortServeur[16];
                    snprintf(dataPortServeur, sizeof(dataPortServeur), "%d", port1 * 256 + port2);
                    printf("Port: %s\n", dataPortServeur);

                    if (connect2Server(dataIPClient, dataPortClient, &descDonneesClient) < 0) {
                        perror("Erreur connexion au serveur FTP");
                        close(descSockCOM);
                        close(descSockRDV);
                        exit(8);
                    }
                    if (connect2Server(dataIPServeur, dataPortServeur, &descDonneesServeur) < 0) {
                        perror("Erreur connexion au serveur FTP");
                        close(descSockCOM);
                        close(descSockRDV);
                        exit(8);
                    }
                    // envoyer la commande PORT au serveur
                    write(descSockCOM, "200 'PORT' OK.\r\n", 17);
                } else if (strncmp(buffer, "LIST", 4) == 0) {
                    // envoyer la commande LIST au serveur
                    write(descSockConnectServeur, buffer, strlen(buffer));
                    memset(buffer, 0, MAXBUFFERLEN);
                    read(descSockConnectServeur, buffer, MAXBUFFERLEN - 1);
                    printf("%s", buffer);
                    write(descSockCOM, buffer, strlen(buffer));

                    read(descDonneesServeur, buffer, MAXBUFFERLEN - 1);
                    printf("%s", buffer);
                    write(descDonneesClient, buffer, strlen(buffer));
                    close(descDonneesClient);
                    close(descDonneesServeur);

                    memset(buffer, 0, MAXBUFFERLEN);
                    read(descSockConnectServeur, buffer, MAXBUFFERLEN - 1);
                    printf("%s", buffer);
                    write(descSockCOM, buffer, strlen(buffer));
                } else {
                    // gestion des autres commandes
                    write(descSockConnectServeur, buffer, strlen(buffer));
                    memset(buffer, 0, MAXBUFFERLEN);
                    read(descSockConnectServeur, buffer, MAXBUFFERLEN - 1);
                    printf("%s", buffer);
                    write(descSockCOM, buffer, strlen(buffer));
                }
            }

            // fermeture de la connexion
            close(descSockCOM);
            exit(0);
        } else {
            close(descSockCOM);
            // attendre le fils
            while (waitpid(-1, NULL, WNOHANG) > 0);
        }
    }
    
}