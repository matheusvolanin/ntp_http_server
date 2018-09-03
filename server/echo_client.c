#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include "echo.h"

int main(int argc, char **argv)
{
	int server_socket, error; 
	struct sockaddr_in server_addr;
	char buffer[BUFFER_SIZE], msg[BUFFER_SIZE], server_name[100];

	sprintf(msg, "MENSAGEM PADRAO!");
	if(argc > 1)
	{
		sprintf(msg, "%s", argv[1]);
	}

	if (argc > 2)
	{
		sprintf(server_name, "%s", argv[2]);
	}
	else
	{
		sprintf(server_name, "%s", SERVER_NAME); // SERVER_NAME definido no "echo.h"
	}

	server_socket = socket(AF_INET, SOCK_STREAM, 0);
	if(server_socket < 0)
	{
		printf("Erro ao criar socket\n");
		return server_socket;
	}

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(PORT); // PORT definida no "echo.h"

	inet_pton(AF_INET, server_name, &server_addr.sin_addr);

	error = connect(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr));
	if (error < 0)
	{
		printf("Erro ao conectar ao servidor\n");
		return error;
	}

	send(server_socket, msg, strlen(msg), 0);

	int length;

	length = recv(server_socket, buffer, BUFFER_SIZE, 0);

	buffer[length] = '\0';
	printf("Recebido: '%s'\n", buffer);

	close(server_socket); // definida em <unistd.h>

	return 0;
}
