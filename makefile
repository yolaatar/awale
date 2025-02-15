# Variables de compilation
CC = gcc
CFLAGS = 
SERVER_SOURCES = Serveur/awale.c Serveur/serveurReworked.c
CLIENT_SOURCES = Client/client2.c
SERVER_OBJECTS = $(SERVER_SOURCES:.c=.o)
CLIENT_OBJECTS = $(CLIENT_SOURCES:.c=.o)
SERVER_EXEC = serveurAwale
CLIENT_EXEC = clientAwale
UTILISATEURS_FILE = Serveur/utilisateurs.txt
PLAYERS_DIR = Serveur/players

# Liste d'utilisateurs pré-créés pour "database"
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
database: $(UTILISATEURS_FILE) $(PLAYERS_DIR)
	@echo "Ajout d'utilisateurs prédéfinis dans $(UTILISATEURS_FILE)..."
	@echo "" > $(UTILISATEURS_FILE) # Réinitialiser le fichier
	@for user in $(USERS); do \
	    echo "$$user" >> $(UTILISATEURS_FILE); \
	done
	@echo "Création des dossiers pour les utilisateurs..."
	@while IFS=, read -r id username password; do \
	    mkdir -p $(PLAYERS_DIR)/$$username; \
		mkdir -p $(PLAYERS_DIR)/$$username/games; \
	    echo "Bio de $$username" > $(PLAYERS_DIR)/$$username/bio; \
	    echo "Deuxième ligne de bio pour $$username" >> $(PLAYERS_DIR)/$$username/bio; \
	    touch $(PLAYERS_DIR)/$$username/friends; \
	    if [ "$$username" = "player1" ]; then \
	        echo "matches:10" > $(PLAYERS_DIR)/$$username/statistics; \
	        echo "wins:6" >> $(PLAYERS_DIR)/$$username/statistics; \
	        echo "losses:4" >> $(PLAYERS_DIR)/$$username/statistics; \
	    elif [ "$$username" = "player2" ]; then \
	        echo "matches:15" > $(PLAYERS_DIR)/$$username/statistics; \
	        echo "wins:10" >> $(PLAYERS_DIR)/$$username/statistics; \
	        echo "losses:5" >> $(PLAYERS_DIR)/$$username/statistics; \
	    elif [ "$$username" = "player3" ]; then \
	        echo "matches:5" > $(PLAYERS_DIR)/$$username/statistics; \
	        echo "wins:2" >> $(PLAYERS_DIR)/$$username/statistics; \
	        echo "losses:3" >> $(PLAYERS_DIR)/$$username/statistics; \
	    else \
	        echo "matches:0" > $(PLAYERS_DIR)/$$username/statistics; \
	        echo "wins:0" >> $(PLAYERS_DIR)/$$username/statistics; \
	        echo "losses:0" >> $(PLAYERS_DIR)/$$username/statistics; \
	    fi; \
	done < $(UTILISATEURS_FILE)
	@if [ -f $(PLAYERS_DIR)/bio ]; then \
	    rm -f $(PLAYERS_DIR)/bio; \
	fi
	@if [ -f $(PLAYERS_DIR)/friends ]; then \
	    rm -f $(PLAYERS_DIR)/friends; \
	fi
	@if [ -f $(PLAYERS_DIR)/statistics ]; then \
	    rm -f $(PLAYERS_DIR)/statistics; \
	fi
		@if [ -d $(PLAYERS_DIR)/games ]; then \
	    rm -rf $(PLAYERS_DIR)/games; \
	fi

	
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
.PHONY: all clean database