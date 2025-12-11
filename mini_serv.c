#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>

fd_set read_fds, write_fds, all_fds;
int count = 0, max_fd;
int ids[1000];
char* msgs[1000];
char buf_read[1001], buf_write[1001];

void error()
{
	write(2, "Fatal error\n", 12);
	exit(1);
}

void broadcast(int auth)
{
	for (int fd = 0; fd <= max_fd; fd++)
	{
		if (FD_ISSET(fd, &write_fds) && fd != auth)
			send(fd, buf_write, strlen(buf_write), 0);
	}
}

int extract_message(char **buf, char **msg)
{
	char	*newbuf;
	int	i;

	*msg = 0;
	if (*buf == 0)
		return (0);
	i = 0;
	while ((*buf)[i])
	{
		if ((*buf)[i] == '\n')
		{
			newbuf = calloc(1, sizeof(*newbuf) * (strlen(*buf + i + 1) + 1));
			if (newbuf == 0)
				return (-1);
			strcpy(newbuf, *buf + i + 1);
			*msg = *buf;
			(*msg)[i + 1] = 0;
			*buf = newbuf;
			return (1);
		}
		i++;
	}
	return (0);
}

char *str_join(char *buf, char *add)
{
	char	*newbuf;
	int		len;

	if (buf == 0)
		len = 0;
	else
		len = strlen(buf);
	newbuf = malloc(sizeof(*newbuf) * (len + strlen(add) + 1));
	if (newbuf == 0)
		return (0);
	newbuf[0] = 0;
	if (buf != 0)
		strcat(newbuf, buf);
	free(buf);
	strcat(newbuf, add);
	return (newbuf);
}


int main(int argc, char **argv) {
	int sockfd, connfd;
	struct sockaddr_in servaddr, cli; 

	if (argc != 2)
	{
		write(2, "Wrong number of arguments", 26);
		exit(1);
	}

	// socket create and verification 
	sockfd = socket(AF_INET, SOCK_STREAM, 0); 
	if (sockfd == -1)
		error(); 
	bzero(&servaddr, sizeof(servaddr)); 

	// assign IP, PORT 
	servaddr.sin_family = AF_INET; 
	servaddr.sin_addr.s_addr = htonl(2130706433); //127.0.0.1
	servaddr.sin_port = htons(atoi(argv[1])); 

	// Binding newly created socket to given IP and verification 
	if ((bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr))) != 0)
		error(); 
	if (listen(sockfd, 10) != 0)
		error(); 

	max_fd = sockfd;
	FD_ZERO(&all_fds);
	FD_SET(sockfd, &all_fds);
	while(1)
	{
		read_fds = write_fds = all_fds;
		if (select(max_fd + 1, &read_fds, &write_fds, 0, 0) < 0)
			error();
		for (int fd = 0; fd <= max_fd; fd++)
		{
			if (FD_ISSET(fd, &read_fds))
			{
				if (fd == sockfd) //new client
				{
					socklen_t len = sizeof(cli);
					connfd = accept(sockfd, (struct sockaddr *)&cli, &len);
					if (connfd < 0)
						error();
					if (connfd > max_fd)
						max_fd = connfd;
					FD_SET(connfd, &all_fds);
					ids[connfd] = count++;
					msgs[connfd] = 0;
					sprintf(buf_write, "server: client %d just arrived\n", ids[connfd]);
					broadcast(connfd);
				}
				else //existing client
				{
					int bytes_read = recv(fd, buf_read, 1000, 0);
					if (bytes_read <= 0) //disconnect
					{
						FD_CLR(fd, &all_fds);
						free(msgs[fd]);
						close(fd);
						sprintf(buf_write, "server: client %d just left\n", ids[fd]);
						broadcast(fd);
					}
					else //incoming message
					{
						char *msg;
						buf_read[bytes_read] = 0;
						msgs[fd] = str_join(msgs[fd], buf_read);
						while(extract_message(&msgs[fd], &msg))
						{
							sprintf(buf_write, "client %d: %s", ids[fd], msg);
							broadcast(fd);
							free(msg);
						}
					}
				}
			}
		}
	}
}