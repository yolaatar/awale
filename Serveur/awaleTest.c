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
        partie->plateau.cases[i].nbGraines = 2;
    }

    // Plateau plateauTest = {
    // .cases = {{4}, {4}, {4}, {4}, {4}, {1}, {1}, {4}, {4}, {4}, {4}, {0}}
    // };
    // partie->plateau = plateauTest;

    snprintf(partie->joueur1.pseudo, sizeof(partie->joueur1.pseudo), "%s", partie->joueur1.pseudo);
    snprintf(partie->joueur2.pseudo, sizeof(partie->joueur2.pseudo), "%s", partie->joueur2.pseudo);


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
    printf("  ---------------------------------\n");
    return 0;
}

int nombreGrainesRestantesJoueur(Partie *partie, int joueur) {
    int nbGraines = 0;
    for (int i = (joueur - 1) * 6; i < joueur * 6; i++) {
        nbGraines += partie->plateau.cases[i].nbGraines;
    }
    return nbGraines;
}

int peutNourrirAdversaire(Partie *partie, int joueurAffame) {
    int adversaire = (joueurAffame == 1) ? 2 : 1;
    int debutAdversaire = (adversaire - 1) * 6;
    int finAdversaire = adversaire * 6;
    int debutJoueurAffame = (joueurAffame - 1) * 6;
    int finJoueurAffame = joueurAffame * 6;

    // Parcourt chaque case de l'adversaire pour voir si un coup peut nourrir le joueur affamé
    for (int i = debutAdversaire; i < finAdversaire; i++) {
        int nbGraines = partie->plateau.cases[i].nbGraines;
        if (nbGraines > 0) {
            // Simule la distribution des graines
            for (int j = 1; j <= nbGraines; j++) {
                int position = (i + j) % 12;
                // Vérifie si une graine tombe dans le camp du joueur affamé
                if (position >= debutJoueurAffame && position < finJoueurAffame) {
                    return 1; // L'adversaire peut nourrir le joueur affamé
                }
            }
        }
    }
    return 0; // L'adversaire ne peut pas nourrir le joueur affamé
}




// Vérifie si un coup donné nourrit l'adversaire
int coupNourritAdversaire(Partie *partie, int caseJouee, int joueur) {
    int nbGraines = partie->plateau.cases[caseJouee].nbGraines;
    int index = caseJouee;

    for (int i = 0; i < nbGraines; i++) {
        index = (index + 1) % 12;
    }

    int adversaire = (joueur == 1) ? 2 : 1;
    if ((adversaire == 1 && index < 6) || (adversaire == 2 && index >= 6)) {
        return 1; // Le coup nourrit l'adversaire
    }

    return 0; // Le coup ne nourrit pas l'adversaire
}

