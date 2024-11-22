#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "server2.h"
#include "client2.h"
#include "awale.h"

#define MAX_CLIENTS 100
#define MAX_SALONS 50
#define MAX_QUEUE 100
#define MAX_SPECTATEURS 10
// Déclaration anticipée de Utilisateur
typedef struct Utilisateur Utilisateur;


typedef struct {
    char username[50];
    float win_rate;
} JoueurClassement;



static void init(void)
{
#ifdef WIN32
    WSADATA wsa;
    int err = WSAStartup(MAKEWORD(2, 2), &wsa);
    if (err < 0)
    {
        puts("WSAStartup failed !");
        exit(EXIT_FAILURE);
    }
#endif
}

static void end(void)
{
#ifdef WIN32
    WSACleanup();
#endif
}


int queue[MAX_QUEUE];
int queueSize = 0;
// Salon salons[MAX_SALONS];
Utilisateur utilisateurs[MAX_CLIENTS];
int nbUtilisateursConnectes = 0;
int nbSalons = 0;
static int nextUserId = 1; 
static int nextSalonId = 1;
Salon salons[MAX_SALONS];


void mettre_a_jour_statistiques(const char *username, int match_increment, int win_increment, int loss_increment);
void envoyer_plateau_spectateur(int idSpectateur, int idSalon);
void sauvegarderPartie(Partie *partie, const char *username1, const char *username2, const char *resultat);
void lire_statistiques(const char *username, int *matches, int *wins, int *losses);




void initialiserUtilisateurs(void)
{
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        utilisateurs[i].sock = INVALID_SOCKET;
        utilisateurs[i].estEnJeu = 0;
        utilisateurs[i].partieEnCours = NULL;
        utilisateurs[i].joueur = NULL;
        utilisateurs[i].isConnected = 0;
        utilisateurs[i].estPublic = 1;
    }
}

void initialiserSalons(void)
{
    for (int i = 0; i < MAX_SALONS; i++)
    {
        salons[i].joueur1 = NULL;
        salons[i].joueur2 = NULL;
        salons[i].statut = 0;
        salons[i].nbSpectateurs = 0;
        salons[i].id = nextSalonId++;
    }
}

void envoyer_plateau_spectateur(int idSpectateur, int idSalon)
{
    char buffer[BUF_SIZE];
    Utilisateur *spectateur = trouverUtilisateurParId(idSpectateur);
    Salon *salon = trouverSalonParId(idSalon);

    snprintf(buffer, BUF_SIZE, "  ---------------------------------\n");
    snprintf(buffer + strlen(buffer), BUF_SIZE - strlen(buffer),
             "  Joueur 1 : %s, Score : %d\n",
             salon->joueur1->username, salon->partie.joueur1.score);

    // Ligne supérieure (cases 0 à 5 pour Joueur 1)
    strncat(buffer, "   ", BUF_SIZE - strlen(buffer) - 1);
    for (int i = 0; i < 6; i++)
    {
        char temp[10];
        snprintf(temp, 10, "%2d ", salon->partie.plateau.cases[i].nbGraines);
        strncat(buffer, temp, BUF_SIZE - strlen(buffer) - 1);
    }
    strncat(buffer, "\n   ", BUF_SIZE - strlen(buffer) - 1);

    // Ligne inférieure (cases 11 à 6 pour Joueur 2)
    for (int i = 11; i >= 6; i--)
    {
        char temp[10];
        snprintf(temp, 10, "%2d ", salon->partie.plateau.cases[i].nbGraines);
        strncat(buffer, temp, BUF_SIZE - strlen(buffer) - 1);
    }
    strncat(buffer, "\n", BUF_SIZE - strlen(buffer) - 1);

    snprintf(buffer + strlen(buffer), BUF_SIZE - strlen(buffer),
             "  Joueur 2 : %s, Score : %d\n",
             salon->joueur2->username, salon->partie.joueur2.score);

    strncat(buffer, "  ---------------------------------\n", BUF_SIZE - strlen(buffer) - 1);

    // Envoyer à qui c'est le tour de jouer
    
    if (salon->tourActuel == 0)
    {
        write_client(spectateur->sock,"C'est au tour du joueur 1.\n");
    }
    else
    {
        write_client(spectateur->sock,"C'est au tour du joueur 2.\n");
    }

    write_client(spectateur->sock, buffer);
}

void envoyer_plateau_aux_users(int idJoueur1, int idJoueur2, Partie *partie, int tour)
{
    char buffer[BUF_SIZE];
    Utilisateur *joueur1 = trouverUtilisateurParId(idJoueur1);
    Utilisateur *joueur2 = trouverUtilisateurParId(idJoueur2);

    // Préparation de l'affichage du plateau
    snprintf(buffer, BUF_SIZE, "  --------------------------\n");

    // Information sur le Joueur 1
    char temp[128];
    snprintf(temp, sizeof(temp), "  Joueur 1 : %s, Score : %d\n", joueur1->username, partie->joueur1.score);
    strncat(buffer, temp, BUF_SIZE - strlen(buffer) - 1);

    // Ligne supérieure (cases 0 à 5 pour Joueur 1)
    strncat(buffer, "   ", BUF_SIZE - strlen(buffer) - 1);
    for (int i = 0; i < 6; i++)
    {
        snprintf(temp, sizeof(temp), "%2d ", partie->plateau.cases[i].nbGraines);
        strncat(buffer, temp, BUF_SIZE - strlen(buffer) - 1);
    }
    strncat(buffer, "\n   ", BUF_SIZE - strlen(buffer) - 1);

    // Ligne inférieure (cases 11 à 6 pour Joueur 2)
    for (int i = 11; i >= 6; i--)
    {
        snprintf(temp, sizeof(temp), "%2d ", partie->plateau.cases[i].nbGraines);
        strncat(buffer, temp, BUF_SIZE - strlen(buffer) - 1);
    }
    strncat(buffer, "\n", BUF_SIZE - strlen(buffer) - 1);

    // Information sur le Joueur 2
    snprintf(temp, sizeof(temp), "  Joueur 2 : %s, Score : %d\n", joueur2->username, partie->joueur2.score);
    strncat(buffer, temp, BUF_SIZE - strlen(buffer) - 1);

    strncat(buffer, "  --------------------------\n", BUF_SIZE - strlen(buffer) - 1);

    // Envoi du plateau aux deux joueurs
    write_client(joueur1->sock, buffer);
    write_client(joueur2->sock, buffer);

    // Envoyer à qui c'est le tour de jouer
    if (tour == 0)
    {
        write_client(joueur1->sock, "C'est à votre tour de jouer.\n");
        write_client(joueur2->sock, "C'est le tour de votre adversaire.\n");
    }
    else
    {
        write_client(joueur1->sock, "C'est le tour de votre adversaire.\n");
        write_client(joueur2->sock, "C'est à votre tour de jouer.\n");
    }
}


void creerSalon(int idJoueur1, int idJoueur2)
{
    if (nbSalons >= MAX_SALONS)
    {
        printf("Nombre maximal de salons atteint.\n");

        return;
    }

    Salon *salon = &salons[nbSalons++];
    Utilisateur *joueur1 = trouverUtilisateurParId(idJoueur1);
    Utilisateur *joueur2 = trouverUtilisateurParId(idJoueur2);
    // tirer au sort qui sera le joueur 1 et le joueur 2
    if (rand() % 2 == 0)
    {
        // print l'état du touractuel
        printf("Tour actuel quand je crée la partie : %d\n", salon->tourActuel);
        salon->joueur2 = joueur1;
        salon->idJoueur2 = idJoueur1;
        strcpy(salon->partie.joueur2.pseudo, joueur1->username);
        salon->joueur1 = joueur2;
        salon->idJoueur1 = idJoueur2;
        strcpy(salon->partie.joueur1.pseudo, joueur2->username);
        salon->tourActuel = 1;
    }
    else
    {
        salon->joueur1 = joueur1;
        salon->idJoueur1 = idJoueur1;
        strcpy(salon->partie.joueur1.pseudo, joueur1->username);
        salon->joueur2 = joueur2;
        salon->idJoueur2 = idJoueur2;
        strcpy(salon->partie.joueur2.pseudo, joueur2->username);
        salon->tourActuel = 0;
        
    }
    salon->statut = 1;
    int tour = salon->tourActuel;
    initialiserPartie(&salon->partie);
    joueur1->estEnJeu = 1;
    joueur2->estEnJeu = 1;
    joueur1->partieEnCours = salon;
    joueur2->partieEnCours = salon;
    joueur1->idPartieEnCours = salon->id;
    joueur2->idPartieEnCours = salon->id;

    envoyer_plateau_aux_users(idJoueur1, idJoueur2, &salon->partie, tour);
}

