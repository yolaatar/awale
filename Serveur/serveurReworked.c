#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "server2.h"
#include "client2.h"
#include "awale.h"

#define BUF_SIZE 1024
#define MAX_CLIENTS 100
#define MAX_SALONS 50

// Déclaration anticipée de Utilisateur
typedef struct Utilisateur Utilisateur;

typedef struct
{
    Utilisateur *joueur1;
    Utilisateur *joueur2;
    Partie partie;
    int tourActuel; // 0 pour joueur1, 1 pour joueur2
    int statut;     // 0 pour attente, 1 pour en cours, 2 pour terminé
} Salon;

struct Utilisateur
{
    SOCKET sock;
    int id;
    char pseudo[20];
    char username[50];
    char password[50];
    int estEnJeu;         // 1 si en jeu, 0 si il ne l'est pas et 2 si il a déjà lancé un défi
    int isConnected;      // 1 si connecté, 0 sinon
    Salon *partieEnCours; // NULL si le joueur n'est pas en jeu
    Joueur *joueur;       // Pointeur vers le joueur (Joueur1 ou Joueur2) dans une partie
    Utilisateur *challenger;
};

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

Salon salons[MAX_SALONS];
Utilisateur utilisateurs[MAX_CLIENTS];
int nbUtilisateursConnectes = 0;
int nbSalons = 0;

void initialiserUtilisateurs(void)
{
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        utilisateurs[i].sock = INVALID_SOCKET;
        utilisateurs[i].estEnJeu = 0;
        utilisateurs[i].partieEnCours = NULL;
        utilisateurs[i].joueur = NULL;
        utilisateurs[i].isConnected = 0;
    }
}

void initialiserSalons(void)
{
    for (int i = 0; i < MAX_SALONS; i++)
    {
        salons[i].joueur1 = NULL;
        salons[i].joueur2 = NULL;
        salons[i].statut = 0;
    }
}

void handleNouvelleConnexion(SOCKET sock)
{
    if (nbUtilisateursConnectes >= MAX_CLIENTS)
    {
        printf("Nombre maximal de clients atteint.\n");
        closesocket(sock);
        return;
    }

    Utilisateur *utilisateur = &utilisateurs[nbUtilisateursConnectes];
    utilisateur->sock = sock;
    utilisateur->estEnJeu = 0;
    utilisateur->partieEnCours = NULL;
    nbUtilisateursConnectes++;
    printf("Nouvel utilisateur connecté !\n");

    // Envoyer un message d'accueil et les options
    write_client(sock, "Bienvenue ! Utilisez /challenge <nom> pour lancer un défi.\n");
}

