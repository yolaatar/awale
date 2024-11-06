#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#define PORT 8080

int main(int argc, char const* argv[])
{
	int sock = 0, valread, client_fd;
	struct sockaddr_in serv_addr;      /* nom de la socket d'entrée dans ce programme */

	char* hello = "Hello from client"; /* message à envoyer */
	char buffer[1024] = { 0 };         /* initialisation à un string vide */

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		printf("\n Socket creation error \n");
		return -1;
	}

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(PORT);

	// Convertion  d'adresse IPv4 et IPv6 à partir d'un text
	// Il faut tout faire "à la main" seul des char sont transférés sur le réseau
        // L'adresse est une adresse locale - 127.0.0.1 est le "localhost"
	if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr)
		<= 0) {
		printf(
			"\nInvalid address/ Address not supported \n");
		return -1;
	}

        /* Connexion de la socket */
	if ((client_fd
		= connect(sock, (struct sockaddr*)&serv_addr,
				sizeof(serv_addr)))
		< 0) {
		printf("\nConnection Failed \n");
		return -1;
	}
	send(sock, hello, strlen(hello), 0);
	printf("Hello message sent to server\n");
	valread = read(sock, buffer, 1024);
	printf("%s\n", buffer);

	// fermeture de la socket
	close(client_fd);
	return 0;
}

