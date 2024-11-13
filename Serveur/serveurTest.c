#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "server2.h"
#include "client2.h"
#include "awale.h"

#define MAX_CLIENTS 2
#define BUF_SIZE 1024

// Définir la structure de Partie globale
Partie partie;

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

// Fonction pour envoyer l'état du plateau aux clients
void envoyer_plateau_aux_clients(Client *clients, int actual, Partie *partie)
{
    char buffer[BUF_SIZE];
    snprintf(buffer, BUF_SIZE, "\n  Joueur 1 : %s, Score : %d\n", partie->joueur1.pseudo, partie->joueur1.score);

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

    snprintf(buffer + strlen(buffer), BUF_SIZE - strlen(buffer), "  Joueur 2 : %s, Score : %d\n\n", partie->joueur2.pseudo, partie->joueur2.score);
    strncat(buffer, "  ---------------------------------\n", BUF_SIZE - strlen(buffer) - 1);

    // Envoyer le plateau à tous les clients
    for (int i = 0; i < actual; i++)
    {
        write_client(clients[i].sock, buffer);
    }
}

// Fonction pour envoyer un message à tous les clients
static void send_message_to_all_clients(Client *clients, int actual, const char *buffer, char from_server, const char *sender_name)
{
    char message[BUF_SIZE];
    for (int i = 0; i < actual; i++)
    {
        // Préparer le message à envoyer
        message[0] = 0;
        if (from_server == 0 && sender_name != NULL)
        {
            strncpy(message, sender_name, BUF_SIZE - 1); // Inclure le nom de l'expéditeur
            strncat(message, " : ", sizeof(message) - strlen(message) - 1);
        }
        strncat(message, buffer, sizeof(message) - strlen(message) - 1);
        write_client(clients[i].sock, message);
    }
}

static void app(void)
{
    SOCKET sock = init_connection();
    char buffer[BUF_SIZE];
    fd_set rdfs;
    Client clients[MAX_CLIENTS];
    int actual = 0;
    int max = sock;
    int partieEnCours = 0; // Indicateur pour démarrer une partie
    int tour_actuel = 0;   // 0 pour le premier joueur, 1 pour le deuxième joueur
    int condition = 1;     // Condition pour vérifier si c'est le tour du joueur

    // Initialiser la partie pour une nouvelle session
    initialiserPartie(&partie);

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

        // Quitter en cas de saisie clavier
        if (FD_ISSET(STDIN_FILENO, &rdfs))
        {
            break;
        }
        else if (FD_ISSET(sock, &rdfs))
        {
            // Nouveau client
            SOCKADDR_IN csin = {0};
            socklen_t sinsize = sizeof csin;
            int csock = accept(sock, (SOCKADDR *)&csin, &sinsize);
            if (csock == SOCKET_ERROR)
            {
                perror("accept()");
                continue;
            }

            // Lire le pseudo du client
            if (read_client(csock, buffer) == -1)
            {
                continue;
            }

            max = csock > max ? csock : max;
            FD_SET(csock, &rdfs);

            Client c = {csock};
            strncpy(c.name, buffer, BUF_SIZE - 1);
            clients[actual] = c;
            actual++;

            // Lancer la partie quand il y a deux joueurs connectés
            if (actual == MAX_CLIENTS)
            {
                partieEnCours = 1;
                send_message_to_all_clients(clients, actual, "La partie commence!\n", 1, NULL);
                envoyer_plateau_aux_clients(clients, actual, &partie); // Afficher le plateau initial
                // on envoie respectivement a chaque joueur s'il commence ou s'il doit attendre le coup de l'autre
                write_client(clients[0].sock, "Vous commencez la partie.\n");
                write_client(clients[1].sock, "Vous devez attendre le coup de l'adversaire.\n");
            }
        }
        else if (partieEnCours)
        {
            // Gérer les messages entrants uniquement pour le joueur dont c'est le tour
            if (FD_ISSET(clients[tour_actuel].sock, &rdfs))
            {
                Client client = clients[tour_actuel];
                int joueur = (tour_actuel == 0) ? 1 : 2; // Déterminer le joueur en fonction de l'index

                // Demande au joueur de choisir une case
                snprintf(buffer, BUF_SIZE, "Joueur %d (%s), choisissez une case à jouer (1-6 pour J1, 7-12 pour J2): ", joueur, client.name);
                write_client(client.sock, buffer);

                // Lire le coup du joueur
                if (read_client(client.sock, buffer) > 0)
                {
                    int caseJouee = atoi(buffer) - 1;

                    // Exécuter le coup du joueur
                    if (jouerCoup(&partie, caseJouee, joueur) == 0)
                    {
                        // Envoyer l'état du plateau aux clients
                        envoyer_plateau_aux_clients(clients, actual, &partie);

                        // Vérifier la fin de partie
                        if (verifierFinPartie(&partie))
                        {
                            partieEnCours = 0;
                            snprintf(buffer, BUF_SIZE, "\nFin de la partie !\n");
                            send_message_to_all_clients(clients, actual, buffer, 1, NULL);

                            // Annonce du gagnant
                            if (partie.joueur1.score > partie.joueur2.score)
                            {
                                snprintf(buffer, BUF_SIZE, "Le gagnant est %s !\n", partie.joueur1.pseudo);
                            }
                            else if (partie.joueur1.score < partie.joueur2.score)
                            {
                                snprintf(buffer, BUF_SIZE, "Le gagnant est %s !\n", partie.joueur2.pseudo);
                            }
                            else
                            {
                                snprintf(buffer, BUF_SIZE, "Match nul !\n");
                            }
                            send_message_to_all_clients(clients, actual, buffer, 1, NULL);
                            clear_clients(clients, actual);
                            break;
                        }
                        else
                        {
                            // Envoyer un message spécifique au joueur qui vient de jouer
                            snprintf(buffer, BUF_SIZE, "Coup joué, attendez le coup de l'adversaire.\n");
                            write_client(client.sock, buffer);
                            condition = 1;

                            // Envoyer un message au joueur dont c'est maintenant le tour
                            int prochain_joueur = 1 - tour_actuel;
                            snprintf(buffer, BUF_SIZE, "Votre adversaire a joué, c'est votre tour!\n");
                            write_client(clients[prochain_joueur].sock, buffer);
                            // Alterner le tour pour l'autre joueur
                            tour_actuel = 1 - tour_actuel;
                        }
                    }
                    else
                    {
                        snprintf(buffer, BUF_SIZE, "Coup illégal, essayez encore.\n");
                        write_client(client.sock, buffer);
                    }
                }
            }
            else
            {
                // Gérer les messages entrants pour l'autre joueur (hors tour)
                for (int i = 0; i < actual; i++)
                {
                    if (i != tour_actuel && FD_ISSET(clients[i].sock, &rdfs) && condition == 1)
                    {
                        snprintf(buffer, BUF_SIZE, "Ce n'est pas votre tour, veuillez attendre.\n");
                        write_client(clients[i].sock, buffer);
                        condition = 0;
                    }
                }
            }
        }
    }

    clear_clients(clients, actual);
    end_connection(sock);
}

static void clear_clients(Client *clients, int actual)
{
    for (int i = 0; i < actual; i++)
    {
        closesocket(clients[i].sock);
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
