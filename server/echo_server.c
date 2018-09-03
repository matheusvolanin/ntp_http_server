#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "echo.h"

int main(void)
{
	int server_socket, client_socket, error; 
	struct sockaddr_in server_addr, client_addr;
	char buffer[BUFFER_SIZE];

	server_socket = socket(AF_INET, SOCK_STREAM, 0);
	if(server_socket < 0)
	{
		printf("Erro ao criar socket\n");
		return server_socket;
	}

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(PORT); // PORT definida no "echo.h"
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	error = bind(server_socket, (struct sockaddr *) &server_addr, sizeof(server_addr));
	//TODO: error handling, error < 0

	error = listen(server_socket, 128); // 128, do manual listen(2)
	//TODO: error handling, error < 0

	while(1)
	{
		socklen_t client_len = sizeof(client_addr);
		client_socket = accept(server_socket, (struct sockaddr *) &client_addr, &client_len);

		printf("Nova conexao de: %s\n", inet_ntoa(client_addr.sin_addr)); // inet_ntoa definida no <arpa/inet.h>

		while (1)
		{
			int read = recv(client_socket, buffer, BUFFER_SIZE, 0); // BUFFER_SIZE definido no "echo.h"
			if (!read) break; // parar o laco interno
			//TODO: error handling, read < 0

			error = send(client_socket, buffer, read, 0);
			//TODO: error handling, error < 0
		}

		printf("Conexao encerrada de: %s\n\n", inet_ntoa(client_addr.sin_addr));
	}

	close(server_socket); // definida em <unistd.h>

	return 0;
}
