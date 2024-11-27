#ifndef SERVER_H
#define SERVER_H

#ifdef WIN32

#include <winsock2.h>
#include <sys/select.h>

#elif defined(linux) || defined(__APPLE__)

#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h> /* close */
#include <netdb.h> /* gethostbyname */
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define closesocket(s) close(s)
typedef int SOCKET;
typedef struct sockaddr_in SOCKADDR_IN;
typedef struct sockaddr SOCKADDR;
typedef struct in_addr IN_ADDR;

#else

#error not defined for this platform

#endif

#define CRLF        "\r\n"
#define PORT         1977
#define BUF_SIZE    2048

// Constantes supplémentaires
#define MAX_CLIENTS 100
#define MAX_SALONS 50
#define MAX_QUEUE 100
#define MAX_SPECTATEURS 10

#include "client2.h"
#include "awale.h"

// Structures supplémentaires
typedef struct Utilisateur Utilisateur;

typedef struct {
    int id;
    Utilisateur *joueur1;
    Utilisateur *joueur2;
    int idJoueur1;
    int idJoueur2;
    Partie partie;
    int tourActuel; // 0 : joueur1, 1 : joueur2
    int statut;     // 0 : attente, 1 : en cours, 2 : terminé
    int spectateurs[MAX_SPECTATEURS];
    int nbSpectateurs;
} Salon;

struct Utilisateur {
    SOCKET sock;
    int id;
    char pseudo[20];
    char username[50];
    char password[50];
    int estPublic;        // 1 : public, 0 : privé
    int estEnJeu;         // 0 : pas en jeu, 1 : en jeu, 2 : défi lancé
    int isConnected;      // 1 : connecté, 0 : déconnecté
    Salon *partieEnCours; // NULL si pas en jeu
    int idPartieEnCours;
    Joueur *joueur;       // Pointeur vers Joueur1 ou Joueur2
    Utilisateur *challenger; // Défi lancé par cet utilisateur
    int idchallenger;
};


// Variables globales supplémentaires
extern int queue[MAX_QUEUE];
extern int queueSize;
extern Salon salons[MAX_SALONS];
extern Utilisateur utilisateurs[MAX_CLIENTS];
extern int nbUtilisateursConnectes;
extern int nbSalons;


static void init(void);
static void end(void);
static void app(void);
static int init_connection(void);
static void end_connection(int sock);
static int read_client(SOCKET sock, char *buffer);
static void write_client(SOCKET sock, const char *buffer);
static void send_message_to_all_clients(Client *clients, int actual, const char *buffer, char from_server, const char *sender_name);
static void remove_client(Client *clients, int to_remove, int *actual);
static void clear_clients(Client *clients, int actual);
int est_deja_connecte(const char *username);
int verifier_identifiants(const char *username, const char *password);
int ajouter_utilisateur(const char *username, const char *password);
int supprimer_ligne_fichier(const char *username, const char *filename, const char *line_to_remove);
int ajouter_ligne_fichier(const char *username, const char *filename, const char *line);
int supprimer_ligne_fichier(const char *username, const char *filename, const char *line_to_remove);
int save_bio_to_file(const char *username, const char *bio);
int verifier_username(const char *username);
void get_user_bio(const char *username, char *bio, size_t bio_size);
void lire_relations(const char *friend_file, const char *prefix, char *output, size_t output_size);
void ajouter_spectateur_salon(int idSalon, int idSpectateur);
Utilisateur *trouverUtilisateurParId(int idjoueur);
Salon *trouverSalonParId(int idSalon);
void initialiserUtilisateurs(void);
void initialiserSalons(void);
void creerSalon(int idJoueur1, int idJoueur2);
void terminerPartie(int idSalon);
void traiter_ff(int idUtilisateur);
void envoyer_plateau_spectateur(int idSpectateur, int idSalon);
void envoyer_plateau_aux_users(int idJoueur1, int idJoueur2, Partie *partie, int tour);
void mettre_a_jour_statistiques(const char *username, int match_increment, int win_increment, int loss_increment);
Utilisateur *trouverUtilisateurParUsername(const char *username);
void traiterMessage(int idUtilisateur, char *message);

#endif /* SERVER_H */
