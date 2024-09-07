#include <bits/stdc++.h>
#include <cstdlib>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/wait.h>
#include <netdb.h>
#include "seats/seats_client_socket.hpp"

#include "proxy_parse.h"

using namespace std;


char* convert_Request_to_string(struct ParsedRequest *req)
{

	/* Set headers */
	ParsedHeader_set(req, "Host", req -> host);
	ParsedHeader_set(req, "Connection", "close");

	int iHeadersLen = ParsedHeader_headersLen(req);

	char *headersBuf;

	headersBuf = (char*) malloc(iHeadersLen + 1);

	if (headersBuf == NULL) {
		fprintf (stderr," Error in memory allocation  of headersBuffer ! \n");
		exit (1);
	}


	ParsedRequest_unparse_headers(req, headersBuf, iHeadersLen);
	headersBuf[iHeadersLen] = '\0';


	int request_size = strlen(req->method) + strlen(req->path) + strlen(req->version) + iHeadersLen + 4;
	
	char *serverReq;

	serverReq = (char *) malloc(request_size + 1);

	if(serverReq == NULL){
		fprintf (stderr," Error in memory allocation for serverrequest ! \n");
		exit (1);
	}

	serverReq[0] = '\0';
	strcpy(serverReq, req->method);
	strcat(serverReq, " ");
	strcat(serverReq, req->path);
	strcat(serverReq, " ");
	strcat(serverReq, req->version);
	strcat(serverReq, "\r\n");
	strcat(serverReq, headersBuf);

	free(headersBuf);

	return serverReq;

}

int createserverSocket(char *pcAddress, char *pcPort) {
  struct addrinfo ahints;
  struct addrinfo *paRes;

  int iSockfd;

  /* Get address information for stream socket on input port */
  memset(&ahints, 0, sizeof(ahints));
  ahints.ai_family = AF_UNSPEC;
  ahints.ai_socktype = SOCK_STREAM;
  if (getaddrinfo(pcAddress, pcPort, &ahints, &paRes) != 0) {
   		fprintf (stderr," Error in server address format ! \n");
		exit (1);
  }

  /* Create and connect */
  if ((iSockfd = socket(paRes->ai_family, paRes->ai_socktype, paRes->ai_protocol)) < 0) {
    	fprintf (stderr," Error in creating socket to server ! \n");
		exit (1);
  }
  if (connect(iSockfd, paRes->ai_addr, paRes->ai_addrlen) < 0) {
    	fprintf (stderr," Error in connecting to server ! \n");
		exit (1);
	}

  /* Free paRes, which was dynamically allocated by getaddrinfo */
  freeaddrinfo(paRes);

  return iSockfd;
}

void writeToserverSocket(const char* buff_to_server, seats::seats_client_socket* cs,int buff_length)
{

	string temp;

	temp.append(buff_to_server);
	
	int totalsent = 0;

	int senteach;

	while (totalsent < buff_length) {
		if ((senteach = cs->send(buff_to_server + totalsent, buff_length - totalsent)) < 0) {
			fprintf (stderr," Error in sending to server ! \n");
				exit (1);
		}
		totalsent += senteach;
	}	

}

void writeToclientSocket(const char* buff_to_server,int sockfd,int buff_length)
{

	string temp;

	temp.append(buff_to_server);

	int totalsent = 0;

	int senteach;

	while (totalsent < buff_length) {
		if ((senteach = send(sockfd, (void *) (buff_to_server + totalsent), buff_length - totalsent, 0)) < 0) {
			fprintf (stderr," Error in sending to server ! \n");
				exit (1);
		}
		totalsent += senteach;

	}	

}

void writeToClient (int Clientfd, seats::seats_client_socket* cs) {
	int MAX_BUF_SIZE = 5000;

	int iRecv;
	char buf[MAX_BUF_SIZE];
    bool end = false;

	while (!end && ((iRecv = cs->recv(buf, MAX_BUF_SIZE)) > 0)) {
	    writeToclientSocket(buf, Clientfd, iRecv);         // writing to client	    
		memset(buf,0,sizeof buf);	
	}      

	/* Error handling */
	if (iRecv < 0) {
		fprintf (stderr,"Yo..!! Error while recieving from server ! \n");
	  exit (1);
	}
}