void envoyer_plateau_aux_users(Utilisateur *joueur1, Utilisateur *joueur2, Partie *partie)
{
    char buffer[BUF_SIZE];

    // Préparation de l'affichage du plateau
    snprintf(buffer, BUF_SIZE, "  ---------------------------------\n");
    strncat(buffer, "  ---------------------------------\n", BUF_SIZE - strlen(buffer) - 1);
    snprintf(buffer, BUF_SIZE, "\n  Joueur 1 : %s, Score : %d\n",
             partie->joueur1.pseudo, partie->joueur1.score);

    // Ligne supérieure (cases 0 à 5 pour Joueur 1)
    strncat(buffer, "   ", BUF_SIZE - strlen(buffer) - 1);
    for (int i = 0; i < 6; i++)
    {
        char temp[10];
        snprintf(temp, 10, "%2d ", partie->plateau.cases[i].nbGraines);
        strncat(buffer, temp, BUF_SIZE - strlen(buffer) - 1);
    }
    strncat(buffer, "\n   ", BUF_SIZE - strlen(buffer) - 1);

    // Ligne inférieure (cases 11 à 6 pour Joueur 2)
    for (int i = 11; i >= 6; i--)
    {
        char temp[10];
        snprintf(temp, 10, "%2d ", partie->plateau.cases[i].nbGraines);
        strncat(buffer, temp, BUF_SIZE - strlen(buffer) - 1);
    }
    strncat(buffer, "\n", BUF_SIZE - strlen(buffer) - 1);

    snprintf(buffer + strlen(buffer), BUF_SIZE - strlen(buffer),
             "  Joueur 2 : %s, Score : %d\n",
             partie->joueur2.pseudo, partie->joueur2.score);

    strncat(buffer, "  ---------------------------------\n", BUF_SIZE - strlen(buffer) - 1);

    // Envoi du plateau aux deux joueurs
    write_client(joueur1->sock, buffer);
    write_client(joueur2->sock, buffer);

    // Envoyer a qui c'est le tour de jouer
    if (partie->tourActuel == 0)
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

void creerSalon(Utilisateur *joueur1, Utilisateur *joueur2)
{
    if (nbSalons >= MAX_SALONS)
    {
        printf("Nombre maximal de salons atteint.\n");

        return;
    }

    Salon *salon = &salons[nbSalons++];
    // tirer au sort qui sera le joueur 1 et le joueur 2
    if (rand() % 2 == 0)
    {
        salon->joueur1 = joueur1;
        strcpy(salon->partie.joueur1.pseudo, joueur1->username);
        salon->joueur2 = joueur2;
        strcpy(salon->partie.joueur2.pseudo, joueur2->username);
    }
    else
    {
        salon->joueur2 = joueur1;
        strcpy(salon->partie.joueur2.pseudo, joueur1->username);
        salon->joueur1 = joueur2;
        strcpy(salon->partie.joueur1.pseudo, joueur2->username);
    }
    salon->tourActuel = 0;
    salon->statut = 1;

    initialiserPartie(&salon->partie);
    joueur1->estEnJeu = 1;
    joueur2->estEnJeu = 1;
    joueur1->partieEnCours = salon;
    joueur2->partieEnCours = salon;

    envoyer_plateau_aux_users(joueur1, joueur2, &salon->partie);
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

void terminerPartie(Salon *salon)
{
    salon->statut = 2;
    salon->joueur1->estEnJeu = 0;
    salon->joueur2->estEnJeu = 0;
    salon->joueur1->partieEnCours = NULL;
    salon->joueur2->partieEnCours = NULL;
    salon->joueur1->challenger = NULL;
    salon->joueur2->challenger = NULL;

    write_client(salon->joueur1->sock, "La partie est terminée.\n");
    write_client(salon->joueur2->sock, "La partie est terminée.\n");

    if (salon->partie.joueur1.score > salon->partie.joueur2.score)
    {
        write_client(salon->joueur1->sock, "Vous avez gagné !\n");
        write_client(salon->joueur2->sock, "Vous avez perdu.\n");
    }
    else if (salon->partie.joueur1.score < salon->partie.joueur2.score)
    {
        write_client(salon->joueur1->sock, "Vous avez perdu.\n");
        write_client(salon->joueur2->sock, "Vous avez gagné !\n");
    }
    else
    {
        write_client(salon->joueur1->sock, "Match nul !\n");
        write_client(salon->joueur2->sock, "Match nul !\n");
    }
}

void envoyerAide(Utilisateur *utilisateur)
{
    write_client(utilisateur->sock, "Commandes disponibles :\n");
    write_client(utilisateur->sock, "/challenge <nom> - Défier un joueur\n");
    write_client(utilisateur->sock, "/play <case> - Jouer sur une case\n");
    write_client(utilisateur->sock, "/help - Afficher l'aide\n");
}

void envoyerMessagePublic(Utilisateur *expediteur, const char *message)
{
    char buffer[BUF_SIZE];
    snprintf(buffer, BUF_SIZE, "%s : %s\n", expediteur->username, message);
    for (int i = 0; i < nbUtilisateursConnectes; i++)
    {
        write_client(utilisateurs[i].sock, buffer);
    }
}

void playCoup(Salon *salon, Utilisateur *joueur, int caseJouee)
{
    Partie *partie = &salon->partie;
    int joueurIndex = (joueur == salon->joueur1) ? 0 : 1;

    if (salon->tourActuel != joueurIndex)
    {
        write_client(joueur->sock, "Ce n'est pas votre tour.\n");
        return;
    }

    else if (jouerCoup(partie, caseJouee, joueurIndex + 1) == 0)
    {
        envoyer_plateau_aux_users(salon->joueur1, salon->joueur2, partie);
        salon->tourActuel = 1 - salon->tourActuel;
    }
    else
    {
        write_client(joueur->sock, "Coup illégal, essayez encore.\n");
    }

    if (verifierFinPartie(partie))
    {
        terminerPartie(salon);
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

void ajouter_utilisateur_connecte(const char *username)
{
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (!utilisateurs[i].isConnected) // Trouver un emplacement libre
        {
            strncpy(utilisateurs[i].username, username, sizeof(utilisateurs[i].username) - 1);
            utilisateurs[i].isConnected = 1;
            return;
        }
    }
}

void traiter_login(Utilisateur *utilisateur, const char *username, const char *password)
{
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
        snprintf(buffer, BUF_SIZE, "Bienvenue %s ! Utilisez /challenge <username> pour lancer un défi.\n", utilisateur->username);
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

    return 1; // Inscription réussie
}

void traiter_signin(Utilisateur *utilisateur, const char *username, const char *password)
{
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

void supprimer_utilisateur_connecte(const char *username)
{
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (utilisateurs[i].isConnected && strcmp(utilisateurs[i].username, username) == 0)
        {
            utilisateurs[i].isConnected = 0;
            memset(utilisateurs[i].username, 0, sizeof(utilisateurs[i].username));
            return;
        }
    }
}

void traiter_logout(Utilisateur *utilisateur)
{
    printf("Utilisateur %s s'est déconnecté.\n", utilisateur->username);

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

void traiter_addfriend(Utilisateur *utilisateur, const char *target_username)
{
    // Prevent adding oneself as a friend
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

void traiter_faccept(Utilisateur *utilisateur, const char *friend_username)
{
    // Path to the current user's friend file
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

void traiter_fdecline(Utilisateur *utilisateur, const char *friend_username)
{
    // Construct the line to remove from the user's friend file
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

void traiter_bio(Utilisateur *utilisateur)
{
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
        fread(bio, 1, bio_size - 1, file);
        bio[bio_size - 1] = '\0'; // Ensure null termination
        fclose(file);
    }
    else
    {
        snprintf(bio, bio_size, "Non renseignée"); // Default message if bio not found
    }
}

void traiter_search(Utilisateur *utilisateur, const char *search_username)
{
    if (!verifier_username(search_username))
    {
        write_client(utilisateur->sock, "Utilisateur introuvable.\n");
        return;
    }

    // Retrieve user bio
    char bio[1024];
    get_user_bio(search_username, bio, sizeof(bio));

    // Format and send the user's information
    char info[BUF_SIZE];
    snprintf(info, sizeof(info),
             "Informations sur %s :\n- Bio : %s\n- Victoires : 0\n- Défaites : 0\n",
             search_username, bio);
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

void traiter_friends(Utilisateur *utilisateur)
{
    char friend_file[150];
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

void traiter_friendrequest(Utilisateur *utilisateur)
{
    char friend_file[150];
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

void traiterMessage(Utilisateur *utilisateur, char *message)
{
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
            if (utilisateur->challenger == NULL)
            {
                write_client(utilisateur->sock, "Erreur : Vous n'avez pas de défi à accepter.\n");
                return;
            }
            Utilisateur *challenger = utilisateur->challenger;

            if (utilisateur->estEnJeu == 1)
            {
                write_client(utilisateur->sock, "Vous êtes déjà en jeu.\n");
            }
            else if (challenger->estEnJeu == 1)
            {
                write_client(utilisateur->sock, "Votre adversaire est déjà en jeu.\n");
            }
            else
            {
                // Créer un salon pour la partie
                creerSalon(challenger, utilisateur);

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
            Salon *salon = utilisateur->partieEnCours;
            playCoup(salon, utilisateur, caseJouee);
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
            traiter_friendrequest(utilisateur);
        }
        else if (strncmp(message, "/search ", 7) == 0)
        {
            char search_username[50];
            if (sscanf(message + 8, "%49s", search_username) == 1)
            {
                traiter_search(utilisateur, search_username);
            }
            else
            {
                write_client(utilisateur->sock, "Format incorrect. Utilisez : /search [username]\n");
            }
        }
        else if (strcmp(message, "/help") == 0)
        {
            envoyerAide(utilisateur);
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
            traiter_logout(utilisateur);
        }
        else if (strncmp(message, "/bio", 4) == 0)
        {
            traiter_bio(utilisateur);
        }
        else if (strncmp(message, "/addfriend ", 11) == 0)
        {
            char target_username[50];
            if (sscanf(message + 11, "%49s", target_username) == 1)
            {
                traiter_addfriend(utilisateur, target_username);
            }
            else
            {
                write_client(utilisateur->sock, "Format incorrect. Utilisez : /addfriend [username]\n");
            }
        }
        else if (strcmp(message, "/friends") == 0)
        {
            traiter_friends(utilisateur);
        }
        else if (strncmp(message, "/faccept ", 9) == 0)
        {
            char friend_username[50];
            if (sscanf(message + 9, "%49s", friend_username) == 1)
            {
                traiter_faccept(utilisateur, friend_username);
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
                traiter_fdecline(utilisateur, friend_username);
            }
            else
            {
                write_client(utilisateur->sock, "Format incorrect. Utilisez : /fdecline [username]\n");
            }
        }
        else if (strcmp(message, "/private") == 0)
        {
            // MODE PRIVÉ
        }
        else if (strcmp(message, "/public") == 0)
        {
            // MODE PUBLIC
        }
        else
        {
            envoyerMessagePublic(utilisateur, message);
        }
    }
    else if (!utilisateur->isConnected)
    {
        if (strncmp(message, "/login ", 7) == 0)
        {
            char username[50], password[50];
            if (sscanf(message + 7, "%49s %49s", username, password) == 2)
            {
                traiter_login(utilisateur, username, password);
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
                traiter_signin(utilisateur, username, password);
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
                { // Déconnexion du client
                    printf("Déconnexion de l'utilisateur : %s\n", utilisateur->username);

                    // Réinitialiser isConnected et d'autres champs utilisateur
                    utilisateur->isConnected = 0;
                    memset(utilisateur->username, 0, sizeof(utilisateur->username));
                    memset(utilisateur->pseudo, 0, sizeof(utilisateur->pseudo));

                    // Fermer la connexion et réorganiser la liste
                    closesocket(utilisateur->sock);
                    utilisateur->sock = INVALID_SOCKET;

                    for (int j = i; j < nbUtilisateursConnectes - 1; j++)
                    {
                        utilisateurs[j] = utilisateurs[j + 1];
                    }
                    nbUtilisateursConnectes--;
                    i--; // Réajuster l'index pour ne pas sauter un utilisateur
                }

                else
                {
                    // Traiter le message de l'utilisateur
                    traiterMessage(utilisateur, buffer);
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
