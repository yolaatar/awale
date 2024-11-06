// awale.h
#ifndef AWALE_H
#define AWALE_H

#define BUF_SIZE 1024

// Structures du jeu Awale
typedef struct {
    int nbGraines;
} Case;

typedef struct {
    Case cases[12];
} Plateau;

typedef struct {
    char pseudo[20];
    int score;
} Joueur;

typedef struct {
    Plateau plateau;
    Joueur joueur1;
    Joueur joueur2;
    Plateau historique[100];
} Partie;

// Fonctions du jeu Awale
void initialiserPartie(Partie *partie);
int jouerCoup(Partie *partie, int caseJouee, int joueur);
void afficherPlateau(Partie *partie);
int verifierFinPartie(Partie *partie);

#endif // AWALE_H