Utilisateur *trouverUtilisateurParUsername(const char *username)
{
    for (int i = 0; i < nbUtilisateursConnectes; i++)
    {
        if (strcmp(utilisateurs[i].username, username) == 0)
        {
            return &utilisateurs[i];
        }
    }
    return NULL; // Aucun utilisateur trouvé
}

Utilisateur *trouverUtilisateurParId(int idjoueur)
{
    for (int i = 0; i < nbUtilisateursConnectes; i++)
    {
        if (utilisateurs[i].id == idjoueur)
        {
            return &utilisateurs[i];
        }
    }
    return NULL; // Aucun utilisateur trouvé
}

Salon *trouverSalonParId(int idSalon)
{
    for (int i = 0; i < nbSalons; i++)
    {
        if (salons[i].id == idSalon)
        {
            return &salons[i];
        }
    }
    return NULL; // Aucun salon trouvé
}

void terminerPartie(int idSalon)
{
    Salon *salon = trouverSalonParId(idSalon);
    if (!salon) {
        printf("Erreur : Salon introuvable avec l'ID %d\n", idSalon);
        return;
    }

    salon->statut = 2;
    salon->joueur1->estEnJeu = 0;
    salon->joueur2->estEnJeu = 0;
    salon->joueur1->partieEnCours = NULL;
    salon->joueur2->partieEnCours = NULL;
    salon->joueur1->challenger = NULL;
    salon->joueur2->challenger = NULL;

    char resultat[256];

    if (salon->partie.joueur1.score > salon->partie.joueur2.score) {
        snprintf(resultat, sizeof(resultat), "Victoire de %s", salon->joueur1->username);
        write_client(salon->joueur1->sock, "Vous avez gagné !\n");
        write_client(salon->joueur2->sock, "Vous avez perdu.\n");
        mettre_a_jour_statistiques(salon->joueur1->username, 1, 1, 0);
        mettre_a_jour_statistiques(salon->joueur2->username, 1, 0, 1);
    } else if (salon->partie.joueur1.score < salon->partie.joueur2.score) {
        snprintf(resultat, sizeof(resultat), "Victoire de %s", salon->joueur2->username);
        write_client(salon->joueur1->sock, "Vous avez perdu.\n");
        write_client(salon->joueur2->sock, "Vous avez gagné !\n");
        mettre_a_jour_statistiques(salon->joueur1->username, 1, 0, 1);
        mettre_a_jour_statistiques(salon->joueur2->username, 1, 1, 0);
    } else {
        snprintf(resultat, sizeof(resultat), "Match nul");
        write_client(salon->joueur1->sock, "Match nul !\n");
        write_client(salon->joueur2->sock, "Match nul !\n");
        mettre_a_jour_statistiques(salon->joueur1->username, 1, 0, 0);
        mettre_a_jour_statistiques(salon->joueur2->username, 1, 0, 0);
    }

    // Appeler sauvegarderPartie
    sauvegarderPartie(&salon->partie, salon->joueur1->username, salon->joueur2->username, resultat);
}



void sauvegarderPartie(Partie *partie, const char *username1, const char *username2, const char *resultat) {
    // Dossiers des deux joueurs
    char filepath1[256], filepath2[256];
    snprintf(filepath1, sizeof(filepath1), "Serveur/players/%s/%s_vs_%s.txt", username1, username1, username2);
    snprintf(filepath2, sizeof(filepath2), "Serveur/players/%s/%s_vs_%s.txt", username2, username1, username2);

    FILE *file1 = fopen(filepath1, "w");
    FILE *file2 = fopen(filepath2, "w");

    if (!file1 || !file2) {
        perror("Erreur lors de l'ouverture des fichiers pour sauvegarde");
        if (file1) fclose(file1);
        if (file2) fclose(file2);
        return;
    }

    // Informations de base
    fprintf(file1, "Partie entre %s et %s\n", username1, username2);
    fprintf(file2, "Partie entre %s et %s\n", username1, username2);

    fprintf(file1, "Coups joués :\n");
    fprintf(file2, "Coups joués :\n");

    // Sauvegarde des coups joués
    for (int i = 0; i < partie->indexCoups; i++) {
        int joueur = (i % 2 == 0) ? 1 : 2; // Alternance entre joueur 1 et joueur 2
        int caseJouee = partie->coups[i] + 1; // Cases sont indexées de 0, +1 pour correspondre au jeu
        fprintf(file1, "Joueur %d joue case %d\n", joueur, caseJouee);
        fprintf(file2, "Joueur %d joue case %d\n", joueur, caseJouee);
    }

    // Ajout du résultat
    fprintf(file1, "Résultat : %s\n", resultat);
    fprintf(file2, "Résultat : %s\n", resultat);

    fclose(file1);
    fclose(file2);
}



void envoyerAide(int idUtilisateur) {
    Utilisateur *utilisateur = trouverUtilisateurParId(idUtilisateur);
    write_client(utilisateur->sock, "Commandes disponibles :\n");
    write_client(utilisateur->sock, "/login [username] [password] - Se connecter.\n");
    write_client(utilisateur->sock, "/signin [username] [password] - Créer un compte.\n");
    write_client(utilisateur->sock, "/logout - Se déconnecter.\n");
    write_client(utilisateur->sock, "/challenge [username] - Défier un joueur.\n");
    write_client(utilisateur->sock, "/accept - Accepter un défi.\n");
    write_client(utilisateur->sock, "/decline - Refuser un défi.\n");
    write_client(utilisateur->sock, "/play [case] - Jouer sur une case.\n");
    write_client(utilisateur->sock, "/ff - Abandonner la partie.\n");
    write_client(utilisateur->sock, "/addfriend [username] - Ajouter un ami.\n");
    write_client(utilisateur->sock, "/faccept [username] - Accepter une demande d'ami.\n");
    write_client(utilisateur->sock, "/fdecline [username] - Refuser une demande d'ami.\n");
    write_client(utilisateur->sock, "/friends - Voir la liste des amis.\n");
    write_client(utilisateur->sock, "/search [username] - Rechercher un utilisateur.\n");
    write_client(utilisateur->sock, "/profile - Voir votre profil.\n");
    write_client(utilisateur->sock, "/public - Passer en mode public.\n");
    write_client(utilisateur->sock, "/private - Passer en mode privé.\n");
    write_client(utilisateur->sock, "/help - Voir cette liste de commandes.\n");
    write_client(utilisateur->sock, "/list - Voir la liste des joueurs en ligne.\n");
    write_client(utilisateur->sock, "/friendrequest - Voir vos demandes d'amis en attente.\n");
    write_client(utilisateur->sock, "/bio - Modifier votre bio.\n");
    write_client(utilisateur->sock, "/msg [username] [message] - Envoyer un message privé.\n");
    write_client(utilisateur->sock, "/leaderboard : Afficher le top 3 des joueurs en fonction de leur winrate.\n");
    
}


void envoyerMessagePublic(int idExpediteur, const char *message)
{
    char buffer[BUF_SIZE];
    Utilisateur *expediteur = trouverUtilisateurParId(idExpediteur);
    snprintf(buffer, BUF_SIZE, "%s : %s\n", expediteur->username, message);
    for (int i = 0; i < nbUtilisateursConnectes; i++)
    {
        write_client(utilisateurs[i].sock, buffer);
    }
}

void playCoup(int idSalon, int idJoueur, int caseJouee)
{
    Salon *salon = trouverSalonParId(idSalon);
    if (salon == NULL)
    {
        printf("Salon is NULL.\n");
        return;
    }

    Partie *partie = &salon->partie;
    if (partie == NULL)
    {
        printf("Partie is NULL.\n");
        return;
    }

    Utilisateur *joueur = trouverUtilisateurParId(idJoueur);
    if (joueur == NULL)
    {
        printf("Joueur is NULL.\n");
        return;
    }

    int joueurIndex = (idJoueur == salon->idJoueur1) ? 0 : 1;

    if (salon->tourActuel != joueurIndex)
    {
        printf("Tour actuel : %d.\n", salon->tourActuel); 
        printf("Joueur index : %d.\n", joueurIndex);
        write_client(joueur->sock, "Ce n'est pas votre tour.\n");
        return;
    }

    else if (jouerCoup(partie, caseJouee, joueurIndex + 1) == 0)
    {
        if (partie->indexCoups < 500) {  // Évitez de dépasser la capacité du tableau
            partie->coups[partie->indexCoups] = caseJouee;
            partie->indexCoups++;
        }
        salon->tourActuel = (salon->tourActuel + 1) % 2;
        int tour = salon->tourActuel;

        // Envoyer le plateau aux deux joueurs
        envoyer_plateau_aux_users(salon->idJoueur1, salon->idJoueur2, partie, tour);

        // Envoyer le plateau à tous les spectateurs
        for (int i = 0; i < salon->nbSpectateurs; i++)
        {
            int idSpectateur = salon->spectateurs[i];
            envoyer_plateau_spectateur(idSpectateur, idSalon);
        }
    }
    else
    {
        write_client(joueur->sock, "Coup illégal, essayez encore.\n");
    }

    if (verifierFinPartie(partie))
    {
        terminerPartie(idSalon);
    }
}

