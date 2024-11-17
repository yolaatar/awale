# Variables de compilation
CC = gcc
CFLAGS = 
SERVER_SOURCES = Serveur/awaleTest.c Serveur/serveurReworked.c
CLIENT_SOURCES = Client/client2.c
SERVER_OBJECTS = $(SERVER_SOURCES:.c=.o)
CLIENT_OBJECTS = $(CLIENT_SOURCES:.c=.o)
SERVER_EXEC = serveurAwale
CLIENT_EXEC = clientAwale
UTILISATEURS_FILE = Serveur/utilisateurs.txt
PLAYERS_DIR = Serveur/players

# Liste d'utilisateurs pré-créés pour "populate"
USERS = "1,player1,mdp1" "2,player2,mdp2" "3,player3,mdp3"

# Règle par défaut
all: $(SERVER_EXEC) $(CLIENT_EXEC) $(UTILISATEURS_FILE)

# Compilation du serveur
$(SERVER_EXEC): $(SERVER_OBJECTS)
	$(CC) $(CFLAGS) -o $@ $(SERVER_OBJECTS)

# Compilation du client
$(CLIENT_EXEC): $(CLIENT_OBJECTS)
	$(CC) $(CFLAGS) -o $@ $(CLIENT_OBJECTS)

# Compilation des fichiers objets pour le serveur
Serveur/%.o: Serveur/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

# Compilation des fichiers objets pour le client
Client/%.o: Client/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

# Création du fichier utilisateurs.txt vide
$(UTILISATEURS_FILE):
	@echo "Création de $(UTILISATEURS_FILE)..."
	@touch $@

# Création du dossier players
$(PLAYERS_DIR):
	@echo "Création du dossier $(PLAYERS_DIR)..."
	@mkdir -p $(PLAYERS_DIR)

# Ajouter des utilisateurs pré-créés et générer leurs dossiers
populate: $(UTILISATEURS_FILE) $(PLAYERS_DIR)
	@echo "Ajout d'utilisateurs prédéfinis dans $(UTILISATEURS_FILE)..."
	@echo "" > $(UTILISATEURS_FILE) # Réinitialiser le fichier
	@for user in $(USERS); do \
	    echo "$$user" >> $(UTILISATEURS_FILE); \
	done
	@echo "Création des dossiers pour les utilisateurs..."
	@while IFS=, read -r id username password; do \
	    mkdir -p $(PLAYERS_DIR)/$$username; \
	    echo "Bio de $$username" > $(PLAYERS_DIR)/$$username/bio; \
	    echo "Deuxième ligne de bio pour $$username" >> $(PLAYERS_DIR)/$$username/bio; \
	    touch $(PLAYERS_DIR)/$$username/friends; \
	done < $(UTILISATEURS_FILE)
	@echo "Les utilisateurs et dossiers ont été créés avec succès."

# Nettoyage
clean:
	rm -f Serveur/*.o Client/*.o $(SERVER_EXEC) $(CLIENT_EXEC)
	@if [ -f $(UTILISATEURS_FILE) ]; then \
		echo "Suppression de $(UTILISATEURS_FILE)..."; \
		rm -f $(UTILISATEURS_FILE); \
	fi
	@if [ -d $(PLAYERS_DIR) ]; then \
		echo "Suppression du dossier $(PLAYERS_DIR)..."; \
		rm -rf $(PLAYERS_DIR); \
	fi

# Phony targets
.PHONY: all clean populate
