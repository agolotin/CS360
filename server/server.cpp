/*
 * Created by: Artem Golotin
 * CS360
 */
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <pthread.h>
#include <semaphore.h>
#include <dirent.h>
#include <vector>
#include <queue>

using namespace std;

#define SOCKET_ERROR	-1
#define SERVER_ERROR	-1
#define MAX_MSG_SZ	1024
#define QUEUE_SIZE	100
#define OK "200 OK"

int hSocket,hServerSocket;  /* handle to socket */
struct hostent* pHostInfo;   /* holds info about a machine */
struct sockaddr_in Address; /* Internet socket address stuct */
int nAddressSize=sizeof(struct sockaddr_in);
char pBuffer[MAX_MSG_SZ];
int nHostPort;
int THREADS = 10;
string dir;
int error_counter;
bool absolutePath = false;

sem_t e;
sem_t s;
sem_t n;
queue<int> work_load;

bool isWhitespace(char c)
{
	switch (c)
	{
		case '\r':
		case '\n': 
		case ' ': 
		case '\0': 
			return true; 
		default: 
			return false;
	}
}

void chomp(char *line)
{
	int len = strlen(line);
	while (isWhitespace(line[len]))
		line[len--] = '\0';
}

char* getLine(int hSocket)
{
	char tline[MAX_MSG_SZ];
	char* line;

	int messagesize = 0;
	int amtread = 0;
	while ((amtread = read(hSocket, tline+messagesize, 1)) < MAX_MSG_SZ)
	{
		if (amtread > 0)
			messagesize += amtread;
		else 
		{
		//	perror("Socket Error is: ");
			//fprintf(stderr, "Read failed on file descriptor %d messagesize = %d\n", hSocket, messagesize);
			//exit(2);
			error_counter++;
			return NULL;
		}
		if (tline[messagesize-1] == '\n')
			break;
	}
	tline[messagesize] = '\0';
	chomp(tline);
	line = (char *)malloc((strlen(tline)+1)*sizeof(char));
	strcpy(line, tline);
	return line;
}

void getHeaderLines(vector<string> &headerLines, int socket)
{
	char* line = getLine(socket);
	while (strlen(line) != 0)
	{
		headerLines.push_back(line);
		line = getLine(socket);
	}
}

string generalHeaderResponse(string status, string cont_type, string cont_len)
{
	string response = "HTTP/1.1 " + status + "\r\n" +
		"Content-Type: " + cont_type + "\r\n" +
		"Content-Length: " + cont_len + "\r\n\r\n";
	return response;
}

void sendResponse(int socket, string header, string filename, bool isfile)
{
	strcpy(pBuffer, header.c_str());
	send(socket, pBuffer, strlen(pBuffer), 0);
	int buffer_len = 1;
	if (isfile)
	{
		unsigned int file = open(filename.c_str(), O_RDONLY, S_IREAD);
		while (buffer_len > 0)
		{
			buffer_len = read(file, pBuffer, MAX_MSG_SZ);
			if (buffer_len > 0)
				send(socket, pBuffer, buffer_len, 0);
		}
	}
	else
	{
		strcpy(pBuffer, filename.c_str());
		write(socket, pBuffer, strlen(pBuffer)+1);
	}
}

string buildDirectoryHTML(vector<string> names)
{
	string html = "<!DOCTYPE html>\n<html>\n\t<body>\n";
	for (int i = 0; i < names.size(); i++)
		html += "\t\t<a href=\"" + names[i] + "\">" + names[i] + "</a><br />\n";
	html += "\t</body>\n</head>\n";

	return html;
}