int est_deja_connecte(const char *username)
{
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (utilisateurs[i].isConnected && strcmp(utilisateurs[i].username, username) == 0)
        {
            return 1; // L'utilisateur est déjà connecté
        }
    }
    return 0; // L'utilisateur n'est pas connecté
}

int verifier_identifiants(const char *username, const char *password)
{
    FILE *file = fopen("Serveur/utilisateurs.txt", "r");
    if (file == NULL)
    {
        perror("Erreur lors de l'ouverture du fichier utilisateurs.txt");
        return 0;
    }

    char line[150];
    Utilisateur user;

    // Lire chaque ligne du fichier
    while (fgets(line, sizeof(line), file))
    {
        sscanf(line, "%d,%49[^,],%49s", &user.id, user.username, user.password);

        if (strcmp(user.username, username) == 0 && strcmp(user.password, password) == 0)
        {
            fclose(file);
            return 1; // Authentification réussie
        }
    }

    fclose(file);
    return 0; // Authentification échouée
}

void traiter_public(int idUtilisateur)
{
    Utilisateur *utilisateur = trouverUtilisateurParId(idUtilisateur);
    utilisateur->estPublic = 1;
    write_client(utilisateur->sock, "Votre statut est maintenant public. Les autres joueurs peuvent observer vos parties.\n");
}

void traiter_prive(int idUtilisateur)
{
    Utilisateur* utilisateur = trouverUtilisateurParId(idUtilisateur);
    utilisateur->estPublic = 0;
    write_client(utilisateur->sock, "Votre statut est maintenant privé. Seuls vos amis peuvent observer vos parties.\n");
}


void traiter_login(int idUtilisateur, const char *username, const char *password)
{
    Utilisateur *utilisateur = trouverUtilisateurParId(idUtilisateur);
    if (est_deja_connecte(username))
    {
        write_client(utilisateur->sock, "Erreur : Cet utilisateur est déjà connecté ailleurs.\n");
        return;
    }
    if (verifier_identifiants(username, password))
    {
        utilisateur->isConnected = 1; // Marque l'utilisateur comme connecté
        strncpy(utilisateur->username, username, sizeof(utilisateur->username) - 1);
        write_client(utilisateur->sock, "Connexion réussie !\n");
        char buffer[BUF_SIZE] = {0};
        snprintf(buffer, BUF_SIZE, "Bienvenue %s ! Utilisez /challenge <username> pour lancer un défi \nou /queueup pour rejoindre la file de matchmaking.\n", utilisateur->username);
        write_client(utilisateur->sock, buffer);
    }
    else
    {
        write_client(utilisateur->sock, "Identifiants incorrects.\n");
    }
}

int verifier_username(const char *username)
{
    FILE *file = fopen("Serveur/utilisateurs.txt", "r");
    if (file == NULL)
    {
        perror("Erreur lors de l'ouverture du fichier utilisateurs.txt");
        return 0;
    }

    char line[150];
    Utilisateur user;

    // Lire chaque ligne du fichier
    while (fgets(line, sizeof(line), file))
    {
        sscanf(line, "%d,%49[^,],%49s", &user.id, user.username, user.password);

        if (strcmp(user.username, username) == 0)
        {
            fclose(file);
            return 1; // username dejà présent
        }
    }

    fclose(file);
    return 0; // Authentification échouée
}

int ajouter_utilisateur(const char *username, const char *password)
{
    if (verifier_username(username))
    {
        return 0; // L'utilisateur existe déjà
    }

    FILE *file = fopen("Serveur/utilisateurs.txt", "a");
    if (!file)
    {
        perror("Erreur lors de l'ouverture du fichier utilisateurs.txt");
        return -1;
    }

    int new_id = 0;
    FILE *file_read = fopen("Serveur/utilisateurs.txt", "r");
    if (file_read)
    {
        char line[150];
        Utilisateur user;
        while (fgets(line, sizeof(line), file_read))
        {
            sscanf(line, "%d,%49[^,],%49s", &user.id, user.username, user.password);
            if (user.id > new_id)
            {
                new_id = user.id;
            }
        }
        fclose(file_read);
    }

    fprintf(file, "%d,%s,%s\n", new_id + 1, username, password);
    fclose(file);

    // Création du répertoire principal 'players'
    char base_dir[] = "Serveur/players";
    mkdir(base_dir, 0777);
    // Création du sous-dossier de l'utilisateur
    char user_dir[150];
    snprintf(user_dir, sizeof(user_dir), "%s/%s", base_dir, username);
    if (mkdir(user_dir, 0777) != 0)
    {
        perror("Erreur lors de la création du dossier utilisateur");
        return -1;
    }
    char bio_file[150], friends_file[150];
    snprintf(bio_file, sizeof(bio_file), "%s/bio", user_dir);
    snprintf(friends_file, sizeof(friends_file), "%s/friends", user_dir);

    FILE *bio = fopen(bio_file, "w");
    if (bio)
        fclose(bio);

    FILE *friends = fopen(friends_file, "w");
    if (friends)
        fclose(friends);

    char stats_file[150];
    snprintf(stats_file, sizeof(stats_file), "%s/statistics", user_dir);

    FILE *stats = fopen(stats_file, "w");
    if (stats)
    {
        // Initialise le fichier avec 0 matchs, 0 victoires, 0 défaites
        fprintf(stats, "matches:0\nwins:0\nlosses:0\n");
        fclose(stats);
    }
    else
    {
        perror("Erreur lors de la création du fichier statistiques");
    }


    return 1; // Inscription réussie
}

void traiter_listeparties(int idUtilisateur)
{
    Utilisateur *utilisateur = trouverUtilisateurParId(idUtilisateur);
    char buffer[BUF_SIZE];
    snprintf(buffer, BUF_SIZE, "Liste des parties en cours :\n");
    int partiesEnCours = 0;
    for (int i = 0; i < nbSalons; i++)
    {
        if (salons[i].statut == 1)
        {
            snprintf(buffer + strlen(buffer), BUF_SIZE - strlen(buffer),
                     "Salon %d : %s vs %s\n", salons[i].id, salons[i].joueur1->username, salons[i].joueur2->username);
            partiesEnCours = 1;
        }
    }
    if (!partiesEnCours)
    {
        snprintf(buffer + strlen(buffer), BUF_SIZE - strlen(buffer), "Aucune partie en cours.\n");
    }
    write_client(utilisateur->sock, buffer);
}

void traiter_signin(int idUtilisateur, const char *username, const char *password)
{
    Utilisateur *utilisateur = trouverUtilisateurParId(idUtilisateur);
    int result = ajouter_utilisateur(username, password);
    if (result == 1)
    {
        char user_dir[150];
        snprintf(user_dir, sizeof(user_dir), "players/%s", username);
        mkdir(user_dir, 0777);
        char bio_file[150];
        snprintf(bio_file, sizeof(bio_file), "%s/bio", user_dir);
        FILE *bio = fopen(bio_file, "w");
        if (bio != NULL)
        {
            fclose(bio);
        }

        char friends_file[150];
        snprintf(friends_file, sizeof(friends_file), "%s/friends", user_dir);
        FILE *friends = fopen(friends_file, "w");
        if (friends != NULL)
        {
            fclose(friends);
        }

        write_client(utilisateur->sock, "Inscription réussie ! Connectez-vous avec /login.\n");
        printf("Nouvel utilisateur inscrit et dossier créé : '%s'\n", username);
    }
    else if (result == 0)
    {
        write_client(utilisateur->sock, "Nom d'utilisateur déjà pris.\n");
    }
    else
    {
        write_client(utilisateur->sock, "Erreur lors de l'inscription.\n");
    }
}

