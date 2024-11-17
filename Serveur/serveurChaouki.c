int ajouter_ligne_fichier(const char *username, const char *filename, const char *line)
{
    char filepath[150];
    snprintf(filepath, sizeof(filepath), "Serveur/players/%s/%s", username, filename);

    FILE *file = fopen(filepath, "a");
    if (!file)
    {
        perror("Erreur lors de l'ouverture du fichier pour ajout");
        return 0;
    }

    fprintf(file, "%s\n", line);
    fclose(file);
    return 1;
}

void traiter_addfriend(Utilisateur *utilisateur, const char *target_username)
{
    // Prevent adding oneself as a friend
    if (strcmp(utilisateur->username, target_username) == 0)
    {
        write_client(utilisateur->sock, "Erreur : Vous ne pouvez pas vous ajouter en ami vous-même.\n");
        return;
    }

    // Check if target user exists in the users file
    FILE *file = fopen("Serveur/utilisateurs.txt", "r");
    if (file == NULL)
    {
        write_client(utilisateur->sock, "Erreur : Impossible d'accéder aux informations utilisateur.\n");
        return;
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

            // Prepare the friend request
            char friends_file[150];
            snprintf(friends_file, sizeof(friends_file), "Serveur/players/%s/friends", target_username);

            FILE *friends = fopen(friends_file, "a");
            if (friends == NULL)
            {
                write_client(utilisateur->sock, "Erreur : Impossible d'envoyer la demande d'ami.\n");
                fclose(file);
                return;
            }

            // Write the friend request to the target user's friends file
            fprintf(friends, "request:%s\n", utilisateur->username);
            fclose(friends);

            // Notify the user about the successful friend request
            char msg[150];
            snprintf(msg, sizeof(msg), "Demande d'ami envoyée à %s.\n", target_username);
            write_client(utilisateur->sock, msg);
            // Notify the target user about the friend request
            Utilisateur *adversaire = trouverUtilisateurParUsername(target_username);
            snprintf(msg, sizeof(msg), "Vous avez reçu une demande d'ami de %s.\n", utilisateur->username);
            write_client(adversaire->sock, msg);
            break;
        }
    }

    fclose(file);

    if (!user_found)
    {
        write_client(utilisateur->sock, "Erreur : Utilisateur introuvable.\n");
    }
}

int supprimer_ligne_fichier(const char *username, const char *filename, const char *line_to_remove)
{
    char filepath[150];
    snprintf(filepath, sizeof(filepath), "Serveur/players/%s/%s", username, filename);

    FILE *file = fopen(filepath, "r");
    if (!file)
    {
        char msg[150];
        snprintf(msg, sizeof(msg), "Erreur lors de l'ouverture du fichier pour suppression %s", filepath);
        perror(msg);
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

void traiter_faccept(Utilisateur *utilisateur, const char *friend_username)
{
    // Path to the current user's friend file
    char friend_file[150];
    snprintf(friend_file, sizeof(friend_file), "Serveur/players/%s/friends", utilisateur->username);

    // Construct the line to remove from the user's friend file
    char line_to_remove[150];
    snprintf(line_to_remove, sizeof(line_to_remove), "request:%s", friend_username);

    // Remove the friend request from the user's file
    if (supprimer_ligne_fichier(utilisateur->username, "friends", line_to_remove))
    {
        char line_to_add[150];
        snprintf(line_to_add, sizeof(line_to_add), "friend:%s", friend_username);

        // Add the friend entry to the user's friend file
        ajouter_ligne_fichier(utilisateur->username, "friends", line_to_add);

        // Add the friend entry to the target user's friend file
        snprintf(line_to_add, sizeof(line_to_add), "friend:%s", utilisateur->username);
        ajouter_ligne_fichier(friend_username, "friends", line_to_add);

        write_client(utilisateur->sock, "Demande d'ami acceptée.\n");

        // Notify the friend if they are online
        Utilisateur *friend_user = trouverUtilisateurParUsername(friend_username);
        if (friend_user != NULL)
        {
            char msg[150];
            snprintf(msg, sizeof(msg), "%s a accepté votre demande d'ami.\n", utilisateur->username);
            write_client(friend_user->sock, msg);
        }
    }
    else
    {
        write_client(utilisateur->sock, "Aucune demande d'ami trouvée de cet utilisateur.\n");
    }
}

void traiter_fdecline(Utilisateur *utilisateur, const char *friend_username) {
    // Construct the line to remove from the user's friend file
    char line_to_remove[150];
    snprintf(line_to_remove, sizeof(line_to_remove), "request:%s", friend_username);

    // Attempt to remove the friend request
    if (supprimer_ligne_fichier(utilisateur->username, "friends", line_to_remove)) {
        write_client(utilisateur->sock, "Demande d'ami refusée.\n");

        // Notify the friend if they are online
        Utilisateur *friend_user = trouverUtilisateurParUsername(friend_username);
        if (friend_user != NULL) {
            char msg[150];
            snprintf(msg, sizeof(msg), "%s a refusé votre demande d'ami :(.\n", utilisateur->username);
            write_client(friend_user->sock, msg);
        }
    } else {
        write_client(utilisateur->sock, "Aucune demande d'ami trouvée de cet utilisateur.\n");
    }
}
