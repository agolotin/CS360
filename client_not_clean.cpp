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
#define MAX_MSSG_SIZE 1024

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
	char tline[MAX_MSSG_SIZE];
	char* line;

	int messagesize = 0;
	int amtread = 0;
	while ((amtread = read(hSocket, tline+messagesize, 1)) < MAX_MSSG_SIZE)
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

//	char* line;
//	char* tline;
//
//	tline = getLine(socket);
//	while(strlen(tline) != 0)
//	{
//		if (strstr(tline, "Content-Length") ||
//				strstr(tline, "Content-Type"))
//		{
//			line = strdup(tline);
//		}
//		else 
//		{
//			line = (char*)malloc((strlen(tline)+10)*sizeof(char));
//			sprintf(line, "HTTP_%s", tline);
//		}
//
//		headerLines.push_back(line);
//		free(tline);
//		tline = getLine(socket);
//	}
//	free(tline);
}

void printDebug(string request, vector<string> &headerLines)
{
	printf(request.c_str());
	printf("\n");
	for (int i = 0; i < headerLines.size(); i++)
		printf(string(headerLines[i] + '\n').c_str());
	printf("\n");
}

int main(int argc, char* argv[])
{
	int hSocket; 					// handler for the socket
	struct hostent* pHostInfo; 		// holds info about th machine
	struct sockaddr_in Address; 	// Internet socket address struct
	long nHostAddress;
	char pBuffer[MAX_MSSG_SIZE];
	unsigned nReadAmount;
	char strHostName[HOST_NAME_SIZE];
	int nHostPort;
	vector<string> headerLines;


	//=================COMMAND LINE PARAMS=================== ||
//	printf("\nGetting parameters from the command line\n");
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
	if (argc < 4 || err == 1) //|| *argv[1] == '-')
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
		"Content-Type: text/html\r\n" +
		"Content-Length: 0\r\n\r\n";

	for (int i = 0; i < times_to_download; i++)
	{
//		printf("Making a socket\n");
		hSocket=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
		if (hSocket == SOCKET_ERROR)
		{
			perror("\nError occured while creating a socket\n");
			exit(1);
		}

		//===============GETTING THE HOST IP================== ||
		pHostInfo = gethostbyname(strHostName);
		memcpy(&nHostAddress, pHostInfo->h_addr, pHostInfo->h_length);

		//==============DEFINING host Address struct========== ||
		Address.sin_addr.s_addr = nHostAddress;
		Address.sin_port = htons(nHostPort);
		Address.sin_family = AF_INET;

		//=============CONNECTING TO SOCKET=================== ||
		if (connect(hSocket, (struct sockaddr*)&Address, sizeof(Address))
				== SOCKET_ERROR)
		{
			perror("\nCould not connect to host\n");
			exit(1);
		}
//		printf("Connection to socket has been established\n");

		//============SENDING GET REQUEST==================== ||
//		printf("\nSending GET request\n");
		if (write(hSocket, _request.c_str(), strlen(_request.c_str())) < 0) //writing() GET request
		{
			perror("\nGET request didn't return any data\n");
			exit(1);
		}
//		printf("GET request sent successfully\n");

		//===============PARSING THE REQUEST OUTPUT================ ||
		getHeaderLines(headerLines, hSocket);

		while((nReadAmount = read(hSocket, pBuffer, MAX_MSSG_SIZE) > 0))
				write(1,pBuffer,nReadAmount);

		//================CLOSE THE SOCKET===================== ||
		close(hSocket);
	}

	//=====================COMMAND LINE OPTIONS CHECK========== ||
	if (debug)
	{
		printDebug(_request, headerLines);
		printf(pBuffer);
	}
	else if (times_to_download > 1)
	{
		printf("\nThe file had been downloaded %d times successfully\n", times_to_download);
	}
	else
	{
		printf(pBuffer);
	}

	return 0;
}
