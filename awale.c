#include <stdio.h>
#include <string.h>
// Définition des structures
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
  int i;
  for (i = 0; i < 12; i++) {
    partie->plateau.cases[i].nbGraines = 4;
  }
  // Initialisation des joueurs
  snprintf(partie->joueur1.pseudo, sizeof(partie->joueur1.pseudo), "Chaouki");
  snprintf(partie->joueur2.pseudo, sizeof(partie->joueur2.pseudo), "yolaatar");

  partie->joueur1.score = 0;
  partie->joueur2.score = 0;
  return 0;
}

int afficherPlateau(Partie *partie) {
  int i;
  printf("\n");
  printf("Joueur 1 : %s\n", partie->joueur1.pseudo);
  for (i = 0; i < 12; i++) {
    if (i >= 6) {
      printf("%d ", partie->plateau.cases[17 - i].nbGraines);
      continue;
    }
    printf("%d ", partie->plateau.cases[i].nbGraines);
    if (i == 5) {
      printf("\n");
    }
  }
  printf("\n");
  printf("Joueur 2 : %s\n", partie->joueur2.pseudo);
  printf("\n");
  return 0;
}

int nombreGrainesRestantesJoueur(Partie *partie, int joueur) {
  int i;
  int nbGraines = 0;
  for (i = 0 + (joueur - 1)*6; i < 12 - (2 - joueur)*6 ; i++) {
    nbGraines += partie->plateau.cases[i].nbGraines;
  }
  return nbGraines;
}

// fonction pour jouer un coup
int jouerCoup(Partie *partie, int caseJouee) {
  //Copie du plateau pour pouvoir annuler un coup si il est illégal
  Plateau plateau;
  Joueur joueurUn, joueurDeux;
  memcpy(&plateau, &(partie->plateau), sizeof(Plateau));
  //On récup le nombre de graines
  int nbGraines = partie->plateau.cases[caseJouee].nbGraines;
  plateau.cases[caseJouee].nbGraines = 0;
  int i = 0;
  //On distribue les graines avec la condition que si on repasse sur la même case on ne met pas de graines
  //On ne met pas de graines dans la case de départ
  while (nbGraines) {
    if ((caseJouee + i) % 12 == caseJouee) {
      i++;
      continue;
    }
    plateau.cases[(caseJouee + i) % 12].nbGraines++;
    nbGraines--;
    i++;
  }
  i--;
  //On vérifie si on a pris des graines
  Plateau plateauTemp;
  memcpy(&plateauTemp, &plateau, sizeof(Plateau));
  //Si c'est joueur 2 qui joue
  if (caseJouee > 5 && plateauTemp.cases[(caseJouee + i) % 12].nbGraines == 2 || plateauTemp.cases[(caseJouee + i) % 12].nbGraines == 3) {
      int nbGrainesAdversaire = nombreGrainesRestantesJoueur(partie, 1)  ;
      int nbGrainesPrises = 0;
      int j = 0;
      //Tant qu'on peut prendre des graines on les prend
      while(plateauTemp.cases[(caseJouee + i - j) % 12].nbGraines == 2 || plateauTemp.cases[(caseJouee + i - j) % 12].nbGraines == 3) {
          nbGrainesPrises += plateauTemp.cases[(caseJouee + i - j) % 12].nbGraines;
          plateauTemp.cases[(caseJouee + i - j) % 12].nbGraines = 0;
          j++;
      }
      //Si on prend toutes les graines de l'autre joueur on ne prend pas les graines
      if (nbGrainesPrises == nbGrainesAdversaire) {
          //On ne prend pas les graines
          joueurUn.score += nbGrainesPrises;
      }
      //Sinon on prend les graines
      else {
          //On prend les graines
          joueurUn.score += nbGrainesPrises;
          memcpy(&(partie->plateau), &plateauTemp, sizeof(Plateau));
      }
  }
    //Si c'est joueur 1 qui joue
    if (caseJouee < 6 && plateauTemp.cases[(caseJouee + i) % 12].nbGraines == 2 || plateauTemp.cases[(caseJouee + i) % 12].nbGraines == 3) {
        int nbGrainesAdversaire = nombreGrainesRestantesJoueur(partie, 2)  ;
        int nbGrainesPrises = 0;
        int j = 0;
        //Tant qu'on peut prendre des graines on les prend
        while(plateauTemp.cases[(caseJouee + i - j) % 12].nbGraines == 2 || plateauTemp.cases[(caseJouee + i - j) % 12].nbGraines == 3) {
            nbGrainesPrises += plateauTemp.cases[(caseJouee + i - j) % 12].nbGraines;
            plateauTemp.cases[(caseJouee + i - j) % 12].nbGraines = 0;
            j++;
        }
        //Si on prend toutes les graines de l'autre joueur on ne prend pas les graines
        if (nbGrainesPrises == nbGrainesAdversaire) {
            //On ne prend pas les graines
            joueurDeux.score += nbGrainesPrises;
        }
        //Sinon on prend les graines
        else {
            //On prend les graines
            joueurDeux.score += nbGrainesPrises;
            memcpy(&(partie->plateau), &plateauTemp, sizeof(Plateau));
        }
    }
    //on vérifie que le coup nourrit l'adversaire si celui est affamé
    if (caseJouee > 5) {
        for (int j = 6; j < 12; j++) {
            if (plateauTemp.cases[j].nbGraines > 0) {
                break;
            }
            if (j == 11) {
                //le coup est illégal on annule le coup
                return 1;
            }
        }
    }
    else {
        for (int j = 0; j < 6; j++) {
            if (plateauTemp.cases[j].nbGraines > 0) {
                break;
            }
            if (j == 5) {
                //Le coup est illégal on annule le coup
                return 1;
            }
        }
    }
    //On met à jour le plateau
    memcpy(&(partie->plateau), &plateau, sizeof(Plateau));
    //On met à jour le score
    if (caseJouee < 6) {
        partie->joueur1.score += nombreGrainesRestantesJoueur(partie, 1);
    }
    else {
        partie->joueur2.score += nombreGrainesRestantesJoueur(partie, 2);
    }
  return 0;
}

int main(int argc, char **argv) {
  // test affichage du plateau
  Partie partie;
  int partieEnCours = 1;
  int tour = 1;
  initialiserPartie(&partie);
  afficherPlateau(&partie);
  while(partieEnCours) {
      //On vérifie qu'aucun joueur n'a gagné
      //1 : On vérifie le score
      if (partie.joueur1.score > 25 || partie.joueur2.score > 25) {
          partieEnCours = 0;
          break;
      }
      //2 : On vérifie
  }
  return 0;


//A faire : finir la condition de personne peut jouer (créer la fonction mirroir de simulation)
//         finir la condition de joueur affamé impossible à nourrir
//        faire la distribution des graines dans ces cas là
//       Gérer le jeu a tour de role + gérer l'input correct selon le joueur qui joue
