
# Projet Awalé - Manuel Utilisateur

## Description
Ce projet implémente le célèbre jeu traditionnel **Awalé**, avec un serveur et des clients pouvant se connecter pour jouer en ligne. Le système utilise des commandes Makefile pour gérer la configuration et le lancement du serveur et des clients.

---

## Commandes Makefile
Le fichier Makefile inclut plusieurs règles pour faciliter la compilation et la gestion des fichiers nécessaires :

### 1. `make`
- **Description** : Compile les fichiers du serveur et du client.
- **Résultat** : Génère deux exécutables :
  - `serveurAwale`
  - `clientAwale`

### 2. `make database`
- **Description** : Crée les dossiers et fichiers nécessaires pour les utilisateurs et initialise une base de données fictive. Chaque utilisateur a un dossier contenant :
  - Un fichier `bio` (informations de profil).
  - Un fichier `friends` (liste d'amis).
  - Un fichier `statistiques` (nombre de matchs, victoires, défaites).
- **Utilisation** : Vous exécuterez d'abord la commande `make database` puis la commande `make`. 

### 3. `make clean`
- **Description** : Supprime les fichiers et dossiers objets, les exécutables, et les fichiers générés par `make database`.

---

## Instructions pour exécuter le projet

### 1. Configuration du serveur
1. Ouvrez un terminal.
2. Lancez la commande suivante pour démarrer le serveur :
   ```bash
   ./serveurAwale 9999
   ```
   **Note** : `9999` est le port utilisé par défaut pour les communications.

### 2. Connexion des clients
1. Ouvrez un autre terminal (client).
2. Récupérez l'adresse IP de la machine où le serveur est lancé en utilisant la commande suivante :
   ```bash
   hostname -I
   ```
   Copiez l'adresse IP affichée.
3. Lancez un client en exécutant :
   ```bash
   ./clientAwale [ADRESSE_IP] 9999
   ```
   Remplacez `[ADRESSE_IP]` par l'adresse IP obtenue à l'étape précédente.

### 3. Exemple
- Serveur :
  ```
  ./serveurAwale 9999
  ```
- Client :
  ```
  ./clientAwale 192.168.1.100 9999
  ```

---

## Fonctionnalités principales

1. **Inscription et Connexion :**

   - Utilisez `/signin [username] [password]` pour créer un nouveau compte.
   - Utilisez `/login [username] [password]` pour vous connecter avec un compte existant.


2. **Liste des commandes :**

   - `/help` : Afficher la liste des commandes disponibles.
   - `/challenge [username]` : Défier un autre joueur.
   - `/accept` : Accepter un défi.
   - `/decline` : Refuser un défi.
   - `/play [case]` : Jouer un coup pendant une partie.
   - `/ff` : Abandonner la partie.
   - `/queueup` : Permet de rejoindre la file de matchmaking.
   - `/leavequeue` : Permet de quitter la file de matchmaking.
   - `/listepartie` : Affiche les parties en cours.
   - `/list` : Afficher la liste des joueurs connectés.
   - `/search [username]` : Rechercher les informations d'un joueur (bio et statistiques).
   - `/profile` : Afficher vos propres informations (profil).
   - `/addfriend [username]` : Envoyer une demande d'ami.
   - `/friends` : Voir votre liste d'amis.
   - `/friendrequest` : Voir vos demandes d'amis en attente.
   - `/faccept [username]` : Accepter une demande d'ami.
   - `/fdecline [username]` : Refuser une demande d'ami.
   - `/bio` : Modifier votre bio (biographie personnelle).
   - `/savebio` : Enregistrer votre bio.
   - `/watch [username]` : Regarder la partie d'un utilisateur.
   - `/watchgame [id]` : Observer une partie par ID.
   - `/unwatch` : Arreter de regarder la partie d'un utilisateur.
   - `/leaderboard` : Afficher le top 3 des joueurs en fonction de leur winrate. 
   - `/historique` : Voir l'historique des parties jouées.
   - `/public` : Passer en mode public pour envoyer des messages visibles par tous.
   - `/private` : Passer en mode privé (fonctionnalité à développer ou préciser).
   - `/logout` : Se déconnecter du serveur.

---


## Auteurs
Ce projet a été conçu pour illustrer la mise en œuvre d'un jeu multi-utilisateurs en réseau.

---

Bon jeu !

### CHAOUKI Youssef
### LAATAR Youssef
