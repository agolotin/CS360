/*
 * Created by: Artem Golotin
 * CS360
 */
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <sys/wait.h>
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
string request_method;
string query_string;
int content_len = 0;
bool envformat = false; // takes care of CGI
int error_counter;

sem_t e;
sem_t s;
sem_t n;
queue<int> work_load;

// Splits char* string to a char* string[]
char** str_split(char* a_str, const char a_delim)
{
	char** result    = 0;
	size_t count     = 0;
	char* tmp        = a_str;
	char* last_delim = 0;
	char delim[2];
	delim[0] = a_delim;
	delim[1] = 0;

	/* Count how many elements will be extracted. */
	while (*tmp)
	{
		if (a_delim == *tmp)
		{
			count++;
			last_delim = tmp;
		}
		tmp++;
	}

	/* Add space for trailing token. */
	count += last_delim < (a_str + strlen(a_str) - 1);
	/* Add space for terminating null string so caller
	   knows where the list of returned strings ends. */
	count++;

	result = (char**)malloc(sizeof(char*) * count);
	
	if (result)
	{
		size_t idx  = 0;
		char* token = strtok(a_str, delim);

		while (token)
		{
			*(result + idx++) = strdup(token);
			token = strtok(0, delim);
		}
		*(result + idx) = 0;
	}
	return result;
}

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

// Specific for CGI scripts
void upcaseAndReplaceDashWithUnderline(char *str)
{
	char *s = str;

	for (int i = 0; s[i] != ':'; i++)
	{
		if (s[i] >= 'a' && s[i] <= 'z')
			s[i] = 'A' + (s[i] - 'a');
		if (s[i] == '-')
			s[i] = '_';
	}
}

// Specific for CGI scripts
char *formatHeader(char *str, char *prefix)
{
	char *result = (char*)malloc(strlen(str)+strlen(prefix));
	char *value = strchr(str,':')+2;
	upcaseAndReplaceDashWithUnderline(str);
	*(strchr(str,':')) = '\0';
	sprintf(result, "%s%s=%s", prefix, str, value);
	return result;
} 

// Specific for CGI scripts
char** processEnvVariables(vector<string> headers, string filename)
{
	char* absolute_path = (char*)malloc((strlen(filename.c_str()) + 1)*sizeof(char));
	realpath(filename.c_str(), absolute_path); // creates an absoute path to the file

	char** result = 0;
	result = (char**)malloc(sizeof(char*) * (headers.size() + 5));

	int i;
	for (i = 0; i < headers.size(); i++)
		*(result + i) = (char*)headers[i].c_str();

	*(result + i++) = "GATEWAY_INTERFACE=CGI/1.1";

	char* tempstr = (char*)malloc((strlen(absolute_path) + 15)*sizeof(char));
	sprintf(tempstr, "REQUEST_URI=%s", absolute_path);
	*(result + i++) = strdup(tempstr);
	free(tempstr);

	tempstr = (char*)malloc((strlen(request_method.c_str()) + 18)*sizeof(char));
	sprintf(tempstr, "REQUEST_METHOD=%s", request_method.c_str());
	*(result + i++) = strdup(tempstr);
	free(tempstr);

	if (query_string != "")
	{
		tempstr = (char*)malloc((strlen(query_string.c_str()) + 15)*sizeof(char));
		sprintf(tempstr, "QUERY_STRING=%s", query_string.c_str());
		*(result + i++) = strdup(tempstr);
		free(tempstr);
	}
	else 
		*(result + i++) = "QUERY_STRING=";

	*(result + i++) = (char*)NULL;

	return result;
}

// Specific for CGI scripts
string cgiHeaderResponse()
{
	string response = "HTTP/1.0 200 OK\r\nMIME-Version:1.0\r\n\r\n";
	return response;
}

int parseContentLen(char* line)
{
	char** tokens = str_split(line, '=');
	content_len = atoi(*(tokens+1));
	printf("Cont_len: %d\n", content_len);
}

