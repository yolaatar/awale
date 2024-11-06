#include <stdio.h>
#include <string.h>

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

int initialiserPartie(Partie *partie) {
    for (int i = 0; i < 12; i++) {
        partie->plateau.cases[i].nbGraines = 4;
    }
    snprintf(partie->joueur1.pseudo, sizeof(partie->joueur1.pseudo), "Chaouki");
    snprintf(partie->joueur2.pseudo, sizeof(partie->joueur2.pseudo), "Yola Atar");

    partie->joueur1.score = 0;
    partie->joueur2.score = 0;
    return 0;
}

int afficherPlateau(Partie *partie) {
    printf("\n  Joueur 1 : %s, Score : %d\n", partie->joueur1.pseudo, partie->joueur1.score);

    // Affichage de la ligne supérieure (cases 0 à 5 pour Joueur 1)
    printf("   ");
    for (int i = 0; i < 6; i++) {
        printf("%2d ", partie->plateau.cases[i].nbGraines); // Affiche les cases de gauche à droite
    }
    printf("\n");

    // Affichage de la ligne inférieure (cases 11 à 6 pour Joueur 2)
    printf("   ");
    for (int i = 11; i >= 6; i--) {
        printf("%2d ", partie->plateau.cases[i].nbGraines); // Affiche les cases de droite à gauche
    }
    printf("\n");

    printf("  Joueur 2 : %s, Score : %d\n\n", partie->joueur2.pseudo, partie->joueur2.score);
    return 0;
}



int nombreGrainesRestantesJoueur(Partie *partie, int joueur) {
    int nbGraines = 0;
    for (int i = (joueur - 1) * 6; i < joueur * 6; i++) {
        nbGraines += partie->plateau.cases[i].nbGraines;
    }
    return nbGraines;
}

void capturerGraines(Partie *partie, int caseJouee, int joueur) {
    int nbGrainesPrises = 0;
    Plateau plateauTemp;
    memcpy(&plateauTemp, &(partie->plateau), sizeof(Plateau));

    int i = (caseJouee + 1) % 12;
    while ((plateauTemp.cases[i].nbGraines == 2 || plateauTemp.cases[i].nbGraines == 3) &&
           ((joueur == 1 && i < 6) || (joueur == 2 && i >= 6))) {
        nbGrainesPrises += plateauTemp.cases[i].nbGraines;
        plateauTemp.cases[i].nbGraines = 0;
        i = (i + 11) % 12; // Move backward
    }

    if (nbGrainesPrises < nombreGrainesRestantesJoueur(partie, 3 - joueur)) {
        if (joueur == 1) partie->joueur1.score += nbGrainesPrises;
        else partie->joueur2.score += nbGrainesPrises;
        memcpy(&(partie->plateau), &plateauTemp, sizeof(Plateau));
    }
}

int jouerCoup(Partie *partie, int caseJouee, int joueur) {
    if (partie->plateau.cases[caseJouee].nbGraines == 0 ||
        (joueur == 1 && caseJouee >= 6) ||
        (joueur == 2 && caseJouee < 6)) {
        return 1; // Coup illégal
    }

    int nbGraines = partie->plateau.cases[caseJouee].nbGraines;
    partie->plateau.cases[caseJouee].nbGraines = 0;

    for (int i = 1; i <= nbGraines; i++) {
        partie->plateau.cases[(caseJouee + i) % 12].nbGraines++;
    }

    capturerGraines(partie, (caseJouee + nbGraines) % 12, joueur);
    return 0;
}

int verifierFinPartie(Partie *partie) {
    if (partie->joueur1.score > 24 || partie->joueur2.score > 24) return 1;
    if (nombreGrainesRestantesJoueur(partie, 1) == 0 || nombreGrainesRestantesJoueur(partie, 2) == 0) return 1;
    return 0;
}

int main() {
    Partie partie;
    int partieEnCours = 1, tour = 1, caseJouee;

    initialiserPartie(&partie);

    while (partieEnCours) {
        afficherPlateau(&partie);
        int joueur = tour % 2 == 1 ? 1 : 2;
        printf("Tour du joueur %d (%s). Choisissez une case à jouer (1-6 pour J1, 7-12 pour J2): ", joueur, joueur == 1 ? partie.joueur1.pseudo : partie.joueur2.pseudo);
        scanf("%d", &caseJouee);

        //condition pour terminer la partie dans le terminal 
        if (caseJouee == -1) {
            partieEnCours = 0;
            break;
        }
        if (jouerCoup(&partie, caseJouee - 1, joueur) == 0) {
            if (verifierFinPartie(&partie)) {
                partieEnCours = 0;
                printf("\nFin de la partie !\n");
                afficherPlateau(&partie);
                if (partie.joueur1.score > partie.joueur2.score) printf("Le gagnant est %s !\n", partie.joueur1.pseudo);
                else if (partie.joueur1.score < partie.joueur2.score) printf("Le gagnant est %s !\n", partie.joueur2.pseudo);
                else printf("Match nul !\n");
            }
            tour++;
        } else {
            printf("Coup illégal, essayez encore.\n");
        }
    }
    return 0;
}