void traiter_logout(int idUtilisateur)
{
    Utilisateur *utilisateur = trouverUtilisateurParId(idUtilisateur);
    printf("Utilisateur %s s'est déconnecté.\n", utilisateur->username);
    // Si l'utilisateur est en jeu, le faire déclarer forfait
    if (utilisateur->estEnJeu)
    {
        if (utilisateur->partieEnCours != NULL)
        {
            traiter_ff(idUtilisateur);
        }
    }

    // Réinitialiser l'état de connexion
    utilisateur->isConnected = 0;
    memset(utilisateur->username, 0, sizeof(utilisateur->username));
    memset(utilisateur->pseudo, 0, sizeof(utilisateur->pseudo));

    // Envoyer un message d'au revoir au client
    write_client(utilisateur->sock, "Vous avez été déconnecté. Au revoir !\n");

    // Fermer la connexion proprement
    closesocket(utilisateur->sock);
    utilisateur->sock = INVALID_SOCKET;
}

void afficherClassementTop3(Utilisateur *utilisateur) {
    JoueurClassement joueurs[MAX_CLIENTS];
    int nbJoueurs = 0;

    // Chemin du fichier des utilisateurs
    char user_file[] = "Serveur/utilisateurs.txt";

    FILE *file = fopen(user_file, "r");
    if (!file) {
        perror("Erreur lors de l'ouverture du fichier utilisateurs.txt");
        write_client(utilisateur->sock, "Erreur : Impossible d'accéder aux informations des utilisateurs.\n");
        return;
    }

    char line[256];
    while (fgets(line, sizeof(line), file)) {
        char username[50];
        if (sscanf(line, "%*d,%49[^,],%*s", username) == 1) {
            int matches = 0, wins = 0, losses = 0;
            lire_statistiques(username, &matches, &wins, &losses);

            if (matches > 0) {
                float win_rate = (float)wins / matches;
                strncpy(joueurs[nbJoueurs].username, username, sizeof(joueurs[nbJoueurs].username) - 1);
                joueurs[nbJoueurs].win_rate = win_rate;
                nbJoueurs++;
            }
        }
    }
    fclose(file);

    // Trier les joueurs par win_rate décroissant
    for (int i = 0; i < nbJoueurs - 1; i++) {
        for (int j = i + 1; j < nbJoueurs; j++) {
            if (joueurs[j].win_rate > joueurs[i].win_rate) {
                JoueurClassement temp = joueurs[i];
                joueurs[i] = joueurs[j];
                joueurs[j] = temp;
            }
        }
    }

    // Envoyer le classement au client
    char buffer[BUF_SIZE];
    snprintf(buffer, BUF_SIZE, "Classement global des meilleurs joueurs (Top 3) :\n");
    write_client(utilisateur->sock, buffer);

    for (int i = 0; i < nbJoueurs && i < 3; i++) {
        snprintf(buffer, BUF_SIZE, "%d. %s - Ratio de victoires : %.2f\n", i + 1, joueurs[i].username, joueurs[i].win_rate);
        write_client(utilisateur->sock, buffer);
    }

    if (nbJoueurs < 3) {
        snprintf(buffer, BUF_SIZE, "Il n'y a pas assez de joueurs pour afficher un top 3 complet.\n");
        write_client(utilisateur->sock, buffer);
    }
}





int ajouter_ligne_fichier(const char *username, const char *filename, const char *line)
{
    char filepath[150];
    snprintf(filepath, sizeof(filepath), "Serveur/players/%s/%s", username, filename);

    FILE *file = fopen(filepath, "a");
    if (!file)
    {
        perror("Erreur lors de l'ouverture du fichier pour ajout");
        return 0;
    }

    fprintf(file, "%s\n", line);
    fclose(file);
    return 1;
}

void traiter_addfriend(int idUtilisateur, const char *target_username)
{
    // Prevent adding oneself as a friend
    Utilisateur *utilisateur = trouverUtilisateurParId(idUtilisateur);
    if (strcmp(utilisateur->username, target_username) == 0)
    {
        write_client(utilisateur->sock, "Erreur : Vous ne pouvez pas vous ajouter en ami vous-même.\n");
        return;
    }

    // Check if target user exists in the users file
    FILE *file = fopen("Serveur/utilisateurs.txt", "r");
    if (file == NULL)
    {
        write_client(utilisateur->sock, "Erreur : Impossible d'accéder aux informations utilisateur.\n");
        return;
    }

    char line[256];
    int user_found = 0;

    while (fgets(line, sizeof(line), file))
    {
        char id[10], username[50], password[50];
        sscanf(line, "%[^,],%[^,],%[^,]", id, username, password);

        if (strcmp(username, target_username) == 0)
        {
            user_found = 1;

            // Prepare the friend request
            char friends_file[150];
            snprintf(friends_file, sizeof(friends_file), "Serveur/players/%s/friends", target_username);

            FILE *friends = fopen(friends_file, "a");
            if (friends == NULL)
            {
                write_client(utilisateur->sock, "Erreur : Impossible d'envoyer la demande d'ami.\n");
                fclose(file);
                return;
            }

            // Write the friend request to the target user's friends file
            fprintf(friends, "request:%s\n", utilisateur->username);
            fclose(friends);

            // Notify the user about the successful friend request
            char msg[150];
            snprintf(msg, sizeof(msg), "Demande d'ami envoyée à %s.\n", target_username);
            write_client(utilisateur->sock, msg);
            // Notify the target user about the friend request
            Utilisateur *adversaire = trouverUtilisateurParUsername(target_username);
            snprintf(msg, sizeof(msg), "Vous avez reçu une demande d'ami de %s.\n", utilisateur->username);
            write_client(adversaire->sock, msg);
            break;
        }
    }

    fclose(file);

    if (!user_found)
    {
        write_client(utilisateur->sock, "Erreur : Utilisateur introuvable.\n");
    }
}

int supprimer_ligne_fichier(const char *username, const char *filename, const char *line_to_remove)
{
    char filepath[150];
    snprintf(filepath, sizeof(filepath), "Serveur/players/%s/%s", username, filename);

    FILE *file = fopen(filepath, "r");
    if (!file)
    {
        char msg[150];
        snprintf(msg, sizeof(msg), "Erreur lors de l'ouverture du fichier pour suppression %s", filepath);
        perror(msg);
        return 0;
    }

    FILE *temp = fopen("temp.txt", "w");
    if (!temp)
    {
        perror("Erreur lors de la création du fichier temporaire");
        fclose(file);
        return 0;
    }

    char line[256];
    int line_removed = 0;

    while (fgets(line, sizeof(line), file))
    {
        if (strncmp(line, line_to_remove, strlen(line_to_remove)) != 0)
        {
            fputs(line, temp);
        }
        else
        {
            line_removed = 1;
        }
    }

    fclose(file);
    fclose(temp);

    remove(filepath);
    rename("temp.txt", filepath);

    return line_removed;
}

void traiter_faccept(int idUtilisateur, const char *friend_username)
{
    // Path to the current user's friend file
    Utilisateur *utilisateur = trouverUtilisateurParId(idUtilisateur);
    char friend_file[150];
    snprintf(friend_file, sizeof(friend_file), "Serveur/players/%s/friends", utilisateur->username);

    // Construct the line to remove from the user's friend file
    char line_to_remove[150];
    snprintf(line_to_remove, sizeof(line_to_remove), "request:%s", friend_username);

    // Remove the friend request from the user's file
    if (supprimer_ligne_fichier(utilisateur->username, "friends", line_to_remove))
    {
        char line_to_add[150];
        snprintf(line_to_add, sizeof(line_to_add), "friend:%s", friend_username);

        // Add the friend entry to the user's friend file
        ajouter_ligne_fichier(utilisateur->username, "friends", line_to_add);

        // Add the friend entry to the target user's friend file
        snprintf(line_to_add, sizeof(line_to_add), "friend:%s", utilisateur->username);
        ajouter_ligne_fichier(friend_username, "friends", line_to_add);

        write_client(utilisateur->sock, "Demande d'ami acceptée.\n");

        // Notify the friend if they are online
        Utilisateur *friend_user = trouverUtilisateurParUsername(friend_username);
        if (friend_user != NULL)
        {
            char msg[150];
            snprintf(msg, sizeof(msg), "%s a accepté votre demande d'ami.\n", utilisateur->username);
            write_client(friend_user->sock, msg);
        }
    }
    else
    {
        write_client(utilisateur->sock, "Aucune demande d'ami trouvée de cet utilisateur.\n");
    }
}

