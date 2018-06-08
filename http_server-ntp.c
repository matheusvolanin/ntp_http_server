#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>
#include <time.h>
#include "ntp.h"

#define PORT 34321
#define BUFFER_SIZE 1024
#define SERVER_NAME "localhost"
#define NTP_PORT 123

char *serverPool[] = { "0.pool.ntp.org", "1.pool.ntp.org", "2.pool.ntp.org", "3.pool.ntp.org" };

int getLine(int socket, char *buf, int size)
{
	int i = 0;
	char c = '\0';
	int n;

	while((i < size - 1) && (c != '\n'))
	{
		n = recv(socket, &c, 1, 0);
		if (n > 0)
		{
			if (c == '\r')
			{
				n = recv(socket, &c, 1, MSG_PEEK);
				if ((n > 0) && (c == '\n'))
					recv(socket, &c, 1, 0);
				else
					c = '\n';
			}
			buf[i] = c;
			i++;
		}
		else
			c = '\n';
	}
	buf[i] = '\0';

	return i;
}

char **spaceSplitStr(char *str, int *nel)
{
	char **strArr = malloc(sizeof(char*));

	if (strlen(str) > 1 && str[strlen(str) - 1] == '\n')
	str[strlen(str) - 1] = ' ';

	char *token = strtok(str, " ");

	*nel = 0;

	while (token)
	{
		strArr = realloc(strArr, sizeof(char*) * ++(*nel));

		strArr[*nel - 1] = token;

		token = strtok(NULL, " ");
	}

	return strArr;
}

void htmlHeaders(int socket)
{
	char buf[BUFFER_SIZE];

	strcpy(buf, "HTTP/1.0 200 OK\r\n");
	send(socket, buf, strlen(buf), 0);
	strcpy(buf, "Server: http_server/1.0.0\r\n");
	send(socket, buf, strlen(buf), 0);
	sprintf(buf, "Content-Type: text/html\r\n");
	send(socket, buf, strlen(buf), 0);
	strcpy(buf, "\r\n");
	send(socket, buf, strlen(buf), 0);
}

void plainHeaders(int socket)
{
	char buf[BUFFER_SIZE];

	strcpy(buf, "HTTP/1.0 200 OK\r\n");
	send(socket, buf, strlen(buf), 0);
	strcpy(buf, "Server: http_server/1.0.0\r\n");
	send(socket, buf, strlen(buf), 0);
	sprintf(buf, "Content-Type: text/plain\r\n");
	send(socket, buf, strlen(buf), 0);
	strcpy(buf, "\r\n");
	send(socket, buf, strlen(buf), 0);
}

void notFoundHeaders(int socket)
{
	char buf[BUFFER_SIZE];

	strcpy(buf, "HTTP/1.0 404 Not Found\r\n");
	send(socket, buf, strlen(buf), 0);
	strcpy(buf, "Server: http_server/1.0.0\r\n");
	send(socket, buf, strlen(buf), 0);
	sprintf(buf, "Content-Type: text/html\r\n");
	send(socket, buf, strlen(buf), 0);
	strcpy(buf, "\r\n");
	send(socket, buf, strlen(buf), 0);
}

void badRequestHeaders(int socket)
{
	char buf[BUFFER_SIZE];

	strcpy(buf, "HTTP/1.0 400 Bad Request\r\n");
	send(socket, buf, strlen(buf), 0);
	strcpy(buf, "Server: http_server/1.0.0\r\n");
	send(socket, buf, strlen(buf), 0);
	sprintf(buf, "Content-Type: text/html\r\n");
	send(socket, buf, strlen(buf), 0);
	strcpy(buf, "\r\n");
	send(socket, buf, strlen(buf), 0);
}

void notImplementedHeaders(int socket)
{
	char buf[BUFFER_SIZE];

	strcpy(buf, "HTTP/1.0 501 Not Implemented\r\n");
	send(socket, buf, strlen(buf), 0);
	strcpy(buf, "Server: http_server/1.0.0\r\n");
	send(socket, buf, strlen(buf), 0);
	sprintf(buf, "Content-Type: text/html\r\n");
	send(socket, buf, strlen(buf), 0);
	strcpy(buf, "\r\n");
	send(socket, buf, strlen(buf), 0);
}

