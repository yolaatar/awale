#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "server2.h"
#include "client2.h"
#include "awale.h"
#include <sys/stat.h>
#include <sys/types.h>

#define MAX_CLIENTS 2
#define BUF_SIZE 1024

// Définir la structure de Partie globale
Partie partie;

// Structure pour représenter un utilisateur
typedef struct
{
    int id;
    char username[50];
    char password[50];
} Utilisateur;

// Structure pour gérer les utilisateurs connectés
typedef struct
{
    char username[50];
    int is_connected;
} ConnectedUser;

#define MAX_CONNECTED_USERS MAX_CLIENTS
ConnectedUser connected_users[MAX_CONNECTED_USERS] = {0}; // Liste globale des utilisateurs connectés

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

int enregistrer_bio(const char *username, const char *bio)
{
    char filename[150];
    snprintf(filename, sizeof(filename), "players/%s/bio", username);

    FILE *file = fopen(filename, "w");
    if (!file)
    {
        perror("Erreur lors de l'ouverture du fichier bio");
        return 0;
    }

    fprintf(file, "%s", bio);
    fclose(file);
    return 1;
}

int ajouter_ligne_fichier(const char *username, const char *filename, const char *line)
{
    char filepath[150];
    snprintf(filepath, sizeof(filepath), "players/%s/%s", username, filename);

    FILE *file = fopen(filepath, "a");
    if (!file)
    {
        perror("Erreur lors de l'ouverture du fichier");
        return 0;
    }

    fprintf(file, "%s\n", line);
    fclose(file);
    return 1;
}

void lire_relations(const char *username, const char *prefix, char *output, size_t output_size)
{
    char filename[150];
    printf("%s\n", username);
    snprintf(filename, sizeof(filename), "%s", username);

    // Débogage : Afficher le chemin complet du fichier
    printf("Débogage : Chemin du fichier friends = %s\n", filename);

    // Vérifier si le fichier existe
    if (access(filename, F_OK) != 0)
    {
        printf("Erreur : Le fichier %s n'existe pas.\n", filename);
        perror("Erreur détectée");
        return;
    }

    // Vérifier si le fichier est accessible en lecture
    if (access(filename, R_OK) != 0)
    {
        printf("Erreur : Le fichier %s n'est pas accessible en lecture.\n", filename);
        perror("Erreur détectée");
        return;
    }

    // Essayer d'ouvrir le fichier
    FILE *file = fopen(filename, "r");
    if (!file)
    {
        perror("Erreur lors de l'ouverture du fichier friends");
        return;
    }

    // Lire le fichier ligne par ligne
    char line[256];
    output[0] = '\0'; // Initialiser la chaîne de sortie à une chaîne vide
    while (fgets(line, sizeof(line), file))
    {
        if (strncmp(line, prefix, strlen(prefix)) == 0)
        {
            strncat(output, line + strlen(prefix), output_size - strlen(output) - 1);
        }
    }

    // Fermer le fichier
    fclose(file);
}

