/*
	CN Assignment 1 - UDP based web client
	Amol Bhagwat
*/

#include <iostream>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fstream>
#include <sstream>
#include <netdb.h>
#include <sys/time.h>

#define BUFFER_SIZE 500000

using namespace std;

std::string make_request(std::string filename, char* hostname, char* portno){
	/* Generates a http message similar to:
	 GET /amol HTTP/1.1
	Host: localhost:60000
	Connection: keep-alive
	Accept: text/html,
	*/
	
	std::string request = "";
		
	request = "GET ";
	request.append(filename); //the resource name
	request.append(" HTTP/1.1\r\n");
	request.append("Host: ");
	request.append(hostname);
	request.append(":");
	request.append(portno);
	request.append("\r\n\r\n");
	
	return request;
}

int main(int argc, char* argv[]){
	int udp_socket, portno, no_of_bytes;
    struct sockaddr_in server_address;
    struct hostent *server;
    socklen_t len;

	// checking parameters
    char buffer[BUFFER_SIZE];
    if (argc < 4) {
       std::cout << "Parameters: hostname port /filename\n";
       return 1;;
    }
    
    portno = atoi(argv[2]);
    udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_socket < 0) {
       std::cout << "Error opening socket\n";
       return 1;
    }
    server = gethostbyname(argv[1]);
    if (server == NULL) {
       std::cout << "Host not found\n";
       return 1;
    }
    
    std::string filename = argv[3];
    if(filename.at(0) != '/'){
		std::cout << "Filename should begin with a \'/\'\n";
		return 1;
	}
    // end of checking parameters
    
    bzero((char *) &server_address, sizeof(server_address));
    server_address.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&server_address.sin_addr.s_addr, server->h_length);
    server_address.sin_port = htons(portno);
    if (connect(udp_socket,(struct sockaddr *) &server_address,sizeof(server_address)) < 0) {
       std::cout << "Error connecting\n";
       return 1;
    }
    
    bzero(buffer,BUFFER_SIZE);
    std::string request = make_request(filename, argv[1], argv[2]);
    
    std::cout << "Sending request:\n";
    std::cout << request;
    
    timeval time_start, time_end, time_diff;
    gettimeofday(&time_start, NULL);
    no_of_bytes = sendto(udp_socket,request.c_str(),request.length(),0,(struct sockaddr *)&server_address,sizeof(server_address));

    if (no_of_bytes < 0) {
       std::cout << "ERROR writing to socket\n";
       return 1;
    }
    
    bzero(buffer,BUFFER_SIZE);
    len = sizeof(server_address);
      
    no_of_bytes = recvfrom(udp_socket, buffer, BUFFER_SIZE,0,NULL, NULL);
    if(no_of_bytes < 0){
		std::cout << "Reading error - could not get response header\n";
		return 1;
	}
	
    printf("Received header:\n%s\n",buffer);
    bzero(buffer,BUFFER_SIZE);
		
	no_of_bytes = recvfrom(udp_socket, buffer, BUFFER_SIZE,0,NULL, NULL);
	if(no_of_bytes < 0){
		std::cout << "Reading error - could not get chunk size\n";
		return 1;
	}
	int chunk_size = atoi(buffer);
	
	long total_received = 0;

	while(1){
		int retval = 1;
		fd_set rfds;
		FD_ZERO(&rfds);
		FD_CLR(udp_socket,&rfds);
		FD_SET(udp_socket,&rfds);
					
		int flag = 1;
		
		timeval timeout_interval;
		timeout_interval.tv_sec = 0;
		timeout_interval.tv_usec = 5000;

		retval = select(udp_socket + 1, &rfds, NULL, NULL, &timeout_interval);//set up a timer for the client in case no packet is received
		if(retval==0){  //timeout
		    cout << "\nTimed out.\n";
		    break;
		}
		if(retval==-1){
		    cout << "Error in receiving." << endl;
		    break;
		}
					
		if((FD_ISSET(udp_socket, &rfds)) && (retval == 1)){
			no_of_bytes = recvfrom(udp_socket, buffer, chunk_size,0,NULL, NULL);
			printf("%s",buffer);
			bzero(buffer,BUFFER_SIZE);
			total_received += no_of_bytes;
		}
		if(no_of_bytes < chunk_size){
			cout << "\nEnd of file reached.\n";
			break;
		}
	}
	gettimeofday(&time_end, NULL);

	timersub(&time_end, &time_start, &time_diff);
	cout << "Total time elapsed: " << time_diff.tv_sec << "s " << time_diff.tv_usec << "us\n";


	std::cout << "Number of bytes received: " << total_received << "\n";	
      
    if (no_of_bytes < 0){
       std::cout << "ERROR reading from socket\n";
       return 1;
    }
    
    close(udp_socket);
    return 0;
}


