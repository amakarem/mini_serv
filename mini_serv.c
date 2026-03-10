#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAX_MSG_SIZE 1000000

typedef struct s_client
{
	int	id;
	char	msg[MAX_MSG_SIZE];
} t_client;

t_client clients[1024];
fd_set active, read_set, write_set;
int max_fd, next_id;

int error_msg(char * msg)
{
	fprintf(stderr, "%s\n", msg);
	return (1);
}

void brodcast(int sender, char *msg)
{
	for(int fd = 0; fd <= max_fd; fd++)
		if (FD_ISSET(fd, &write_set) && fd != sender)
			send(fd, msg, strlen(msg), 0);
}

int main(int argc, char **argv)
{
	if (argc != 2)
		return (error_msg("Wrong number of arguments"));
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0)
		return (error_msg("Fatal error"));
	max_fd = sockfd;
	FD_ZERO(&active);
	FD_SET(sockfd, &active);
	struct sockaddr_in serv;
	memset(&serv, 0, sizeof(serv));
	serv.sin_family = AF_INET;
	serv.sin_addr.s_addr = inet_addr("127.0.0.1");
	serv.sin_port = htons(atoi(argv[1]));

	if (bind(sockfd, (struct sockaddr *)&serv, sizeof(serv)) < 0)
		return (error_msg("Fatal error"));
	if (listen(sockfd, 10) < 0)
		return (error_msg("Fatal error"));

	while (1)
	{
		read_set = write_set = active;
		if (select(max_fd + 1, &read_set, &write_set, NULL, NULL) < 0)
			continue;
		for (int fd = 0; fd <= max_fd; fd++)
		{
			if (!FD_ISSET(fd, &read_set))
				continue;
			if (fd == sockfd)
			{
				int client = accept(sockfd, NULL, NULL);
				if (client < 0)
					continue;
				if (client > max_fd)
					max_fd = client;
				clients[client].id = next_id++;
				clients[client].msg[0] = 0;
				FD_SET(client, &active);
				char msg[100];
				snprintf(msg, sizeof(msg), "server: client %d just arrived\n", clients[client].id);
				brodcast(client, msg);
			}
			else
			{
				char buf[MAX_MSG_SIZE];
				int n = recv(fd, buf, sizeof(buf), 0);
				if (n <= 0)
				{
					char msg[100];
					snprintf(msg, sizeof(msg), "server: client %d just left\n", clients[fd].id);
					brodcast(fd, msg);
					FD_CLR(fd, &active);
					close(fd);
				}
				else
				{
					buf[n] = 0;
					strcat(clients[fd].msg, buf);

					char *line;
					while ((line = strchr(clients[fd].msg, '\n')))
					{
						*line = 0;
						char msg[MAX_MSG_SIZE + 21];
						snprintf(msg, sizeof(msg), "client %d: %s\n", clients[fd].id, clients[fd].msg);
						brodcast(fd, msg);
						memmove(clients[fd].msg, line + 1, strlen(line + 1) + 1);
					}
				}
			}
		}
	}
	return (0);
}