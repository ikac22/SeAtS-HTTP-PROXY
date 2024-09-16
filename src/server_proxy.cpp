#include <bits/stdc++.h>
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
#include "seats/seats_server_socket.hpp"

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

void writeToserverSocket(const char* buff_to_server,int sockfd,int buff_length)
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

void writeToclientSocket(const char* buff_to_server,seats::seats_socket* sockfd,int buff_length)
{

	int totalsent = 0;

	int senteach;

	while (totalsent < buff_length) {
		if ((senteach = sockfd->send((buff_to_server + totalsent), buff_length - totalsent)) < 0) {
			fprintf (stderr," Error in sending to server ! \n");
			exit (1);
		}
		totalsent += senteach;
	}	
}

void writeToClient (seats::seats_socket* Clientfd, int Serverfd) {
	int MAX_BUF_SIZE = 5000;

	int iRecv;
	char buf[MAX_BUF_SIZE];

	while ((iRecv = recv(Serverfd, buf, MAX_BUF_SIZE, 0)) > 0) {
	    writeToclientSocket(buf, Clientfd,iRecv);         // writing to client	    
		memset(buf,0,sizeof buf);	
	}      

	/* Error handling */
	if (iRecv < 0) {
		fprintf (stderr,"Yo..!! Error while recieving from server ! \n");
	  exit (1);
	}
}


void datafromclient(seats::seats_socket* cs, char* host, char* port)
{	
	int MAX_BUFFER_SIZE = 5000;

	char buf[MAX_BUFFER_SIZE];

	char *request_message;  // Get message from URL

	request_message = (char *) malloc(MAX_BUFFER_SIZE); 

	if (request_message == NULL) {
		fprintf (stderr," Error in memory allocation ! \n");
		exit (1);
	}	

	request_message[0] = '\0';

	int total_recieved_bits = 0;

	while (strstr(request_message, "\r\n\r\n") == NULL) {  // determines end of request

	  int recvd = cs->recv(buf, MAX_BUFFER_SIZE);

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
    printf("Received:\n%s\n", request_message);

	// struct ParsedRequest *req;    // contains parsed request
	//
	// req = ParsedRequest_create();
	//
	// if (ParsedRequest_parse(req, request_message, strlen(request_message)) < 0) {		
	// 	fprintf (stderr,"Error in request message..only http and get with headers are allowed ! \n");
	// 	exit(0);
	// }
	//
	// /*final request to be sent*/
	// 
	// char*  browser_req  = convert_Request_to_string(req);		
		
	int iServerfd;

	iServerfd = createserverSocket(host, port);

	writeToserverSocket(request_message, iServerfd, total_recieved_bits);
    printf("Getting response from server and sending to clinet.\n");
	writeToClient(cs, iServerfd);
    printf("Sent response to client\n");

	// ParsedRequest_destroy(req);
		
	close(iServerfd);

}

int main (int argc, char *argv[]) 
{

	if (argc<4) 
  	{
  		fprintf (stderr,"SORRY! Provide A Port ! \n");
  		return 1;
  	}


    setbuf(stdout, NULL);

  	int  portno = atoi(argv[1]);           // argument through terminal 
    char* host = argv[2];
    char* srv_port = argv[3];

    seats::seats_server_socket ss(portno); 
    seats::seats_socket* cs;
        
    cs = ss.accept();
  	if (!cs){
  	    fprintf(stderr, "ERROR: Failed on initial TCP connection with client. \n");
        return 1;
 	}
    if(cs->accept()){
  	    fprintf(stderr, "ERROR: Failed on initial TLS connection with client. \n");
        return 1; 
    }
    char a[10];
    cs->recv(a, 6);

  	while(1) {
  		
  		/* A browser request starts here */
  	    fprintf(stdout, "Waiting connection from client proxy... \n");

        cs = ss.accept();
  		if (!cs){
  			fprintf(stderr, "ERROR! On Accepting Request ! i.e requests limit crossed \n");
            return 1;
 		}
        
        
        int pid = fork();
 
 		if(pid == 0){
            if(cs->accept()){
                printf("Failed to establish secure attested tunnel.\n");
                return 1;
            }
            else{
                printf("Successfully established secure attested tunnel.\n");
            }
 		    datafromclient(cs, host, srv_port);
            delete cs;
 			_exit(0);
 		}else{
            delete cs;
 		}

 	}

    cs->close();

	return 0;
}