void* serve(void* thread_id)
{
	while (work_load.size() != -1)
	{
		sem_wait(&n); // wait until there's a request
		sem_wait(&s);

		int socket = work_load.front();
		work_load.pop();
		vector<string> headerLines;

		getHeaderLines(headerLines, socket);
		struct stat filestat;
		string tempfn = headerLines[0];
		string filename = "";
		for (int i = 0; i < tempfn.length(); i++)
		{
			if (i > 3)
			{
				if (isspace(tempfn[i]))
					break;
				else
					filename += tempfn[i];
			}
		}
		string temp = dir;
		temp.append(filename);
		if (absolutePath)
			filename = temp;
		else
			filename = "." + temp;

		if ((stat(filename.c_str(), &filestat) == -1)) /* Case 404 Not Found */
		{
			struct stat st;
			stat("404.html", &st);
			char size[256];
			sprintf(size, "%d", st.st_size); 

			string header = generalHeaderResponse("404 Not Found", "text/html", size);
			/* Sending the response */
			sendResponse(socket, header, "404.html", true);
		}
		if (S_ISREG(filestat.st_mode))  /* Case 200 OK; file */
		{
			char size[256];
			sprintf(size, "%d", filestat.st_size); 
			string header;

			if (strstr(filename.c_str(), ".txt"))
				header = generalHeaderResponse(OK, "text/plain", size);
			else if (strstr(filename.c_str(), ".html"))
				header = generalHeaderResponse(OK, "text/html", size);
			else if (strstr(filename.c_str(), ".jpg"))
				header = generalHeaderResponse(OK, "image/jpg", size);
			else if (strstr(filename.c_str(), ".gif"))
				header = generalHeaderResponse(OK, "image/gif", size);
			/* Sending the response */
			sendResponse(socket, header, filename, true);
		}
		if (S_ISDIR(filestat.st_mode))  /* Case 200 OK; directory */
		{
			DIR *dirp;
			struct dirent *dp;
			bool sent = false;
			vector<string> dir_names;

			string header;
			dirp = opendir(filename.c_str());
			while ((dp = readdir(dirp)) != NULL)
			{
				if (strstr(dp->d_name, "index.html"))
				{
					filename = filename + "/index.html";
					struct stat st;
					stat(filename.c_str(), &st);
					char size[256];
					sprintf(size, "%d", st.st_size); 

					header = generalHeaderResponse(OK, "text/html", size);
					/* Sending the response */
					sendResponse(socket, header, filename, true);
					sent = true;
					break;
				}
				else 
					dir_names.push_back(dp->d_name);
			}
			(void)closedir(dirp);

			if (!sent)
			{
				string dirHTML = buildDirectoryHTML(dir_names);
				char size[256];
				sprintf(size, "%d", dirHTML.length()); 

				header = generalHeaderResponse(OK, "text/html", size);
				/* Sending the response */
				sendResponse(socket, header, dirHTML, false);
			}
		}

		close(socket);
		sem_post(&s);
		sem_post(&e);
	}
}

int main(int argc, char* argv[])
{
	nHostPort = atoi(argv[1]);
	THREADS = atoi(argv[2]);
	dir = argv[3];
	if (strstr(dir.c_str(), "/users/"))
		absolutePath = true;

	hServerSocket = socket(AF_INET,SOCK_STREAM,0);
	if (hServerSocket == SERVER_ERROR)
	{
		printf("\nERROR: Could not connect to host\n");
		return 0;
	}

	Address.sin_addr.s_addr = INADDR_ANY;
	Address.sin_port = htons(nHostPort);
	Address.sin_family = AF_INET;

	if (bind(hServerSocket, (struct sockaddr*)&Address, sizeof(Address)) 
			== SOCKET_ERROR)
	{
		printf("ERROR: Could not bind to port\n");
		return 0;
	}
	getsockname(hServerSocket, (struct sockaddr*)&Address, (socklen_t*)&nAddressSize);

	if (listen(hServerSocket, QUEUE_SIZE) == SOCKET_ERROR)
	{
		printf("\nERROR: Could not create a listening queue\n");
		return 0;
	}
	int optval = 1;
	setsockopt(hServerSocket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)); // allows several requests on a port at the same time

	/* Initializing threads */
	sem_init(&s, 0, 1); // working semaphore
	sem_init(&e, 0, 1000); // allowed connections
	sem_init(&n, 0, 0); // thing that tells the thread to be able to work
	pthread_t threads[THREADS];
	for (int i = 0; i < THREADS; i++)
		pthread_create(&threads[i], NULL, serve, &i);
	/* End initializing threads */

	/* Processing requests */
	printf("The server is up and running\n");
	for (;;)
	{
		hSocket = accept(hServerSocket, (struct sockaddr*)&Address, (socklen_t*)&nAddressSize);
		if (hSocket < 0)
		{
			printf("ERROR: Could not accept connection\n");
		}
		else 
		{
			sem_wait(&e);
			sem_wait(&s);
			work_load.push(hSocket);
			sem_post(&s);
			sem_post(&n);
		}
	}
	close(hServerSocket);
}