void datafromclient(void* sockid, char* host, int port)
{	
	int MAX_BUFFER_SIZE = 5000;

	char buf[MAX_BUFFER_SIZE];

	int newsockfd = *((int*)sockid);

	char *request_message;  // Get message from URL

	request_message = (char *) malloc(MAX_BUFFER_SIZE); 

	if (request_message == NULL) {
		fprintf (stderr," Error in memory allocation ! \n");
		exit (1);
	}	

	request_message[0] = '\0';

	int total_recieved_bits = 0;

	while (strstr(request_message, "\r\n\r\n") == NULL) {  // determines end of request

	  int recvd = recv(newsockfd, buf, MAX_BUFFER_SIZE, 0) ;

	  if(recvd < 0 ){
	  	fprintf (stderr," Error while recieving ! \n");
		exit (1);
	  				
	  }else if(recvd == 0) {
	  		break;
	  } else {

	  	total_recieved_bits += recvd;

	  	/* if total message size greater than our string size,double the string size */

	  	buf[recvd] = '\0';
	  	if (total_recieved_bits > MAX_BUFFER_SIZE) {
			MAX_BUFFER_SIZE *= 2;
			request_message = (char *) realloc(request_message, MAX_BUFFER_SIZE);
			if (request_message == NULL) {
				fprintf (stderr," Error in memory re-allocation ! \n");
				exit (1);
			}
		}
	  }

	  strcat(request_message, buf);

	}

    seats::seats_client_socket* cs = new seats::seats_client_socket();
    printf("Connecting to server...\n");
    if(cs->connect(host, port)){
        printf("Failed to connect to server.");
        return;
    }
    else{
        printf("Successfully established secure attested tunnel.\n");
    } 
		
	writeToserverSocket(request_message, cs, total_recieved_bits);
	writeToClient(newsockfd, cs);

    free(request_message);
    delete cs; 
}

int main (int argc, char *argv[]) 
{ 
	int sockfd,newsockfd;

	struct sockaddr_in serv_addr; 
	struct sockaddr cli_addr;

    setbuf(stdout, NULL);
	if (argc<4) 
  	{
  		fprintf (stderr,"SORRY! Provide A Port ! \n");
  		return 1;
  	}
 
  	sockfd = socket(AF_INET, SOCK_STREAM, 0);   // create a socket

  	if (sockfd<0) {
  		fprintf (stderr,"SORRY! Cannot create a socket ! \n");
  		return 1;
  	}

  	memset(&serv_addr,0,sizeof serv_addr);

  	int  portno = atoi(argv[1]);           // argument through terminal 
 	serv_addr.sin_family = AF_INET;     // ip4 family 
  	serv_addr.sin_addr.s_addr = INADDR_ANY;  // represents for localhost i.e 127.0.0.1 
 	serv_addr.sin_port = htons(portno); 


 	int binded = bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));

 	if (binded <0 ) {
 		fprintf (stderr,"Error on binding! \n");
  		return 1;
 	}
  	
  	listen(sockfd, 1);  // can have maximum of 1 browser requests

  	int clilen = sizeof(struct sockaddr);


  	while(1) {
  		
  		/* A browser request starts here */

        printf("Waiting for connection from browser.\n");

  		newsockfd = accept(sockfd,&cli_addr, (socklen_t*) &clilen); 

        printf("Accepted connection from browser.\n");

  		if (newsockfd <0){
  			fprintf(stderr, "ERROR! On Accepting Request ! i.e requests limit crossed \n");
 		}

        printf("Waiting request data.\n");

        int pid = fork();

 		if(pid == 0){

 			datafromclient((void*)&newsockfd, argv[2], atoi(argv[3]));
 			close(newsockfd);
 			_exit(0);
 		}else{
 			close(newsockfd);     // pid =1 parent process
 		}

        printf("Request data sent.\n");
 		
        close(newsockfd);

 	}

 	close(sockfd);


	return 0;
}
