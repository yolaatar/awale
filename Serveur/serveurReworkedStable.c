#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
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
    int estEnJeu;         // 1 si en jeu, 0 sinon
    int isConnected ;    // 1 si connecté, 0 sinon
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
}

void creerSalon(Utilisateur *joueur1, Utilisateur *joueur2)
{
    if (nbSalons >= MAX_SALONS)
    {
        printf("Nombre maximal de salons atteint.\n");
        return;
    }

    Salon *salon = &salons[nbSalons++];
    salon->joueur1 = joueur1;
    salon->joueur2 = joueur2;
    salon->tourActuel = 0;
    salon->statut = 1;

    initialiserPartie(&salon->partie);
    joueur1->estEnJeu = 1;
    joueur2->estEnJeu = 1;
    joueur1->partieEnCours = salon;
    joueur2->partieEnCours = salon;

    envoyer_plateau_aux_users(joueur1, joueur2, &salon->partie);
}

Utilisateur *trouverUtilisateurParPseudo(const char *pseudo)
{
    for (int i = 0; i < nbUtilisateursConnectes; i++)
    {
        if (strcmp(utilisateurs[i].pseudo, pseudo) == 0)
        {
            return &utilisateurs[i];
        }
    }
    return NULL;
}

void terminerPartie(Salon *salon)
{
    salon->statut = 2;
    salon->joueur1->estEnJeu = 0;
    salon->joueur2->estEnJeu = 0;
    write_client(salon->joueur1->sock, "La partie est terminée.\n");
    write_client(salon->joueur2->sock, "La partie est terminée.\n");
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
    snprintf(buffer, BUF_SIZE, "%s : %s\n", expediteur->pseudo, message);
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

    if (jouerCoup(partie, caseJouee, joueurIndex + 1) == 0)
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
    FILE *file = fopen("utilisateurs.txt", "r");
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

void traiter_login(Utilisateur *utilisateur, const char *username, const char *password) {
    if (est_deja_connecte(username)) {
        write_client(utilisateur->sock, "Erreur : Cet utilisateur est déjà connecté ailleurs.\n");
        return;
    }
    if (verifier_identifiants(username, password)) {
        ajouter_utilisateur_connecte(username);
        strncpy(utilisateur->username, username, sizeof(utilisateur->username) - 1);
        write_client(utilisateur->sock, "Connexion réussie !\n");
    } else {
        write_client(utilisateur->sock, "Identifiants incorrects.\n");
    }
}

void traiterMessage(Utilisateur *utilisateur, char *message)
{
    if (strncmp(message, "/challenge ", 11) == 0)
    {
        char *pseudo = message + 11;
        Utilisateur *adversaire = trouverUtilisateurParPseudo(pseudo);
        if (adversaire != NULL && adversaire->estEnJeu == 0)
        {
            adversaire->challenger = utilisateur; // Set pending challenge
            write_client(adversaire->sock, "Vous avez reçu un défi ! Tapez /accept pour accepter ou /decline pour refuser.\n");
            write_client(utilisateur->sock, "Défi envoyé ! En attente de réponse.\n");
        }
        else
        {
            write_client(utilisateur->sock, "Joueur non disponible.\n");
        }
    }
    else if (strcmp(message, "/accept") == 0 && utilisateur->challenger)
    {
        Utilisateur *challenger = utilisateur->challenger;
        creerSalon(challenger, utilisateur); // Create game if challenge is accepted
        utilisateur->challenger = NULL;      // Clear challenge
        challenger->challenger = NULL;
    }
    else if (strcmp(message, "/decline") == 0 && utilisateur->challenger)
    {
        write_client(utilisateur->challenger->sock, "Défi refusé.\n");
        write_client(utilisateur->sock, "Défi refusé.\n");
        utilisateur->challenger = NULL; // Clear challenge
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
            strncat(buffer, utilisateurs[i].pseudo, BUF_SIZE - strlen(buffer) - 1);
            strncat(buffer, "\n", BUF_SIZE - strlen(buffer) - 1);
        }
        write_client(utilisateur->sock, buffer);
    }
    else if (strcmp(message, "/help") == 0)
    {
        envoyerAide(utilisateur);
    }
    else if (strncmp(message, "/login ", 7) == 0)
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
        // Signup functionality
        // Extract username and password and call ajouter_utilisateur()
    }
    else if (strcmp(message, "/logout") == 0)
    {
        // Logout functionality
        // Call supprimer_utilisateur_connecte() and close socket
    }
    else if (strncmp(message, "/bio ", 5) == 0)
    {
        // Bio functionality
        // Retrieve or save bio based on input
    }
    else if (strncmp(message, "/addfriend ", 11) == 0)
    {
        // Add friend functionality
    }
    else if (strcmp(message, "/friends") == 0)
    {
        // Show friend list functionality
    }
    else if (strncmp(message, "/faccept ", 9) == 0)
    {
        // Accept friend request functionality
    }
    else
    {
        envoyerMessagePublic(utilisateur, message);
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
                snprintf(buffer, BUF_SIZE, "Bienvenue %s ! Utilisez /challenge <pseudo> pour lancer un défi.\n", utilisateur->pseudo);
                write_client(csock, buffer);
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
                if (n == 0)
                { // Déconnexion de l'utilisateur
                    printf("Déconnexion de %s\n", utilisateur->pseudo);
                    closesocket(utilisateur->sock);
                    // Gestion de la suppression de l'utilisateur
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