void traiter_fdecline(int idUtilisateur, const char *friend_username)
{
    // Construct the line to remove from the user's friend file
    Utilisateur *utilisateur = trouverUtilisateurParId(idUtilisateur);
    char line_to_remove[150];
    snprintf(line_to_remove, sizeof(line_to_remove), "request:%s", friend_username);

    // Attempt to remove the friend request
    if (supprimer_ligne_fichier(utilisateur->username, "friends", line_to_remove))
    {
        write_client(utilisateur->sock, "Demande d'ami refusée.\n");

        // Notify the friend if they are online
        Utilisateur *friend_user = trouverUtilisateurParUsername(friend_username);
        if (friend_user != NULL)
        {
            char msg[150];
            snprintf(msg, sizeof(msg), "%s a refusé votre demande d'ami :(.\n", utilisateur->username);
            write_client(friend_user->sock, msg);
        }
    }
    else
    {
        write_client(utilisateur->sock, "Aucune demande d'ami trouvée de cet utilisateur.\n");
    }
}

void lire_statistiques(const char *username, int *matches, int *wins, int *losses)
{
    char stats_file[150];
    snprintf(stats_file, sizeof(stats_file), "Serveur/players/%s/statistics", username);

    FILE *file = fopen(stats_file, "r");
    if (file)
    {
        char line[256];
        while (fgets(line, sizeof(line), file))
        {
            if (strncmp(line, "matches:", 8) == 0)
                *matches = atoi(line + 8);
            else if (strncmp(line, "wins:", 5) == 0)
                *wins = atoi(line + 5);
            else if (strncmp(line, "losses:", 7) == 0)
                *losses = atoi(line + 7);
        }
        fclose(file);
    }
    else
    {
        perror("Erreur lors de la lecture des statistiques");
        *matches = *wins = *losses = 0;
    }
}

void mettre_a_jour_statistiques(const char *username, int match_increment, int win_increment, int loss_increment)
{
    char stats_file[150];
    snprintf(stats_file, sizeof(stats_file), "Serveur/players/%s/statistics", username);

    int matches = 0, wins = 0, losses = 0;
    lire_statistiques(username, &matches, &wins, &losses);

    matches += match_increment;
    wins += win_increment;
    losses += loss_increment;

    FILE *file = fopen(stats_file, "w");
    if (file)
    {
        fprintf(file, "matches:%d\nwins:%d\nlosses:%d\n", matches, wins, losses);
        fclose(file);
    }
    else
    {
        perror("Erreur lors de la mise à jour des statistiques");
    }
}




int save_bio_to_file(const char *username, const char *bio)
{
    char bio_file[150];
    snprintf(bio_file, sizeof(bio_file), "Serveur/players/%s/bio", username);

    FILE *file = fopen(bio_file, "w");
    if (file == NULL)
    {
        perror("Erreur lors de l'ouverture du fichier bio");
        return 0;
    }

    fprintf(file, "%s", bio);
    fclose(file);
    return 1;
}

void traiter_bio(int idUtilisateur)
{
    Utilisateur *utilisateur = trouverUtilisateurParId(idUtilisateur);
    write_client(utilisateur->sock, "Entrer votre bio (max 10 lignes). Tapez /savebio pour enregistrer :\n");
    char bio[1024] = "";
    char line[128];
    int line_count = 0;
    while (line_count < 10)
    {
        int n = read_client(utilisateur->sock, line);
        line[strcspn(line, "\r\n")] = '\0'; // Remove trailing newline

        if (n <= 0 || strcmp(line, "/savebio") == 0)
        {
            break;
        }

        if (strlen(line) > 0)
        {
            strncat(bio, line, sizeof(bio) - strlen(bio) - 1);
            strncat(bio, "\n", sizeof(bio) - strlen(bio) - 1);
            line_count++;
        }
    }

    if (save_bio_to_file(utilisateur->username, bio))
    {
        write_client(utilisateur->sock, "Biographie enregistrée avec succès.\n");
    }
    else
    {
        write_client(utilisateur->sock, "Erreur lors de l'enregistrement de la biographie.\n");
    }
}

void get_user_bio(const char *username, char *bio, size_t bio_size)
{
    char bio_file[150];
    snprintf(bio_file, sizeof(bio_file), "Serveur/players/%s/bio", username);

    FILE *file = fopen(bio_file, "r");
    if (file)
    {
        size_t bytes_read = fread(bio, 1, bio_size - 1, file);
        bio[bytes_read] = '\0'; // Assure la terminaison du buffer
        fclose(file);

        // Vérifier si le fichier était vide ou uniquement composé d'espaces/blancs
        if (bytes_read == 0 || strlen(bio) == 0 || strspn(bio, " \t\r\n") == strlen(bio))
        {
            snprintf(bio, bio_size, "Aucune bio");
        }
    }
    else
    {
        snprintf(bio, bio_size, "Aucune bio"); // Message par défaut si le fichier n'existe pas
    }
}


void traiter_search(int idUtilisateur, const char *search_username)
{
    Utilisateur *utilisateur = trouverUtilisateurParId(idUtilisateur);
    if (!verifier_username(search_username))
    {
        
        write_client(utilisateur->sock, "Utilisateur introuvable.\n");
        return;
    }

    // Récupérer la bio de l'utilisateur
    char bio[1024];
    
    get_user_bio(search_username, bio, sizeof(bio));

    // Lire les statistiques
    int matches = 0, wins = 0, losses = 0;
    lire_statistiques(search_username, &matches, &wins, &losses);

    // Format et envoi des informations
    char info[BUF_SIZE];
    snprintf(info, sizeof(info),
             "Informations sur %s :\n- Bio : %s\n- Matchs joués : %d\n- Victoires : %d\n- Défaites : %d\n",
             search_username, bio, matches, wins, losses);
    write_client(utilisateur->sock, info);
}


void lire_relations(const char *friend_file, const char *prefix, char *output, size_t output_size)
{
    FILE *file = fopen(friend_file, "r");
    if (!file)
    {
        perror("Erreur lors de l'ouverture du fichier d'amis");
        snprintf(output, output_size, "Erreur : Impossible d'accéder aux informations d'amis.\n");
        return;
    }

    char line[256];
    output[0] = '\0'; // Initialize output to an empty string

    while (fgets(line, sizeof(line), file))
    {
        if (strncmp(line, prefix, strlen(prefix)) == 0)
        {
            strncat(output, line + strlen(prefix), output_size - strlen(output) - 1);
        }
    }

    fclose(file);
}

void traiter_friends(int idUtilisateur)
{
    char friend_file[150];
    Utilisateur *utilisateur = trouverUtilisateurParId(idUtilisateur);
    snprintf(friend_file, sizeof(friend_file), "Serveur/players/%s/friends", utilisateur->username);

    char output[BUF_SIZE];
    lire_relations(friend_file, "friend:", output, sizeof(output));

    if (strlen(output) > 0)
    {
        write_client(utilisateur->sock, "Vos amis :\n");
        write_client(utilisateur->sock, output);
    }
    else
    {
        write_client(utilisateur->sock, "Vous n'avez aucun ami.\n");
    }
}

void traiter_friendrequest(int idUtilisateur)
{
    char friend_file[150];
    Utilisateur *utilisateur = trouverUtilisateurParId(idUtilisateur);
    snprintf(friend_file, sizeof(friend_file), "Serveur/players/%s/friends", utilisateur->username);

    char output[BUF_SIZE];
    output[0] = '\0';

    // Read friend requests with "request:" prefix
    lire_relations(friend_file, "request:", output, sizeof(output));

    if (strlen(output) > 0)
    {
        write_client(utilisateur->sock, "Demandes d'amis reçues :\n");
        write_client(utilisateur->sock, output);
    }
    else
    {
        write_client(utilisateur->sock, "Vous n'avez aucune demande d'ami.\n");
    }
}