// Specific for CGI scripts
void processCGI(int socket, string filename, vector<string> headers)
{
	char** env;
	char** arg;
	int fd1[2]; // serve to cgi pipe
	int fd2[2]; // cgi to serve pipe
	pipe(fd1);
	pipe(fd2);

	pid_t pid = fork();
	if (pid == 0) // child
	{
		env = processEnvVariables(headers, filename.c_str());
		if (query_string != "")
			arg = str_split((char*)query_string.c_str(), '='); 
		else 
			arg = NULL;

		close(fd1[1]); 				 // close the write side of the pipe from the server
		dup2(fd1[0], STDIN_FILENO);  // dup the pipe to stdin
		close(fd2[0]);   			 // close the read side of the pipe to the server
		dup2(fd2[1], STDOUT_FILENO); // dup the pipe to stdout

		execve(filename.c_str(), arg, env);
	}
	else if (pid > 0) //parent
	{
		int stat;
		int err;

		close(fd1[0]); // close the read side of the pipe to the CGI script
		close(fd2[1]); // close the write side of the pipe from the CGI script

		// Handling POST request
		if (strstr(request_method.c_str(), "POST"))
		{
			int amtread = 0;
			int amt = 0;
			while ((amtread < content_len) && (amt = read(socket, pBuffer, MAX_MSG_SZ)))
			{
				amtread += amt;
				write(fd1[1], pBuffer, amt);
			}
		}
		//Wait for the child to respond
		err = wait(&stat);
		
		/* Sending response */
		string header = cgiHeaderResponse();
		strcpy(pBuffer, header.c_str());
		write(socket, pBuffer, strlen(pBuffer));

		int buffer_len = 1;
		while (buffer_len > 0)
		{
			buffer_len = read(fd2[0], pBuffer, MAX_MSG_SZ);
			if (buffer_len > 0)
				write(socket, pBuffer, buffer_len);
		}
		/* End sending response */
		
		close(fd1[0]);
		close(fd2[1]);
	}
	printf("Harvesting zombies after CGI performed\n");
	//Harvest zombie children
	if (fd1[0] != -1)
		close(fd1[0]);
	if (fd1[1] != -1)
		close(fd1[1]);
	if (fd2[0] != -1)
		close(fd2[0]);
	if (fd2[1] != -1)
		close(fd2[1]);
	if (pid != (pid_t) -1)
		kill(pid, SIGKILL);
}

void getHeaderLines(vector<string> &headerLines, int socket, bool envformat)
{
	char *line;
	char *tline = getLine(socket);

	while (strlen(tline) != 0)
	{
		if (strstr(tline, "Content-Length") ||
			strstr(tline, "Content-Type"))
		{
			if (envformat)
			{
				line = formatHeader(tline, "");
				if (strstr(request_method.c_str(), "POST") &&
					strstr(line, "LENGTH"))
					parseContentLen(strdup(line));
			}
			else
			{
				line = (char*)malloc((strlen(tline)+10) * sizeof(char));
				strcpy(line, tline);
			}
		}
		else
		{
			if (envformat)
				line = formatHeader(tline, "HTTP_");
			else 
			{
				line = (char*)malloc((strlen(tline)+10) * sizeof(char));
				strcpy(line, tline);
			}
		}
		headerLines.push_back(line);
		//printf("Header line: %s\n", line);
		free(tline);
		tline = getLine(socket);
	}
	free(tline);
}

string generalHeaderResponse(string status, string cont_type, string cont_len)
{
	string response = "HTTP/1.0 " + status + "\r\n" +
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

string parseFirstLine(int socket)
{
	char* tempfn = getLine(socket);

	char** tokens = str_split(tempfn, ' ');
	request_method = *(tokens);

	char** url = str_split(*(tokens+1), '?');
	string filename = *(url);
	if (*(url+1) != NULL)
		query_string = *(url+1);
	else 
		query_string = "";

	string temp = dir;
	temp.append(filename);
	filename = "." + temp;
	
	envformat = false;
	if (strstr(filename.c_str(), ".cgi") || 
		strstr(filename.c_str(), ".pl"))
		envformat = true;

	return filename;
}

void* serve(void* thread_id)
{
	while (work_load.size() != -1)
	{
		sem_wait(&n); // wait until there's a request
		sem_wait(&s);
		int socket = work_load.front();
		work_load.pop();

		struct stat filestat;
		string filename = parseFirstLine(socket);

		//After processing the first line, read the rest of the headers
		//knowing whether or not it's a cgi script
		vector<string> headerLines;
		getHeaderLines(headerLines, socket, envformat);

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

			if (envformat)
				processCGI(socket, filename, headerLines);
			else 
			{
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
		printf("Successfully completed the request\n");

		sem_post(&s);
		sem_post(&e); // release the semaphores
	}
}

int main(int argc, char* argv[])
{
	nHostPort = atoi(argv[1]);
	THREADS = atoi(argv[2]);
	dir = argv[3];

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
	sem_init(&s, 0, 1); 	// working semaphore
	sem_init(&e, 0, 1000);  // allowed connections
	sem_init(&n, 0, 0); 	// thing that tells the thread to be able to work
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
