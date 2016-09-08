/*
	CN Assignment 1 - UDP based web server
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
#include <time.h>

#define BUFFER_SIZE 50000
#define CHUNK_SIZE  100

using namespace std;

string make_header(std::ifstream &);
string parse_request(char []);
void get_resource(string , std::ifstream &);

int main(int argc, char *argv[]) {
	int server_socket, client_socket, portno, no_of_bytes;
	char buffer[BUFFER_SIZE];

	struct sockaddr_in server_address, client_address;
	socklen_t client_length;

	if(argc < 2){
		std::cout << "Usage: " << argv[0] << " portno\n";
		return 1;	// no parameters passed
	}
	
	// creating the socket
	server_socket = socket(AF_INET, SOCK_DGRAM, 0);
	if(server_socket < 0){
		cout << "Error while creating socket\n";
		return 1;
	}

	bzero((char *) &server_address, sizeof(server_address));
	portno = atoi(argv[1]);

	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(portno);
	server_address.sin_addr.s_addr = htonl(INADDR_ANY);

	// bind the socket to the port
	if(bind(server_socket, (struct sockaddr *) &server_address, sizeof(server_address)) < 0){
		cout << "Binding error\n";
		return 1;
	}

	while(1){
		listen(server_socket, 5);
		client_length = sizeof(client_address);
		
		cout << "Waiting for request...\n";

		if(client_socket < 0){
			cout << "Error while accepting connection\n";
			continue;
		}

		bzero(buffer, BUFFER_SIZE);
		no_of_bytes = recvfrom(server_socket,buffer,BUFFER_SIZE,0,(struct sockaddr *)&client_address,&client_length);
      
		if(no_of_bytes<0){
			cout << "Read error\n";
			continue;
		}
		printf("Request from client:\n%s", buffer);

		// get the requested filename from the received request
		string requested_filename = parse_request(buffer);
		std::ifstream file;
		get_resource(requested_filename, file);

		// create the response to send back
		std::string header = make_header(file);
		
		// sending header back to host
		no_of_bytes = sendto(server_socket,header.c_str(),header.length(),0,(struct sockaddr *)&client_address,sizeof(client_address));
		if(no_of_bytes<0){
			cout << "Writing error - could not send response header\n";
			return 1;
		}
		
		
		int chunk_size = CHUNK_SIZE;
		bzero(buffer, BUFFER_SIZE);
		snprintf(buffer, sizeof(buffer), "%d", chunk_size); // converting int to ascii
		no_of_bytes = sendto(server_socket, buffer, strlen(buffer),0,(struct sockaddr *)&client_address,sizeof(client_address));
		if(no_of_bytes<0){
			cout << "Writing error - could not send chunk size\n";
			return 1;
		}
		
		char c[CHUNK_SIZE] = {};
		int no_of_bytes_sent = 0;
		int total_sent = 0;
		
		
		do{
			file.read(c, CHUNK_SIZE);
			no_of_bytes_sent = sendto(server_socket, c,strlen(c),0,(struct sockaddr *)&client_address,sizeof(client_address));
			usleep(1000);
			bzero(c, CHUNK_SIZE);
			total_sent += no_of_bytes_sent;	
		}
		while( no_of_bytes_sent == CHUNK_SIZE );
		time_t t1;
		time(&t1);
		std::cout << "Time when last packet sent: " << ctime(&t1) << "\n";
		
		std::cout << "Number of bytes sent: " << total_sent << "\n";
		if(no_of_bytes<0){
			cout << "Writing error - could not send data\n";
			return 1;
		}
		
		file.close();
		close(client_socket);
	}
	close(server_socket);

	return 0;
}

std::string parse_request(char buffer[]){
	// this function parses the client request and extracts the requested file's name

	std::string filename = "";
	char current_char = '\0';
	int i = 5;
	// starting with index 5 as the request will start with "GET /"
	// ignoring the '/' as including it will imply the root of the filesystem
	// when opening a file which starts with a slash

	while((current_char = *(buffer+i))!= ' '){
		// starting with the character after '/' and going up to next space
		filename += current_char;
		i++;
	}

	//cout << "Requested resource: " << filename << endl;
	return filename;
}

void get_resource(string filename, std::ifstream &file){
	file.open(filename.c_str());
}

std::string make_header(std::ifstream &file){
	// makes the response
	
	std::string response = "";
	
	if(file.is_open()){
		// requested file exists, sending it with the response
		response = "HTTP/1.1 200 OK\r\nContent-type: text/html\r\n\r\n";
	}
	else{
		// requested file not present
		response = "HTTP/1.1 404 Not Found\r\nContent-type: text/html\r\n\r\n";
	}

	return response;
}