void traiter_ff(int idUtilisateur)
{
    Utilisateur *utilisateur = trouverUtilisateurParId(idUtilisateur);
    
    if (utilisateur->partieEnCours == NULL || utilisateur->estEnJeu != 1) {

        write_client(utilisateur->sock, "Erreur : Vous n'êtes pas en jeu.\n");
        return;
    }

    Salon *salon = trouverSalonParId(utilisateur->idPartieEnCours);
    Utilisateur *adversaire = (salon->joueur1 == utilisateur) ? salon->joueur2 : salon->joueur1;

    // Notifier les joueurs
    write_client(utilisateur->sock, "Vous avez abandonné la partie. Défaite enregistrée.\n");
    write_client(adversaire->sock, "Votre adversaire a abandonné la partie. Victoire enregistrée.\n");

    // Notifier les spectateurs

    for (int i = 0; i < salon->nbSpectateurs; i++)
    {
        Utilisateur *spectateur = trouverUtilisateurParId(salon->spectateurs[i]);

        char buffer[BUF_SIZE];
        snprintf(buffer, BUF_SIZE, "Le joueur %s a abandonné la partie. Le joueur %s est déclaré vainqueur.\n",
                 utilisateur->username, adversaire->username);
        write_client(spectateur->sock, buffer);
    }

    // Mettre à jour les statistiques
    mettre_a_jour_statistiques(utilisateur->username, 1, 0, 1); // Défaite pour l'utilisateur
    mettre_a_jour_statistiques(adversaire->username, 1, 1, 0);  // Victoire pour l'adversaire

    // Sauvegarder la partie
    char resultat[256];
    snprintf(resultat, sizeof(resultat), "Le joueur %s a abandonné. Victoire de %s.", utilisateur->username, adversaire->username);
    sauvegarderPartie(&salon->partie, salon->joueur1->username, salon->joueur2->username, resultat);

    // Réinitialiser l'état des joueurs et du salon
    utilisateur->estEnJeu = 0;
    adversaire->estEnJeu = 0;
    utilisateur->partieEnCours = NULL;
    adversaire->partieEnCours = NULL;
    salon->statut = 2; // Partie terminée
}


void ajouter_spectateur_salon(int idSalon, int idSpectateur)

{
    Salon *salon = trouverSalonParId(idSalon);
    Utilisateur *spectateur = trouverUtilisateurParId(idSpectateur);
    if (salon->nbSpectateurs >= MAX_SPECTATEURS)
    {
        write_client(spectateur->sock, "Erreur : Nombre maximum de spectateurs atteint pour cette partie.\n");
        return;
    }

    salon->spectateurs[salon->nbSpectateurs++] = spectateur->id;
}


void traiter_watch(int idSpectateur, const char *search_username)
{
    // Trouver l'utilisateur à observer

    Utilisateur *spectateur = trouverUtilisateurParId(idSpectateur);
    printf("pseudo du joueur que je veux regarder : %s\n",search_username);

    Utilisateur *joueur = trouverUtilisateurParUsername(search_username);

    if (joueur == NULL)
    {
        write_client(spectateur->sock, "Erreur : Joueur introuvable.\n");
        return;
    }

    if (!joueur->estEnJeu || joueur->partieEnCours == NULL)
    {
        write_client(spectateur->sock, "Erreur : Ce joueur n'est pas en train de jouer.\n");
        return;
    }

    // Ajouter le spectateur à la liste des spectateurs
    write_client(spectateur->sock, "Vous regardez la partie en cours...\n");

    // Envoyer l'état initial du plateau au spectateur
    envoyer_plateau_spectateur(idSpectateur, joueur->idPartieEnCours);

    // Ajouter le spectateur à la liste des spectateurs dans le salon
    ajouter_spectateur_salon(joueur->idPartieEnCours, idSpectateur);
}

void traiter_watch_game(int idSpectateur, int salonId)
{
    Utilisateur *spectateur = trouverUtilisateurParId(idSpectateur);

    // Vérifie si l'utilisateur est valide
    if (!spectateur)
    {
        write_client(spectateur->sock, "Erreur : Utilisateur introuvable.\n");
        return;
    }

    // Recherche du salon par ID
    Salon *salon = trouverSalonParId(salonId);
    if (!salon)
    {
        write_client(spectateur->sock, "Erreur : Salon introuvable.\n");
        return;
    }

    // Vérification de l'état du salon
    if (salon->statut != 1)
    {
        write_client(spectateur->sock, "Erreur : La partie demandée n'est pas en cours.\n");
        return;
    }

    // Utilise traiter_watch pour rejoindre la partie en observant le joueur 1
    traiter_watch(idSpectateur, salon->joueur1->username);
}


void startMatchmakingGame() {
    if (queueSize >= 2) {
        Utilisateur *player1 = trouverUtilisateurParId(queue[0]);
        Utilisateur *player2 = trouverUtilisateurParId(queue[1]);

        // Remove players from the queue
        for (int i = 2; i < queueSize; i++) {
            queue[i - 2] = queue[i];
        }
        queueSize -= 2;

        player1->estEnJeu = 1;
        player2->estEnJeu = 1;

        // Create a new salon (game session) for the two players
        creerSalon(player2->id, player1->id);
    }
}

void joinQueue(int idUtilisateur) {
    Utilisateur *utilisateur = trouverUtilisateurParId(idUtilisateur);
    if (utilisateur->estEnJeu != 0) {
        write_client(utilisateur->sock, "Vous ne pouvez pas rejoindre la file d'attente en étant en jeu.\n");
        return;
    }

    for (int i = 0; i < queueSize; i++) {
        if (queue[i] == idUtilisateur) {
            write_client(utilisateur->sock, "Vous êtes déjà dans la file d'attente.\n");
            return;
        }
    }

    if (queueSize < MAX_QUEUE) {
        queue[queueSize++] = idUtilisateur;
        utilisateur->estEnJeu = 4; // Set a special state to indicate they're in the queue
        write_client(utilisateur->sock, "Vous avez rejoint la file d'attente pour un match.\n");

        // Check if we can start a game
        if (queueSize >= 2) {
            startMatchmakingGame();
        }
    } else {
        write_client(utilisateur->sock, "La file d'attente est pleine.\n");
    }
}

// Function to remove a player from the queue
void leaveQueue(int idUtilisateur) {
    Utilisateur *utilisateur = trouverUtilisateurParId(idUtilisateur);
    int found = -1;
    for (int i = 0; i < queueSize; i++) {
        if (queue[i] == idUtilisateur) {
            found = i;
            break;
        }
    }

    if (found != -1) {
        // Shift players down in the queue
        for (int i = found; i < queueSize - 1; i++) {
            queue[i] = queue[i + 1];
        }
        queueSize--;
        utilisateur->estEnJeu = 0; // Reset state to not in game
        write_client(utilisateur->sock, "Vous avez quitté la file d'attente.\n");
    } else {
        write_client(utilisateur->sock, "Vous n'êtes pas dans la file d'attente.\n");
    }
}

void traiter_msg(int idUtilisateur, char* target_username, char* msg) 
{
    Utilisateur *utilisateur = trouverUtilisateurParId(idUtilisateur);
    Utilisateur *target = trouverUtilisateurParUsername(target_username);
    if (target == NULL) {
        write_client(utilisateur->sock, "Erreur : Utilisateur introuvable.\n");
        return;
    }

    char buffer[BUF_SIZE];
    snprintf(buffer, BUF_SIZE, "Message de %s : %s\n", utilisateur->username, msg);
    write_client(target->sock, buffer);
    write_client(utilisateur->sock, "Message envoyé.\n");
}

void traiter_unwatch(int idUtilisateur)
{
    // Vérifiez si l'utilisateur est un spectateur
    Utilisateur *utilisateur = trouverUtilisateurParId(idUtilisateur);  
    int found = 0;
    for (int i = 0; i < nbSalons; i++)
    {
        Salon *salon = &salons[i];
        for (int j = 0; j < salon->nbSpectateurs; j++)
        {
            if (salon->spectateurs[j] == utilisateur->id)
            {
                found = 1;

                // Retirez le spectateur de la liste
                for (int k = j; k < salon->nbSpectateurs - 1; k++)
                {
                    salon->spectateurs[k] = salon->spectateurs[k + 1];
                }
                salon->nbSpectateurs--;

                // Notifiez l'utilisateur
                write_client(utilisateur->sock, "Vous avez arrêté de regarder la partie.\n");

                // Quittez les boucles après avoir supprimé l'utilisateur
                break;
            }
        }
        if (found)
            break;
    }

    if (!found)
    {
        write_client(utilisateur->sock, "Erreur : Vous ne regardez aucune partie actuellement.\n");
    }
}


