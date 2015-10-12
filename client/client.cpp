/*
 * CREATED BY: Artem Golotin
 * CS360
 */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string>
#include <cstring>
#include <iostream>
#include <cstdio>
#include <stdlib.h>
#include <vector>             
#include <fcntl.h>
#include <unistd.h>

using namespace std;

#define SOCKET_ERROR -1
#define HOST_NAME_SIZE 255
#define MAX_MSG_SZ 1024


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
			perror("Socket Error is: ");
			fprintf(stderr, "Read failed on file descriptor %d messagesize = %d\n", hSocket, messagesize);
			exit(2);
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
	headerLines.clear();

	char* line = getLine(socket);
	while (strlen(line) != 0)
	{
		headerLines.push_back(line);
		line = getLine(socket);
	}
}

void printDebug(string request, vector<string> &headerLines)
{
	printf(string(request + '\n').c_str());
	for (int i = 0; i < headerLines.size(); i++)
		printf(string(headerLines[i] + '\n').c_str());
	printf("\n");
}

void printOutput(int socket, char buffer[MAX_MSG_SZ], vector<string> headerLines)
{
	int rval;

	while((rval = read(socket,buffer,MAX_MSG_SZ)) > 0) 
	{
		write(1, buffer, rval);
		if (strstr(headerLines[headerLines.size()-1].c_str(), "keep-alive"))
			break;
	}
}

int main(int argc, char* argv[])
{
	int hSocket; 					// handler for the socket
	struct hostent* pHostInfo; 		// holds info about th machine
	struct sockaddr_in Address; 	// Internet socket address struct
	long nHostAddress;
	char buffer[MAX_MSG_SZ];
	char strHostName[HOST_NAME_SIZE];
	int nHostPort;
	vector<string> headerLines;
	int success_count = 0;


	//=================COMMAND LINE PARAMS=================== ||
	extern char* optarg;
	int c, times_to_download = 1, err = 0;
	bool debug = false;
	while ((c = getopt(argc, argv, "c:d")) != -1)
	{
		switch(c)
		{
			case 'c':
				if (atoi(optarg) == 0)
					err = 1;
				else
					times_to_download = atoi(optarg); 
				break;
			case 'd':
				debug = true;
				break;
			default:
				err = 1;
				break;
		}
	}
	//===================USAGE=============================== ||
	if (argc < 4 || err == 1)
	{
		perror("ERROR: Usage: ./download [(-c count) || (-d)] host_name port_num PATH/TO/web_page.html");
		exit(1);
	}

	string page = argv[optind+2];
	strcpy(strHostName,argv[optind]);
	nHostPort = atoi(argv[optind+1]);

	//===============CREATING GET REQUEST==================== ||
	string _request = "GET " + page + " HTTP/1.1\r\n" +
		"Host: " + strHostName + "\r\n" +
		"Accept: */*\r\n" +
		"Content-Type: */*\r\n" +
		"Content-Length: 0\r\n\r\n";

	for (int i = 0; i < times_to_download; i++)
	{
		hSocket=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
		if (hSocket == SOCKET_ERROR)
		{
			perror("\nError occured while creating a socket\n");
			exit(1);
		}

		//===============GETTING THE HOST IP================== ||
		pHostInfo = gethostbyname(strHostName);
		if (pHostInfo == NULL)
		{
			perror("Error occured while getting host by name");
			exit(1);
		}
		memcpy(&nHostAddress, pHostInfo->h_addr, pHostInfo->h_length);

		//==============DEFINING host Address struct========== ||
		Address.sin_addr.s_addr = nHostAddress;
		Address.sin_port = htons(nHostPort);
		Address.sin_family = AF_INET;

		//=============CONNECTING TO SOCKET=================== ||
		if (connect(hSocket, (struct sockaddr*)&Address, sizeof(Address))
				== SOCKET_ERROR)
		{
			perror("\nERROR: Could not connect to host\n");
			exit(1);
		}

		//============SENDING GET REQUEST==================== ||
		if (write(hSocket, _request.c_str(), strlen(_request.c_str())) < 0) 
		{
			perror("\nERROR: GET request didn't return any data\n");
			exit(1);
		}

		//===============PARSING THE REQUEST OUTPUT================ ||
		getHeaderLines(headerLines, hSocket);
		if (strstr(headerLines[0].c_str(), "OK"))
			success_count++;

		//=====================COMMAND LINE OPTIONS CHECK========== ||
		if (debug)
		{
			printDebug(_request, headerLines);
			printOutput(hSocket, buffer, headerLines);
		}
		else if (times_to_download == 1)
		{
			printOutput(hSocket, buffer, headerLines);
		}
		else 
		{
			int rlen = read(hSocket,buffer,MAX_MSG_SZ);
			while (rlen != 0) 
			{
				if (rlen == MAX_MSG_SZ)
					rlen = read(hSocket,buffer,MAX_MSG_SZ);
				else 
					rlen = 0;
			}
				
		}
		//=====END PARSING REQUEST AND CLOSE THE SOCKET=========== ||
		close(hSocket);
	}

	//=====================TIMES TO DOWNLOAD CHECK========== ||
	if (times_to_download > 1)
	{
		printf("The file has been downloaded successfully %d times out of %d\n", success_count, times_to_download);
	}

	return 0;
}