void capturerGraines(Partie *partie, int caseJouee, int joueur) {
    int nbGrainesPrises = 0;
    Plateau plateauTemp;
    memcpy(&plateauTemp, &(partie->plateau), sizeof(Plateau));

    int i = caseJouee;
    while ((plateauTemp.cases[i].nbGraines == 2 || plateauTemp.cases[i].nbGraines == 3) &&
           ((joueur == 1 && i >= 6) || (joueur == 2 && i < 6))) {
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
    int adversaire = (joueur == 1) ? 2 : 1;

    // Vérifie que le joueur joue bien dans son camp et que la case n'est pas vide
    if (partie->plateau.cases[caseJouee].nbGraines == 0) {
        printf("Coup illégal : la case est vide.\n");
        return 1;
    }
    if ((joueur == 1 && (caseJouee < 0 || caseJouee >= 6)) ||
        (joueur == 2 && (caseJouee < 6 || caseJouee >= 12))) {
        printf("Coup illégal : le joueur joue en dehors de son camp.\n");
        return 1;
    }

    // Si l'adversaire est en famine, le coup doit le nourrir
    if (nombreGrainesRestantesJoueur(partie, adversaire) == 0 && !coupNourritAdversaire(partie, caseJouee, joueur)) {
        printf("Coup illégal : l'adversaire est en famine, le coup doit le nourrir.\n");
        return 1;
    }

    int nbGraines = partie->plateau.cases[caseJouee].nbGraines;
    partie->plateau.cases[caseJouee].nbGraines = 0;

    int index = caseJouee;
    for (int i = 0; i < nbGraines; i++) {
        index = (index + 1) % 12;
        partie->plateau.cases[index].nbGraines++;
    }

    // Vérifie que la capture ne se fait que dans le camp adverse
    if ((joueur == 1 && index >= 6) || (joueur == 2 && index < 6)) {
        capturerGraines(partie, index, joueur);
    }

    return 0; // Coup valide
}


int peutForcerCapture(Partie *partie, int joueur) {
    int debutJoueur = (joueur - 1) * 6;
    int finJoueur = joueur * 6;
    int debutAdversaire = (joueur == 1) ? 6 : 0;
    int finAdversaire = (joueur == 1) ? 12 : 6;

    // Parcourt toutes les cases du joueur
    for (int i = debutJoueur; i < finJoueur; i++) {
        int nbGraines = partie->plateau.cases[i].nbGraines;
        if (nbGraines > 0) {
            // Simule la distribution des graines
            for (int j = 1; j <= nbGraines; j++) {
                int position = (i + j) % 12;
                // Vérifie si la dernière graine tombe dans une case de l'adversaire contenant 2 ou 3 graines
                if (position >= debutAdversaire && position < finAdversaire &&
                    j == nbGraines && // La dernière graine
                    (partie->plateau.cases[position].nbGraines == 2 || partie->plateau.cases[position].nbGraines == 3)) {
                    return 1; // Le joueur peut forcer une capture
                }
            }
        }
    }
    return 0; // Aucun coup ne permet de forcer une capture
}



int verifierFinPartie(Partie *partie) {
    // Vérifie si un joueur a plus de 24 points, ce qui signifie qu'il a gagné
    if (partie->joueur1.score > 24 || partie->joueur2.score > 24) {
        return 1;
    }

    // Vérifie si l'un des joueurs est en famine et si l'adversaire peut le nourrir
    if (nombreGrainesRestantesJoueur(partie, 1) == 0) {
        if (!peutNourrirAdversaire(partie, 1)) {
            //On distribue les graines restantes au joueur qui joue 
            for (int i = 6; i < 12; i++) {
                partie->joueur2.score += partie->plateau.cases[i].nbGraines;
                partie->plateau.cases[i].nbGraines = 0;
            }
            return 1; // Le joueur 1 est en famine et ne peut pas être nourri
        }
    } else if (nombreGrainesRestantesJoueur(partie, 2) == 0) {
        if (!peutNourrirAdversaire(partie, 2)) {
            //On distribue les graines restantes au joueur qui joue
            for (int i = 0; i < 6; i++) {
                partie->joueur1.score += partie->plateau.cases[i].nbGraines;
                partie->plateau.cases[i].nbGraines = 0;
            }
            return 1; // Le joueur 2 est en famine et ne peut pas être nourri
        }
    }

    // Vérifie si aucun joueur ne peut forcer une capture
    if (!peutForcerCapture(partie, 1) && !peutForcerCapture(partie, 2)) {
        return 1; // Aucun joueur ne peut forcer une capture, la partie se termine
    }

    return 0; // La partie peut continuer
}

// int main() {
//     Partie partie;
//     int partieEnCours = 1, tour = 1, caseJouee;

//     initialiserPartie(&partie);

//     while (partieEnCours) {
//         afficherPlateau(&partie);
//         int joueur = tour % 2 == 1 ? 1 : 2;
//         printf("Tour %d, joueur %d (%s). Choisissez une case à jouer (1-6 pour J1, 7-12 pour J2): ", tour, joueur, joueur == 1 ? partie.joueur1.pseudo : partie.joueur2.pseudo);
//         scanf("%d", &caseJouee);

//         //condition pour terminer la partie dans le terminal 
//         if (caseJouee == -1) {
//             printf("Partie annulée.\n");
//             partieEnCours = 0;
//             break;
//         }
//         // Ajuste l'index pour correspondre aux indices du tableau (0-11 au lieu de 1-12)
//         caseJouee -= 1;
//         if (jouerCoup(&partie, caseJouee, joueur) == 0) {
//             if (verifierFinPartie(&partie)) {
//                 partieEnCours = 0;
//                 printf("\nFin de la partie !\n");
//                 afficherPlateau(&partie);
//                 if (partie.joueur1.score > partie.joueur2.score) printf("Le gagnant est %s !\n", partie.joueur1.pseudo);
//                 else if (partie.joueur1.score < partie.joueur2.score) printf("Le gagnant est %s !\n", partie.joueur2.pseudo);
//                 else printf("Match nul !\n");
//             }
//             tour++;
//         } else {
//             //On vérifie si l'adversaire est en famine
//             if(joueur == 1 && nombreGrainesRestantesJoueur(&partie, 2) == 0) {
//                 printf("Le joueur 2 est en famine, vous devez le nourrir.\n");
//             } else if(joueur == 2 && nombreGrainesRestantesJoueur(&partie, 1) == 0) {
//                 printf("Le joueur 1 est en famine, vous devez le nourrir.\n");
//             } else
//             printf("Coup illégal, essayez encore.\n");
//         }
//     }
//     return 0;
// }