void traiterMessage(int idUtilisateur, char *message)
{
    Utilisateur *utilisateur = trouverUtilisateurParId(idUtilisateur);
    if (utilisateur->isConnected)
    {
        if (strncmp(message, "/challenge ", 11) == 0)
        {
            char *username = message + 11;
            Utilisateur *adversaire = trouverUtilisateurParUsername(username);

            // Vérifier si l'utilisateur tente de se défier lui-même
            if (adversaire == utilisateur)
            {
                write_client(utilisateur->sock, "Vous ne pouvez pas vous défier vous-même.\n");
                return;
            }

            // Vérifier les différents états pour éviter des défis multiples
            if (utilisateur->estEnJeu == 1)
            {
                write_client(utilisateur->sock, "Vous êtes déjà en jeu.\n");
            }
            else if (utilisateur->estEnJeu == 2)
            {
                write_client(utilisateur->sock, "Vous avez déjà lancé un défi. Utilisez /decline pour l'annuler.\n");
            }
            else if (adversaire == NULL)
            {
                write_client(utilisateur->sock, "Joueur introuvable.\n");
            }
            else if (adversaire->estEnJeu == 1)
            {
                write_client(utilisateur->sock, "Ce joueur est déjà en jeu.\n");
            }
            else if (adversaire->challenger != NULL)
            {
                write_client(utilisateur->sock, "Ce joueur a déjà un défi en attente.\n");
            }
            else
            {
                // Définir le défi en attente
                adversaire->challenger = utilisateur;
                utilisateur->challenger = adversaire;
                utilisateur->idchallenger = adversaire->id;
                adversaire->idchallenger = utilisateur->id;
                utilisateur->estEnJeu = 2;
                adversaire->estEnJeu = 3;

                // Informer les deux joueurs
                char buffer[BUF_SIZE];
                snprintf(buffer, BUF_SIZE, "Vous avez reçu un défi de la part de %s ! Tapez /accept pour accepter ou /decline pour refuser.\n", utilisateur->username);
                write_client(adversaire->sock, buffer);
                write_client(utilisateur->sock, "Défi envoyé ! En attente de réponse.\n");
            }
        }
        else if (strcmp(message, "/accept") == 0)
        {
            Utilisateur *challenger = trouverUtilisateurParId(utilisateur->idchallenger);
            if (challenger == NULL)
            {
                write_client(utilisateur->sock, "Erreur : Vous n'avez pas de défi à accepter.\n");
                return;
            }
            if (utilisateur->estEnJeu == 1)
            {
                write_client(utilisateur->sock, "Vous êtes déjà en jeu.\n");
            }
            else if (challenger == NULL || challenger->estEnJeu == 1)
            {
                write_client(utilisateur->sock, "Votre adversaire est déjà en jeu.\n");
            }
            else
            {
                // Créer un salon pour la partie
                creerSalon(utilisateur->idchallenger, idUtilisateur);

                // Nettoyer les états de défi pour les deux utilisateurs
                utilisateur->challenger = NULL;
                challenger->challenger = NULL;
                utilisateur->estEnJeu = 1;
                challenger->estEnJeu = 1;
            }
        }
        else if (strcmp(message, "/decline") == 0)
        {
            // Check if the user has a challenge
            if (utilisateur->challenger == NULL)
            {
                write_client(utilisateur->sock, "Erreur : Vous n'avez pas de défi à refuser.\n");
                return;
            }

            // Only the inviter (estEnJeu = 2) can cancel the challenge
            if (utilisateur->estEnJeu == 2)
            {
                Utilisateur *adversaire = utilisateur->challenger;

                // Reset challenge states
                utilisateur->estEnJeu = 0;
                adversaire->estEnJeu = 0;

                // Inform both users
                write_client(utilisateur->sock, "Défi annulé.\n");
                char buffer[BUF_SIZE];
                snprintf(buffer, BUF_SIZE, "Votre défi a été annulé par %s.\n", utilisateur->username);
                write_client(adversaire->sock, buffer);

                // Clear challenge references
                utilisateur->challenger = NULL;
                adversaire->challenger = NULL;
            }
            else if (utilisateur->estEnJeu == 3)
            {
                write_client(utilisateur->sock, "Défi refusé.\n");
                char buffer[BUF_SIZE];
                snprintf(buffer, BUF_SIZE, "Votre défi a été refusé par %s.\n", utilisateur->username);
                write_client(utilisateur->challenger->sock, buffer);

                // Reset challenge states
                utilisateur->estEnJeu = 0;
                utilisateur->challenger->estEnJeu = 0;

                // Clear challenge references
                utilisateur->challenger->challenger = NULL;
                utilisateur->challenger = NULL;
            }
        }
        else if (strncmp(message, "/play ", 6) == 0 && utilisateur->estEnJeu)
        {
            int caseJouee = atoi(message + 6) - 1;
            Salon *salon = trouverSalonParId(utilisateur->idPartieEnCours);
            if (salon != NULL) {
                playCoup(salon->id, idUtilisateur, caseJouee);
            } else {
                write_client(utilisateur->sock, "Erreur : Salon introuvable.\n");
            }
        }
        else if (strcmp(message, "/list") == 0)
        {
            char buffer[BUF_SIZE] = "Joueurs en ligne :\n";
            for (int i = 0; i < nbUtilisateursConnectes; i++)
            {
                strncat(buffer, utilisateurs[i].username, BUF_SIZE - strlen(buffer) - 1);
                strncat(buffer, "\n", BUF_SIZE - strlen(buffer) - 1);
            }
            write_client(utilisateur->sock, buffer);
        }
        else if (strcmp(message, "/friendrequest") == 0)
        {
            traiter_friendrequest(idUtilisateur);
        }
        else if (strncmp(message, "/search ", 7) == 0)
        {
            char search_username[50];
            if (sscanf(message + 8, "%49s", search_username) == 1)
            {
                traiter_search(idUtilisateur, search_username);
            }
            else
            {
                write_client(utilisateur->sock, "Format incorrect. Utilisez : /search [username]\n");
            }
        }
        else if (strcmp(message, "/queueup") == 0)
        {
            joinQueue(idUtilisateur);
        }
        else if (strcmp(message, "/leavequeue") == 0)
        {
            leaveQueue(idUtilisateur);
        }
        else if (strcmp(message, "/help") == 0)
        {
            envoyerAide(idUtilisateur);
        }
        else if (strncmp(message, "/login ", 7) == 0)
        {
            write_client(utilisateur->sock, "Vous devez être déconnecté pour utiliser cette commande !\n");
        }
        else if (strncmp(message, "/signin ", 8) == 0)
        {
            write_client(utilisateur->sock, "Vous devez être déconnecté pour utiliser cette commande !\n");
        }
        else if (strcmp(message, "/logout") == 0)
        {
            traiter_logout(idUtilisateur);
        }
        else if (strncmp(message, "/bio", 4) == 0)
        {
            traiter_bio(idUtilisateur);
        }
        else if (strncmp(message, "/addfriend ", 11) == 0)
        {
            char target_username[50];
            if (sscanf(message + 11, "%49s", target_username) == 1)
            {
                traiter_addfriend(idUtilisateur, target_username);
            }
            else
            {
                write_client(utilisateur->sock, "Format incorrect. Utilisez : /addfriend [username]\n");
            }
        }
        else if (strcmp(message, "/friends") == 0)
        {
            traiter_friends(idUtilisateur);
        }
        else if (strncmp(message, "/faccept ", 9) == 0)
        {
            char friend_username[50];
            if (sscanf(message + 9, "%49s", friend_username) == 1)
            {
                traiter_faccept(idUtilisateur, friend_username);
            }
            else
            {
                write_client(utilisateur->sock, "Format incorrect. Utilisez : /faccept [username]\n");
            }
        }
        else if (strncmp(message, "/fdecline ", 9) == 0)
        {
            char friend_username[50];
            if (sscanf(message + 9, "%49s", friend_username) == 1)
            {
                traiter_fdecline(idUtilisateur, friend_username);
            }
            else
            {
                write_client(utilisateur->sock, "Format incorrect. Utilisez : /fdecline [username]\n");
            }
        }
        else if (strcmp(message, "/public") == 0)
        {
            traiter_public(idUtilisateur);
        }
        else if (strcmp(message, "/private") == 0)
        {
            traiter_prive(idUtilisateur);
        }

        else if (strcmp(message, "/profile") == 0)
        {
            traiter_search(idUtilisateur, utilisateur->username);
            // Afficher mon profil
        }
        else if (strcmp(message, "/listepartie") == 0) {
            traiter_listeparties(idUtilisateur);
        }
        else if ((strcmp(message, "/ff") == 0) && utilisateur->estEnJeu)
        {
            traiter_ff(idUtilisateur);
            // traiter_search(utilisateur, utilisateur->username);
            // Afficher mon profil
        }
        else if ((strcmp(message, "/ff") == 0) && !utilisateur->estEnJeu)
        {
            write_client(utilisateur->sock, "Vous devez être en partie pour executer cette commande !\n");
           
        }
        else if (strncmp(message, "/watch ", 7) == 0) // Comparer 7 caractères ("/watch ")
        {
            char search_username[50];
            if (sscanf(message + 7, "%49s", search_username) == 1) // Décalage de 7 pour ignorer "/watch "
            {
                traiter_watch(idUtilisateur, search_username);
            }
            else
            {
                write_client(utilisateur->sock, "Format incorrect. Utilisez : /watch [username]\n");
            }
        }
        else if (strncmp(message, "/msg ", 5) == 0)
        {
            char target_username[50], msg[BUF_SIZE];
            if (sscanf(message + 5, "%49s %999[^\n]", target_username, msg) == 2)
            {
                traiter_msg(idUtilisateur, target_username, msg);
            }
            else
            {
                write_client(utilisateur->sock, "Format incorrect. Utilisez : /msg [username] [message]\n");
            }
        }
        else if (strcmp(message, "/unwatch") == 0)
        {
            traiter_unwatch(idUtilisateur);
        }
        else if (strcmp(message, "/leaderboard") == 0) {
            afficherClassementTop3(utilisateur);
            
        }
        else if (strncmp(message, "/watchgame", 10) == 0)
        {
            int salonId = -1; // Par défaut, un ID non valide
            if (sscanf(message + 11, "%d", &salonId) == 1) // Extraire un ID numérique
            {
                traiter_watch_game(idUtilisateur, salonId);
            }
            else
            {
                Utilisateur *utilisateur = trouverUtilisateurParId(idUtilisateur);
                write_client(utilisateur->sock, "Format incorrect. Utilisez : /watchgame [ID salon]\n");
            }
        }




        else
        {
            envoyerMessagePublic(idUtilisateur, message);
        }
    }
    else if (!utilisateur->isConnected)
    {
        if (strncmp(message, "/login ", 7) == 0)
        {
            char username[50], password[50];
            if (sscanf(message + 7, "%49s %49s", username, password) == 2)
            {
                traiter_login(idUtilisateur, username, password);
            }
            else
            {
                write_client(utilisateur->sock, "Format incorrect. Utilisez : /login [username] [password]\n");
            }
        }
        else if (strncmp(message, "/signin ", 8) == 0)
        {
            char username[50], password[50];
            if (sscanf(message + 8, "%49s %49s", username, password) == 2)
            {
                traiter_signin(idUtilisateur, username, password);
            }
            else
            {
                write_client(utilisateur->sock, "Format incorrect. Utilisez : /signin [username] [password]\n");
            }
        }
        else
        {
            write_client(utilisateur->sock, "Vous devez être connecté pour utiliser cette commande !\n");
        }
    }
}
static void app(void)
{
    SOCKET sock = init_connection();
    fd_set rdfs;
    char buffer[BUF_SIZE];
    Utilisateur *utilisateur;
    int max = sock;

    // Initialisation des structures
    initialiserUtilisateurs();
    initialiserSalons();

    while (1)
    {
        // Préparer le set de sockets pour `select`
        FD_ZERO(&rdfs);
        FD_SET(STDIN_FILENO, &rdfs);
        FD_SET(sock, &rdfs);

        // Ajouter tous les utilisateurs connectés
        for (int i = 0; i < nbUtilisateursConnectes; i++)
        {
            FD_SET(utilisateurs[i].sock, &rdfs);
            max = (utilisateurs[i].sock > max) ? utilisateurs[i].sock : max;
        }

        // Attente d'une activité sur un des sockets
        if (select(max + 1, &rdfs, NULL, NULL, NULL) == -1)
        {
            perror("select()");
            exit(errno);
        }

        // Gestion d'une entrée depuis le terminal pour arrêter le serveur
        if (FD_ISSET(STDIN_FILENO, &rdfs))
        {
            break;
        }

        // Gérer les nouvelles connexions
        if (FD_ISSET(sock, &rdfs))
        {
            SOCKADDR_IN csin = {0};
            socklen_t sinsize = sizeof csin;
            SOCKET csock = accept(sock, (SOCKADDR *)&csin, &sinsize);

            if (csock == SOCKET_ERROR)
            {
                perror("accept()");
                continue;
            }

            // Ajouter le nouvel utilisateur
            if (nbUtilisateursConnectes < MAX_CLIENTS)
            {
                utilisateur = &utilisateurs[nbUtilisateursConnectes++];
                utilisateur->id = nextUserId++;
                utilisateur->sock = csock;
                utilisateur->estEnJeu = 0;
                utilisateur->partieEnCours = NULL;
                utilisateur->joueur = NULL;

                // Lire le pseudo de l'utilisateur
                read_client(csock, utilisateur->pseudo);
                write_client(csock, "Bienvenue ! Utilisez /login pour vous connecter ou /signin pour s'inscrire.\n");
            }
            else
            {
                write_client(csock, "Nombre maximum d'utilisateurs atteint.\n");
                closesocket(csock);
            }
        }

        // Gérer les messages des utilisateurs existants
        for (int i = 0; i < nbUtilisateursConnectes; i++)
        {
            utilisateur = &utilisateurs[i];

            if (FD_ISSET(utilisateur->sock, &rdfs))
            {
                int n = read_client(utilisateur->sock, buffer);
                if (n == 0 || utilisateur->sock == INVALID_SOCKET)
                {
                    printf("Déconnexion de l'utilisateur : %s\n", utilisateur->username);

                    if (utilisateur->estEnJeu && utilisateur->partieEnCours != NULL)
                    {
                        traiter_ff(utilisateur->id); // Appeler traiter_ff pour enregistrer l'abandon
                    }

                    // Réinitialiser les états
                    utilisateur->isConnected = 0;
                    memset(utilisateur->username, 0, sizeof(utilisateur->username));
                    memset(utilisateur->pseudo, 0, sizeof(utilisateur->pseudo));

                    closesocket(utilisateur->sock);
                    utilisateur->sock = INVALID_SOCKET;

                    for (int j = i; j < nbUtilisateursConnectes - 1; j++)
                    {
                        utilisateurs[j] = utilisateurs[j + 1];
                    }
                    nbUtilisateursConnectes--;
                    i--;
                }


                else
                {
                    // Traiter le message de l'utilisateur
                    traiterMessage(utilisateur->id, buffer);
                }
            }
        }
    }

    // Fermeture de toutes les connexions restantes
    for (int i = 0; i < nbUtilisateursConnectes; i++)
    {
        closesocket(utilisateurs[i].sock);
    }
    for (int i = 0; i < nbUtilisateursConnectes; i++)
    {
        utilisateurs[i].isConnected = 0; // Réinitialiser l'état des utilisateurs
        closesocket(utilisateurs[i].sock);
    }

    end_connection(sock);
}