int supprimer_ligne_fichier(const char *username, const char *filename, const char *line_to_remove)
{
    char filepath[150];
    snprintf(filepath, sizeof(filepath), "players/%s/%s", username, filename);

    FILE *file = fopen(filepath, "r");
    if (!file)
    {
        perror("Erreur lors de l'ouverture du fichier");
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

int lire_bio(const char *username, char *buffer, size_t buffer_size)
{
    char filename[150];
    snprintf(filename, sizeof(filename), "players/%s/bio", username);

    FILE *file = fopen(filename, "r");
    if (!file)
    {
        return 0; // Si le fichier n'existe pas ou erreur
    }

    fread(buffer, 1, buffer_size - 1, file);
    fclose(file);
    buffer[buffer_size - 1] = '\0'; // Assurer la terminaison
    return 1;
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
int ajouter_utilisateur(const char *username, const char *password)
{
    if (verifier_identifiants(username, password))
    {
        return 0; // L'utilisateur existe déjà
    }

    FILE *file = fopen("utilisateurs.txt", "a");
    if (!file)
    {
        perror("Erreur lors de l'ouverture du fichier utilisateurs.txt");
        return -1;
    }

    int new_id = 0;
    FILE *file_read = fopen("utilisateurs.txt", "r");
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
    if (mkdir(user_dir) != 0)
    {
        perror("Erreur lors de la création du dossier utilisateur");
        return -1;
    }
#else
    if (mkdir(user_dir, 0777) != 0)
    {
        perror("Erreur lors de la création du dossier utilisateur");
        return -1;
    }
#endif

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

// Vérifie si un utilisateur est déjà connecté
int est_deja_connecte(const char *username)
{
    for (int i = 0; i < MAX_CONNECTED_USERS; i++)
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

// Fonction principale
static void app(void)
{
    SOCKET sock = init_connection();
    char buffer[BUF_SIZE];
    fd_set rdfs;
    Client clients[MAX_CLIENTS];
    int actual = 0;
    int max = sock;

    while (1)
    {
        FD_ZERO(&rdfs);
        FD_SET(STDIN_FILENO, &rdfs);
        FD_SET(sock, &rdfs);

        for (int i = 0; i < actual; i++)
        {
            FD_SET(clients[i].sock, &rdfs);
        }

        if (select(max + 1, &rdfs, NULL, NULL, NULL) == -1)
        {
            perror("select()");
            exit(errno);
        }

        if (FD_ISSET(STDIN_FILENO, &rdfs))
        {
            break;
        }
        else if (FD_ISSET(sock, &rdfs))
        {
            SOCKADDR_IN csin = {0};
            socklen_t sinsize = sizeof csin;
            int csock = accept(sock, (SOCKADDR *)&csin, &sinsize);
            if (csock == SOCKET_ERROR)
            {
                perror("accept()");
                continue;
            }

            max = csock > max ? csock : max;
            FD_SET(csock, &rdfs);

            Client c = {csock};
            clients[actual] = c;
            actual++;

            write_client(c.sock, "Veuillez vous connecter avec /login [username] [password] ou vous inscrire avec /signin [username] [password].\n");
        }
        else
        {
            for (int i = 0; i < actual; i++)
            {
                if (FD_ISSET(clients[i].sock, &rdfs))
                {
                    int n = read_client(clients[i].sock, buffer);

                    if (n <= 0 || strlen(buffer) == 0)
                    {
                        continue;
                    }

                    buffer[strcspn(buffer, "\r\n")] = '\0';

                    if (strncmp(buffer, "/logout", 7) == 0)
                    {
                        supprimer_utilisateur_connecte(clients[i].name);
                        write_client(clients[i].sock, "Vous avez été déconnecté. Au revoir !\n");

                        printf("Utilisateur '%s' s'est déconnecté.\n", clients[i].name);

                        closesocket(clients[i].sock);
                        for (int j = i; j < actual - 1; j++)
                        {
                            clients[j] = clients[j + 1];
                        }
                        actual--;
                        continue;
                    }

                    if (strncmp(buffer, "/login ", 7) == 0)
                    {
                        char username[50], password[50];
                        if (sscanf(buffer + 7, "%49s %49s", username, password) == 2)
                        {
                            if (est_deja_connecte(username))
                            {
                                write_client(clients[i].sock, "Erreur : Cet utilisateur est déjà connecté ailleurs.\n");
                                continue;
                            }

                            if (verifier_identifiants(username, password))
                            {
                                ajouter_utilisateur_connecte(username);
                                strncpy(clients[i].name, username, sizeof(clients[i].name) - 1);
                                write_client(clients[i].sock, "Connexion réussie !\n");
                                envoyer_menu_principal(&clients[i]);

                                printf("Utilisateur '%s' s'est connecté.\n", username);
                            }
                            else
                            {
                                write_client(clients[i].sock, "Identifiants incorrects. Veuillez réessayer avec /login [username] [password].\n");
                            }
                        }
                        else
                        {
                            write_client(clients[i].sock, "Format incorrect. Utilisez : /login [username] [password]\n");
                        }
                    }

                    else if (strncmp(buffer, "/signin ", 8) == 0)
                    {
                        char username[50], password[50];
                        if (sscanf(buffer + 8, "%49s %49s", username, password) == 2)
                        {
                            int result = ajouter_utilisateur(username, password);
                            if (result == 1)
                            {
                                char user_dir[150];
                                snprintf(user_dir, sizeof(user_dir), "players/%s", username);

#ifdef _WIN32
                                mkdir(user_dir);
#else
                                mkdir(user_dir, 0777);
#endif

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

                                write_client(clients[i].sock, "Inscription réussie ! Vous pouvez maintenant vous connecter avec /login [username] [password].\n");
                                printf("Nouvel utilisateur inscrit et dossier créé : '%s'\n", username);
                            }
                            else if (result == 0)
                            {
                                write_client(clients[i].sock, "Nom d'utilisateur déjà pris. Veuillez en choisir un autre.\n");
                            }
                            else
                            {
                                write_client(clients[i].sock, "Erreur lors de l'inscription. Veuillez réessayer.\n");
                            }
                        }
                        else
                        {
                            write_client(clients[i].sock, "Format incorrect. Utilisez : /signin [username] [password]\n");
                        }
                    }
                    else if (strncmp(buffer, "/bio", 4) == 0)
                    {
                        write_client(clients[i].sock, "Entrez votre bio (max 10 lignes). Tapez /savebio pour enregistrer :\n");

                        char bio[1024] = "";
                        char line[128];
                        int line_count = 0;

                        while (line_count < 10)
                        {
                            int n = read_client(clients[i].sock, line);
                            line[strcspn(line, "\r\n")] = '\0';

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

                        char bio_file[150];
                        snprintf(bio_file, sizeof(bio_file), "players/%s/bio", clients[i].name);

                        FILE *file = fopen(bio_file, "w");
                        if (file != NULL)
                        {
                            fprintf(file, "%s", bio);
                            fclose(file);
                            write_client(clients[i].sock, "Votre bio a été enregistrée avec succès !\n");
                        }
                        else
                        {
                            write_client(clients[i].sock, "Erreur lors de l'enregistrement de votre bio. Veuillez réessayer.\n");
                        }

                        envoyer_menu_principal(&clients[i]);
                    }

                    else if (strncmp(buffer, "/addfriend ", 11) == 0)
                    {
                        char target_username[50];
                        if (sscanf(buffer + 11, "%49s", target_username) == 1)
                        {
                            if (strcmp(clients[i].name, target_username) == 0)
                            {
                                write_client(clients[i].sock, "Erreur : Vous ne pouvez pas vous demander en ami vous-même.\n");
                                continue;
                            }

                            FILE *file = fopen("utilisateurs.txt", "r");
                            if (file == NULL)
                            {
                                write_client(clients[i].sock, "Erreur : Impossible d'accéder aux informations utilisateur.\n");
                                continue;
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

                                    char friends_file[150];
                                    snprintf(friends_file, sizeof(friends_file), "players/%s/friends", target_username);

                                    FILE *friends = fopen(friends_file, "a");
                                    if (friends == NULL)
                                    {
                                        write_client(clients[i].sock, "Erreur : Impossible d'envoyer la demande d'ami.\n");
                                        fclose(file);
                                        continue;
                                    }

                                    fprintf(friends, "request:%s\n", clients[i].name);
                                    fclose(friends);

                                    char msg[150];
                                    snprintf(msg, sizeof(msg), "Demande d'ami envoyée à %s.\n", target_username);
                                    write_client(clients[i].sock, msg);
                                    break;
                                }
                            }

                            fclose(file);

                            if (!user_found)
                            {
                                write_client(clients[i].sock, "Erreur : Utilisateur introuvable.\n");
                            }
                        }
                        else
                        {
                            write_client(clients[i].sock, "Format incorrect. Utilisez : /addfriend [username]\n");
                        }
                    }
                    else if (strncmp(buffer, "/friends", 8) == 0)
                    {
                        char friend_file[150];
                        snprintf(friend_file, sizeof(friend_file), "players/%s/friends", clients[i].name);

                        char output[BUF_SIZE];
                        lire_relations(friend_file, "friend:", output, sizeof(output));

                        if (strlen(output) > 0)
                        {
                            write_client(clients[i].sock, "Vos amis :\n");
                            write_client(clients[i].sock, output);
                        }
                        else
                        {
                            write_client(clients[i].sock, "Vous n'avez aucun ami.\n");
                        }
                    }
                    else if (strncmp(buffer, "/friendrequest", 14) == 0)
                    {
                        char friend_file[150];
                        snprintf(friend_file, sizeof(friend_file), "players/%s/friends", clients[i].name);

                        char output[BUF_SIZE];
                        output[0] = '\0';

                        // Lire les demandes d'amis avec le préfixe "request:"
                        lire_relations(friend_file, "request:", output, sizeof(output));

                        if (strlen(output) > 0)
                        {
                            write_client(clients[i].sock, "Demandes d'amis reçues :\n");
                            write_client(clients[i].sock, output);
                        }
                        else
                        {
                            write_client(clients[i].sock, "Vous n'avez aucune demande d'ami.\n");
                        }
                    }
                    else if (strncmp(buffer, "/faccept ", 9) == 0)
                    {
                        char friend_username[50];
                        if (sscanf(buffer + 8, "%49s", friend_username) == 1)
                        {
                            char friend_file[150];
                            snprintf(friend_file, sizeof(friend_file), "players/%s/friends", clients[i].name);

                            char line_to_remove[150];
                            snprintf(line_to_remove, sizeof(line_to_remove), "request:%s", friend_username);

                            if (supprimer_ligne_fichier(clients[i].name, "friends", line_to_remove))
                            {
                                char line_to_add[150];
                                snprintf(line_to_add, sizeof(line_to_add), "friend:%s", friend_username);

                                // Ajouter l'ami dans le fichier du joueur actuel
                                ajouter_ligne_fichier(clients[i].name, "friends", line_to_add);

                                // Ajouter l'ami dans le fichier du joueur cible
                                char friend_file_other[150];
                                snprintf(friend_file_other, sizeof(friend_file_other), "players/%s/friends", friend_username);
                                snprintf(line_to_add, sizeof(line_to_add), "friend:%s", clients[i].name);
                                ajouter_ligne_fichier(friend_username, "friends", line_to_add);

                                write_client(clients[i].sock, "Demande d'ami acceptée.\n");
                            }
                            else
                            {
                                write_client(clients[i].sock, "Aucune demande de cet utilisateur.\n");
                            }
                        }
                        else
                        {
                            write_client(clients[i].sock, "Format incorrect. Utilisez : /accept [username]\n");
                        }
                    }
                    else if (strncmp(buffer, "/fdecline ", 9) == 0)
                    {
                        char friend_username[50];
                        if (sscanf(buffer + 8, "%49s", friend_username) == 1)
                        {
                            char line_to_remove[150];
                            snprintf(line_to_remove, sizeof(line_to_remove), "request:%s", friend_username);

                            if (supprimer_ligne_fichier(clients[i].name, "friends", line_to_remove))
                            {
                                write_client(clients[i].sock, "Demande d'ami refusée.\n");
                            }
                            else
                            {
                                write_client(clients[i].sock, "Aucune demande de cet utilisateur.\n");
                            }
                        }
                        else
                        {
                            write_client(clients[i].sock, "Format incorrect. Utilisez : /refuse [username]\n");
                        }
                    }
                    else if (strncmp(buffer, "/search ", 8) == 0)
                    {
                        char search_username[50];
                        if (sscanf(buffer + 8, "%49s", search_username) == 1)
                        {
                            // Vérifier si l'utilisateur existe dans "utilisateurs.txt"
                            FILE *file = fopen("utilisateurs.txt", "r");
                            if (!file)
                            {
                                write_client(clients[i].sock, "Erreur : Impossible d'accéder aux informations.\n");
                                perror("Erreur ouverture utilisateurs.txt");
                                continue;
                            }

                            char line[256], id[10], username[50], password[50];
                            int user_found = 0;

                            while (fgets(line, sizeof(line), file))
                            {
                                sscanf(line, "%[^,],%[^,],%[^,]", id, username, password);
                                if (strcmp(username, search_username) == 0)
                                {
                                    user_found = 1;
                                    break;
                                }
                            }
                            fclose(file);

                            if (!user_found)
                            {
                                write_client(clients[i].sock, "Utilisateur introuvable.\n");
                            }
                            else
                            {
                                // Charger la bio de l'utilisateur cible
                                char bio[1024] = "";
                                char bio_file[150];
                                snprintf(bio_file, sizeof(bio_file), "players/%s/bio", search_username);

                                FILE *bio_fp = fopen(bio_file, "r");
                                if (bio_fp)
                                {
                                    fread(bio, 1, sizeof(bio) - 1, bio_fp);
                                    fclose(bio_fp);
                                }
                                else
                                {
                                    snprintf(bio, sizeof(bio), "Non renseignée");
                                }

                                // Afficher les informations de l'utilisateur
                                char info[BUF_SIZE];
                                snprintf(info, sizeof(info),
                                         "Informations sur %s :\n- Bio : %s\n- Victoires : 0\n- Défaites : 0\n",
                                         search_username, bio);
                                write_client(clients[i].sock, info);
                            }
                        }
                        else
                        {
                            write_client(clients[i].sock, "Format incorrect. Utilisez : /search [username]\n");
                        }
                    }
                    else if (strncmp(buffer, "/menu", 5) == 0)
                    {
                        write_client(clients[i].sock, "Retour au menu principal.\n");
                        envoyer_menu_principal(&clients[i]);
                    }
                }
            }
        }
    }

    clear_clients(clients, actual);
    end_connection(sock);
}

// Nettoyage des clients
static void clear_clients(Client *clients, int actual)
{
    for (int i = 0; i < actual; i++)
    {
        closesocket(clients[i].sock);
    }
}

// Initialisation de la connexion
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

// Fermeture de la connexion
static void end_connection(int sock)
{
    closesocket(sock);
}

// Lire les données du client
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

// Écrire des données au client
static void write_client(SOCKET sock, const char *buffer)
{
    if (send(sock, buffer, strlen(buffer), 0) < 0)
    {
        perror("send()");
        exit(errno);
    }
}

// Fonction principale
int main(int argc, char **argv)
{
    init();
    app();
    end();
    return EXIT_SUCCESS;
}