int getNtpTime(time_t *time)
{
	int ntp_socket, n, s = random() % 4;
	char *serverName = serverPool[s];
	ntp_packet packet = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

	memset(&packet, 0, sizeof(ntp_packet));

	*((char *)&packet + 0) = 0x1b; // mandrake

	struct sockaddr_in serv_addr;
	struct hostent *server;

	ntp_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (ntp_socket < 0)
	{
		printf(">>error getNtpTime: creating socket\n");
		return -1;
	}

	server = gethostbyname(serverName); // DNS -> 2 IP: defined in <netdb.h>
	if (server == NULL)
	{
		printf(">>error getNtpTime: getting hostByName\n");
		return -1;
	}

	bzero((char*)&serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;

	bcopy((char*)server->h_addr, (char*)&serv_addr.sin_addr.s_addr, server->h_length);
	serv_addr.sin_port = htons(NTP_PORT);

	if (connect(ntp_socket, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
	{
		printf(">>error getNtpTime: connecting to server\n");
		return -1;
	}

	n = write(ntp_socket, (char*)&packet, sizeof(ntp_packet));
	if (n < 0)
	{
		printf(">>error getNtpTime: sending ntp_packet\n");
		return -1;
	}

	n = read(ntp_socket, (char*)&packet, sizeof(ntp_packet));
	if (n < 0)
	{
		printf(">>error getNtpTime: reading from server\n");
		return -1;
	}

	packet.txTm_s = ntohl(packet.txTm_s);

	*time = (time_t)(packet.txTm_s - NTP_TIMESTAMP_DELTA); // the magic: https://www.eecis.udel.edu/~mills/y2k.html

	return s;
}


void processUrl(int socket, char *url)
{
	printf(">processUrl\n");
	char buf[BUFFER_SIZE];

	if (url != NULL && strcmp(url, "/ntpTime") == 0)
	{
		htmlHeaders(socket);

		sprintf(buf, "<html>\r\n\t<head>\r\n\t\t<title>Index of %s</title>\r\n\t</head>\r\n", url);
		send(socket, buf, strlen(buf), 0);

		strcpy(buf, "\t<body>\r\n");
		send(socket, buf, strlen(buf), 0);

		time_t localTime;
		time(&localTime);

		sprintf(buf, "\t<h3>LocalTime: %s</h3>\r\n", ctime(&localTime));
		send(socket, buf, strlen(buf), 0);

		time_t ntpTime;

		int s = getNtpTime(&ntpTime);
		if (s >= 0)
		{
			sprintf(buf, "\t<h3>NtpTime: %s</h3>\r\n", ctime(&ntpTime));
			send(socket, buf, strlen(buf), 0);

			sprintf(buf, "\t<h4>SKEW (ntp - local): %ld</h4>\r\n", ntpTime - localTime);
			send(socket, buf, strlen(buf), 0);

			struct in_addr server_addr;
			struct hostent *server_hostent = gethostbyname(serverPool[s]);
			server_addr.s_addr = *((uint32_t*) server_hostent->h_addr);

			sprintf(buf, "\t<h4>ServerName: %s, IP: %s</h4>\r\n", serverPool[s], inet_ntoa(server_addr));
			send(socket, buf, strlen(buf), 0);
		}
		else
		{
			strcpy(buf, "\t<h4>Erro ao recuperar informação do NTP server!</h4>\r\n");
			send(socket, buf, strlen(buf), 0);
		}

		strcpy(buf, "\t</body>\r\n</html>\r\n");
		send(socket, buf, strlen(buf), 0);

		printf("<processUrl\n");
		return;
	}

	char *basePath = "./files";
	struct stat pathStat;
	char *path;

	path = (char*)malloc(sizeof(char) * (strlen(basePath) + strlen(url) + 1));
	sprintf(path, "%s%s", basePath, url);

	if (stat(path, &pathStat) != 0)
	{
		printf(">>notFound\n");
		notFoundHeaders(socket);
		strcpy(buf, "<html>\r\n\t<head>\r\n\t\t<title>404 Not Found</title>\r\n\t</head>\r\n");
		send(socket, buf, strlen(buf), 0);
		strcpy(buf, "\t<body>\r\n\t\t<h1>404 Not Found</h1>\r\n\t</body>\r\n</html>\r\n");
		send(socket, buf, strlen(buf), 0);

		return;
	}
	
	if (S_ISDIR(pathStat.st_mode))
	{
		printf(">>directory\n");
		//dir, show files
		DIR *d = opendir(path);

		htmlHeaders(socket);

		sprintf(buf, "<html>\r\n\t<head>\r\n\t\t<title>Index of %s</title>\r\n\t</head>\r\n", url);
		send(socket, buf, strlen(buf), 0);
		sprintf(buf, "\t<body>\r\n\t\t<h1>Index of %s</h1>\r\n\t\t<ul>\r\n", url);
		send(socket, buf, strlen(buf), 0);

		while (1)
		{
			struct dirent *entry = readdir(d);
			if (!entry)
				break;

			if (!(strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0))
			{
				if (entry->d_type == DT_DIR)
					sprintf(buf, "\t\t\t<li><a href=\"%s%s/\">%s/</a></li>\r\n", url, entry->d_name, entry->d_name);
				else
					sprintf(buf, "\t\t\t<li><a href=\"%s%s\">%s</a></li>\r\n", url, entry->d_name, entry->d_name);

				send(socket, buf, strlen(buf), 0);
			}
		}

		sprintf(buf, "\t\t</ul>\r\n\t</body>\r\n</html>");
		send(socket, buf, strlen(buf), 0);
		
		closedir(d);
	}
	else if (S_ISREG(pathStat.st_mode))
	{
		printf(">>file\n");
		//file, send contents
		FILE *f = fopen(path, "r");

		if (f == NULL)
		{
			notFoundHeaders(socket);
			strcpy(buf, "<html>\r\n\t<head>\r\n\t\t<title>404 Not Found</title>\r\n\t</head>\r\n");
			send(socket, buf, strlen(buf), 0);
			strcpy(buf, "\t<body>\r\n\t\t<h1>404 Not Found</h1>\r\n\t</body>\r\n</html>\r\n");
			send(socket, buf, strlen(buf), 0);

			return;
		}

		plainHeaders(socket);

		char buf[BUFFER_SIZE];

		fgets(buf, sizeof(buf), f);
		while (!feof(f))
		{
			send(socket, buf, strlen(buf), 0);
			fgets(buf, sizeof(buf), f);
		}

		fclose(f);
	}
	else
	{
		//Bad Request
		badRequestHeaders(socket);
		strcpy(buf, "<html>\r\n\t<head>\r\n\t\t<title>400 Bad Request</title>\r\n\t</head>\r\n");
		send(socket, buf, strlen(buf), 0);
		strcpy(buf, "\t<body>\r\n\t\t<h1>400 Bad Request</h1>\r\n\t</body>\r\n</html>\r\n");
		send(socket, buf, strlen(buf), 0);
	}
	printf("<processUrl\n");
}

void handleClient(int client_socket)
{
	printf(">handleClient\n");
	char buffer[BUFFER_SIZE];
	char *method, *url, *httpVersion, **strArr = NULL;
	int eor = 0, strArrCount = 0;

	method = (char*)malloc(sizeof(char));
	url = (char*)malloc(sizeof(char));
	httpVersion = (char*)malloc(sizeof(char));

	method[0] = '\0';
	url[0] = '\0';
	httpVersion[0] = '\0';

	while (!eor)
	{
		int read = getLine(client_socket, buffer, BUFFER_SIZE);
		
		strArrCount = 0;
		strArr = spaceSplitStr(buffer, &strArrCount);

		if (strArr != NULL && strlen(strArr[0]) > 1)
		{
			if (strlen(method) == 0)
			{
				method = (char*)realloc(method, (strlen(strArr[0]) + 1)*sizeof(char));
				strcpy(method, strArr[0]);

				if (strArrCount > 1)
				{
					url = (char*)realloc(url, (strlen(strArr[1]) + 1)*sizeof(char));
					strcpy(url, strArr[1]);
				}

				if (strArrCount > 2)
				{
					httpVersion = (char*)realloc(httpVersion, (strlen(strArr[2]) + 1)*sizeof(char));
					strcpy(httpVersion, strArr[2]);
				}
			}

		}
		
		if (strlen(method) > 1 && read > 0 && buffer[0] == '\n')
		{
			printf("## Request ##\n\tMETHOD: %s\n\tVersion: %s\n\tURL: %s\n", method, httpVersion, url);
			eor = 1;
			if (strcmp(method, "GET") == 0)
			{
				processUrl(client_socket, url);
			}
			else if (strcmp(method, "HEAD") == 0)
			{
				notImplementedHeaders(client_socket);
			}
			else if (strcmp(method, "POST") == 0 || strcmp(method, "PUT") == 0)
			{
				notImplementedHeaders(client_socket);
			}
			else
			{
				badRequestHeaders(client_socket);
			}
		}
	}

	free(method);
	free(url);
	free(httpVersion);

	close(client_socket);
	printf("<handleClient\n");
}

int main(void)
{
	int server_socket, client_socket, error, nc = 0, reuse = 1; 
	struct sockaddr_in server_addr, client_addr;

	server_socket = socket(AF_INET, SOCK_STREAM, 0);
	if(server_socket < 0)
	{
		printf("Erro ao criar socket\n");
		return server_socket;
	}

	memset(&server_addr, 0, sizeof(server_addr)); // preciosismo

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(PORT);
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(reuse)); // evitar hangs TIME_WAIT

	error = bind(server_socket, (struct sockaddr *) &server_addr, sizeof(server_addr));
	if (error < 0)
	{
		printf("Erro ao efetuar bind\n");
		return error;
	}

	error = listen(server_socket, 128); // 128, do manual listen(2)
	if (error < 0)
	{
		printf("Erro ao efetuar listen\n");
		return error;
	}

	while(1)
	{
		socklen_t client_len = sizeof(client_addr);

		memset(&client_addr, 0, sizeof(client_addr)); // preciosismo

		client_socket = accept(server_socket, (struct sockaddr *) &client_addr, &client_len);
		if(client_socket >= 0)
		{
			printf("Nova conexao de: %s port %d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port)); // inet_ntoa, ntohs definida no <arpa/inet.h>

			handleClient(client_socket);
			
			printf("Conexao encerrada de: %s\n", inet_ntoa(client_addr.sin_addr));
			printf("Total conexões: %d\n\n", ++nc);
		}
	}

	close(server_socket); // definida em <unistd.h>

	return 0;
}