static void clear_clients(Client *clients, int actual)
{
    for (int i = 0; i < actual; i++)
    {
        closesocket(utilisateurs[i].sock);
    }
}

static void remove_client(Client *clients, int to_remove, int *actual)
{
    memmove(clients + to_remove, clients + to_remove + 1, (*actual - to_remove - 1) * sizeof(Client));
    (*actual)--;
}

static int init_connection(void)
{
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    SOCKADDR_IN sin = {0};

    if (sock == INVALID_SOCKET)
    {
        perror("socket()");
        exit(errno);
    }

    sin.sin_addr.s_addr = htonl(INADDR_ANY);
    sin.sin_port = htons(PORT);
    sin.sin_family = AF_INET;

    if (bind(sock, (SOCKADDR *)&sin, sizeof sin) == SOCKET_ERROR)
    {
        perror("bind()");
        exit(errno);
    }

    if (listen(sock, MAX_CLIENTS) == SOCKET_ERROR)
    {
        perror("listen()");
        exit(errno);
    }

    return sock;
}

static void end_connection(int sock)
{
    closesocket(sock);
}

static int read_client(SOCKET sock, char *buffer)
{
    int n = recv(sock, buffer, BUF_SIZE - 1, 0);
    if (n < 0)
    {
        perror("recv()");
        n = 0;
    }

    buffer[n] = 0;

    return n;
}

static void write_client(SOCKET sock, const char *buffer)
{
    if (send(sock, buffer, strlen(buffer), 0) < 0)
    {
        perror("send()");
        exit(errno);
    }
}

int main(int argc, char **argv)
{
    init();
    app();
    end();

    return EXIT_SUCCESS;
}
