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

typedef struct {
    Utilisateur *joueur1;
    Utilisateur *joueur2;
    Partie partie;
    int tourActuel;              // 0 pour joueur1, 1 pour joueur2
    int statut;                  // 0 pour attente, 1 pour en cours, 2 pour terminé
} Salon;

struct Utilisateur {
    SOCKET sock;
    int id; 
    char pseudo[20];
    char username[50];
    char password[50];
    int estEnJeu;                // 1 si en jeu, 0 sinon
    Salon *partieEnCours;       // NULL si le joueur n'est pas en jeu
    Joueur *joueur;              // Pointeur vers le joueur (Joueur1 ou Joueur2) dans une partie
    Utilisateur *challenger;
};




int ajouter_ligne_fichier(const char *username, const char *filename, const char *line) {
    char filepath[150];
    snprintf(filepath, sizeof(filepath), "players/%s/%s", username, filename);

    FILE *file = fopen(filepath, "a");
    if (!file) {
        perror("Erreur lors de l'ouverture du fichier");
        return 0;
    }

    fprintf(file, "%s\n", line);
    fclose(file);
    return 1;
}

void lire_relations(const char *username, const char *prefix, char *output, size_t output_size) {
    char filename[150];
    printf("%s\n",username);
    snprintf(filename, sizeof(filename), "%s", username);

    // Débogage : Afficher le chemin complet du fichier
    printf("Débogage : Chemin du fichier friends = %s\n", filename);

    // Vérifier si le fichier existe
    if (access(filename, F_OK) != 0) {
        printf("Erreur : Le fichier %s n'existe pas.\n", filename);
        perror("Erreur détectée");
        return;
    }

    // Vérifier si le fichier est accessible en lecture
    if (access(filename, R_OK) != 0) {
        printf("Erreur : Le fichier %s n'est pas accessible en lecture.\n", filename);
        perror("Erreur détectée");
        return;
    }

    // Essayer d'ouvrir le fichier
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Erreur lors de l'ouverture du fichier friends");
        return;
    }

    // Lire le fichier ligne par ligne
    char line[256];
    output[0] = '\0'; // Initialiser la chaîne de sortie à une chaîne vide
    while (fgets(line, sizeof(line), file)) {
        if (strncmp(line, prefix, strlen(prefix)) == 0) {
            strncat(output, line + strlen(prefix), output_size - strlen(output) - 1);
        }
    }

    // Fermer le fichier
    fclose(file);
}


int supprimer_ligne_fichier(const char *username, const char *filename, const char *line_to_remove) {
    char filepath[150];
    snprintf(filepath, sizeof(filepath), "players/%s/%s", username, filename);

    FILE *file = fopen(filepath, "r");
    if (!file) {
        perror("Erreur lors de l'ouverture du fichier");
        return 0;
    }

    FILE *temp = fopen("temp.txt", "w");
    if (!temp) {
        perror("Erreur lors de la création du fichier temporaire");
        fclose(file);
        return 0;
    }

    char line[256];
    int line_removed = 0;

    while (fgets(line, sizeof(line), file)) {
        if (strncmp(line, line_to_remove, strlen(line_to_remove)) != 0) {
            fputs(line, temp);
        } else {
            line_removed = 1;
        }
    }

    fclose(file);
    fclose(temp);

    remove(filepath);
    rename("temp.txt", filepath);

    return line_removed;
}

// Fonction pour vérifier les identifiants dans le fichier
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

