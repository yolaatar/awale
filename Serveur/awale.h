// awale.h
#ifndef AWALE_H
#define AWALE_H

#include <stdio.h>

// Définition de constantes
#define BUF_SIZE 2048

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
    Plateau plateau ;
    Joueur joueur1 ;
    Joueur joueur2 ;
    Plateau historique[100] ;
    int tourActuel ;   
    int statut ; 
} Partie;

// Déclarations de fonctions
int initialiserPartie(Partie *partie);
int afficherPlateau(Partie *partie);
int nombreGrainesRestantesJoueur(Partie *partie, int joueur);
int peutNourrirAdversaire(Partie *partie, int joueurAffame);
int coupNourritAdversaire(Partie *partie, int caseJouee, int joueur);
void capturerGraines(Partie *partie, int caseJouee, int joueur);
int jouerCoup(Partie *partie, int caseJouee, int joueur);
int peutForcerCapture(Partie *partie, int joueur);
int verifierFinPartie(Partie *partie);

#endif // AWALE_H
