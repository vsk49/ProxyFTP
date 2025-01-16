# ProxyFTP

Implementation d'un proxy pour le protocole FTP en langage C.

## Protocole FTP

Le protocole FTP (File Transfer Protocol) est un protocole standard utilisé pour transférer des fichiers entre un client et un serveur sur un réseau informatique. FTP utilise des connexions de contrôle et de données séparées entre le client et le serveur.

## Fonctionnement du Proxy

Ce proxy sert d'intermédiaire entre le client et le serveur FTP. Il gère les connexions des clients, transfère les commandes au serveur FTP et renvoie les réponses au client. Les principales fonctionnalités incluent :

- Initialisation d'une socket d'écoute pour les connexions clients.
- Acceptation des connexions clients et création d'un nouveau processus pour gérer chaque connexion.
- Transmission des identifiants de connexion et des commandes du client au serveur FTP.
- Gestion des commandes PASV et PORT pour gérer les connexions de données pour les transferts de fichiers.

## Comment lancer et tester le proxy

### Compilation et Lancement

1. Compiler le proxy en utilisant le Makefile fourni :

   ```sh
   make run

2. Connectez-vous au proxy en utilisant un client FTP (dans ce cas là, j'utilise ftp-ssl)

   ```sh
   ftp -z nossl -d <adresse ip du proxy> <port du proxy affiche>

3. Tapez votre nom sous forme de nomUtilisateur@nomHote. Dans cet exemple, je me connecte au serveur ftp.fau.de

   ```sh
   anonymous@ftp.fau.de

4. Tapez votre adresse mail comme mot de passe, comme votre nom utilsateur est "anonymous"

  ```sh
  <votre adresse mail>



Voila ! Vous est connecte au serveur FTP. Maintenant vous pouvez faire les commandes ftp comme ls, 
pwd, cd. (Pour l'instant, je n'ai pas gere les commandes PUT et GET, ca arrive plus tard)