// Fonction pour ajouter un utilisateur dans le fichier
int ajouter_utilisateur(const char *username, const char *password) {
    if (verifier_identifiants(username, password)) {
        return 0; // L'utilisateur existe déjà
    }

    FILE *file = fopen("utilisateurs.txt", "a");
    if (!file) {
        perror("Erreur lors de l'ouverture du fichier utilisateurs.txt");
        return -1;
    }

    int new_id = 0;
    FILE *file_read = fopen("utilisateurs.txt", "r");
    if (file_read) {
        char line[150];
        Utilisateur user;
        while (fgets(line, sizeof(line), file_read)) {
            sscanf(line, "%d,%49[^,],%49s", &user.id, user.username, user.password);
            if (user.id > new_id) {
                new_id = user.id;
            }
        }
        fclose(file_read);
    }

    fprintf(file, "%d,%s,%s\n", new_id + 1, username, password);
    fclose(file);

    // Création du répertoire principal 'players'
    char base_dir[] = "players";

#ifdef _WIN32
    mkdir(base_dir);
#else
    mkdir(base_dir, 0777);
#endif

    // Création du sous-dossier de l'utilisateur
    char user_dir[150];
    snprintf(user_dir, sizeof(user_dir), "%s/%s", base_dir, username);

#ifdef _WIN32
    if (mkdir(user_dir) != 0) {
        perror("Erreur lors de la création du dossier utilisateur");
        return -1;
    }
#else
    if (mkdir(user_dir, 0777) != 0) {
        perror("Erreur lors de la création du dossier utilisateur");
        return -1;
    }
#endif

    char bio_file[150], friends_file[150];
    snprintf(bio_file, sizeof(bio_file), "%s/bio", user_dir);
    snprintf(friends_file, sizeof(friends_file), "%s/friends", user_dir);

    FILE *bio = fopen(bio_file, "w");
    if (bio) fclose(bio);

    FILE *friends = fopen(friends_file, "w");
    if (friends) fclose(friends);

    return 1; // Inscription réussie
}

int est_deja_connecte(const char *username)
{
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (connected_users[i].is_connected && strcmp(connected_users[i].username, username) == 0)
        {
            return 1; // L'utilisateur est déjà connecté
        }
    }
    return 0; // L'utilisateur n'est pas connecté
}

// Ajoute un utilisateur à la liste des connectés
void ajouter_utilisateur_connecte(const char *username)
{
    for (int i = 0; i < MAX_CONNECTED_USERS; i++)
    {
        if (!connected_users[i].is_connected) // Trouver un emplacement libre
        {
            strncpy(connected_users[i].username, username, sizeof(connected_users[i].username) - 1);
            connected_users[i].is_connected = 1;
            return;
        }
    }
}

// Supprime un utilisateur de la liste des connectés
void supprimer_utilisateur_connecte(const char *username)
{
    for (int i = 0; i < MAX_CONNECTED_USERS; i++)
    {
        if (connected_users[i].is_connected && strcmp(connected_users[i].username, username) == 0)
        {
            connected_users[i].is_connected = 0;
            memset(connected_users[i].username, 0, sizeof(connected_users[i].username));
            return;
        }
    }
}

int enregistrer_bio(const char *username, const char *bio) {
    char filename[150];
    snprintf(filename, sizeof(filename), "players/%s/bio", username);

    FILE *file = fopen(filename, "w");
    if (!file) {
        perror("Erreur lors de l'ouverture du fichier bio");
        return 0;
    }

    fprintf(file, "%s", bio);
    fclose(file);
    return 1;
}

int lire_bio(const char *username, char *buffer, size_t buffer_size) {
    char filename[150];
    snprintf(filename, sizeof(filename), "players/%s/bio", username);

    FILE *file = fopen(filename, "r");
    if (!file) {
        return 0; // Si le fichier n'existe pas ou erreur
    }

    fread(buffer, 1, buffer_size - 1, file);
    fclose(file);
    buffer[buffer_size - 1] = '\0'; // Assurer la terminaison
    return 1;
}

int ajouter_bio(const char *username, const char *bio)
{
    FILE *file = fopen("utilisateurs.txt", "r+");
    if (file == NULL)
    {
        perror("Erreur lors de l'ouverture du fichier utilisateurs.txt");
        return 0;
    }

    // Créer un fichier temporaire pour mettre à jour les données
    FILE *temp = fopen("utilisateurs_temp.txt", "w");
    if (temp == NULL)
    {
        perror("Erreur lors de la création du fichier temporaire");
        fclose(file);
        return 0;
    }

    char line[256];
    char updated_line[512];
    int bio_added = 0;

    while (fgets(line, sizeof(line), file))
    {
        char id[10], user[50], pass[50];
        sscanf(line, "%[^,],%[^,],%[^,\n]", id, user, pass);

        if (strcmp(user, username) == 0)
        {
            // Ajouter la bio à l'utilisateur
            snprintf(updated_line, sizeof(updated_line), "%s,%s,%s,%s\n", id, user, pass, bio);
            fputs(updated_line, temp);
            bio_added = 1;
        }
        else
        {
            fputs(line, temp); // Copier les autres lignes telles quelles
        }
    }

    fclose(file);
    fclose(temp);

    // Remplacer l'ancien fichier par le nouveau
    remove("utilisateurs.txt");
    rename("utilisateurs_temp.txt", "utilisateurs.txt");

    return bio_added;
}
