/*
      CN Assignment 1 - Multi-threaded TCP server
      Amol Bhagwat
*/

/* References:
1. https://gist.github.com/silv3rm00n/5821760#file-tcp_server-c-L56
2. http://www.cs.cmu.edu/afs/cs/academic/class/15492-f07/www/pthreads.html
*/

#include <pthread.h>
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
#include <sys/time.h>
#include <netinet/tcp.h>

#define BUFFER_SIZE 1000
#define TIMEOUT_SEC 5
#define TIMEOUT_USEC 0
     
using namespace std;

bool validate_request(char*);
bool check_if_persistent(char*);
bool get_resource(string, std::ifstream &);

int get_filelength(std::ifstream &);

std::string itos(int);
std::string parse_request(char[]);
std::string make_response(std::ifstream &);
std::string make_response(int);

void set_timeout(timeval &, int, int);

void *listener(void *);

int main(int argc, char *argv[]) {
      // setting up the socket
      int server_socket, client_socket, portno, n;
      char buffer[BUFFER_SIZE];

      struct sockaddr_in server_address, client_address;
      socklen_t client_length;

      if(argc < 2){     // no parameters passed
            cout << "Usage: " << argv[0] << " portno\n";
            return 1;
      }           
      server_socket = socket(AF_INET, SOCK_STREAM, 0);
      if(server_socket < 0){
            cout << "Error while creating socket\n";
            return 1;
      }

      bzero((char *) &server_address, sizeof(server_address));
      portno = atoi(argv[1]);

      server_address.sin_family = AF_INET;
      server_address.sin_port = htons(portno);
      server_address.sin_addr.s_addr = INADDR_ANY;

      if(bind(server_socket, (struct sockaddr *) &server_address, sizeof(server_address)) < 0){
            cout << "Binding error\n";
            return 1;
      }
      
      listen(server_socket, 5);
      client_length = sizeof(client_address);
            
      pthread_t thread_id;
      
      while(client_socket = accept(server_socket, (struct sockaddr *) &client_address, &client_length)){
            if( pthread_create( &thread_id , NULL ,  listener , (void*) &client_socket) < 0){
                  cout << "Could not create thread.\n";
                  exit(EXIT_FAILURE);
            }
      }
      exit(0);
}


void *listener(void *cli_sock){
    int client_socket = *(int*)cli_sock;
    char buffer[BUFFER_SIZE];
    struct timeval timeout_interval;

    while(1){
      bzero(buffer, BUFFER_SIZE);
      cout << "Waiting for a request..." << endl;
      
      // setting up variables for the timer
      int retval = 1;
      fd_set rfds;
      FD_ZERO(&rfds);
      FD_CLR(client_socket,&rfds);
      FD_SET(client_socket,&rfds);
      
      int flag = 1;
      setsockopt(client_socket,
                  IPPROTO_TCP,
                  TCP_NODELAY,
                  (char *) &flag,
                  sizeof(int));
      set_timeout(timeout_interval, TIMEOUT_SEC, TIMEOUT_USEC);
      retval = select(client_socket + 1, &rfds, NULL, NULL, &timeout_interval);
      if(retval==0){  //timeout
          cout << "Timed out.\n";
          break;
      }
      if(retval==-1){
          cout << "Error in receiving." << endl;
          break;
      }
      
      if((FD_ISSET(client_socket, &rfds)) && (retval == 1)){
            int n = read(client_socket, buffer, BUFFER_SIZE);   
            cout << "Read " << n << " bytes from client." << endl;
            if(n<0){
                  cout << "Read error\n";
                  break;
            }
            if(n == 0){
                  cout << "Got a blank request. Closing connection.\n";
                  break;
            }
            printf("Request from client:\n%s", buffer);
            std::string response;
            
            // validating the client's request
            if(!validate_request(buffer)){
                  response = make_response(400);
                  write(client_socket, response.c_str(), response.length());  
                  cout << "Invalid request." << endl;
                  break;
            }
            string requested_filename = parse_request(buffer);
            std::ifstream file;
            bool file_exists = get_resource(requested_filename, file);
            bool keep_alive = check_if_persistent(buffer);
      
            // sending the response header to the client
            response = make_response(file);
            n = write(client_socket, response.c_str(), response.length());
            if(n<0){
                  cout << "Error while sending response header!\n";
                  break;
            }
            cout << "Sending reply:" << endl;
            cout << response << endl;
            
            usleep(500000); /* waiting 0.5 sec after sending header */
            if(file_exists){
                  // no need for an else case, that has been taken care of
                  while(!file.eof()){
                        bzero(buffer, BUFFER_SIZE);
                        file.read(buffer, BUFFER_SIZE);
                        n = write(client_socket, buffer, file.gcount());
                        if(n<0){
                              cout << "Error while sending file data!\n";
                              break;
                        }                 
                  }
                  cout << "Sent file successfully." << endl;
                  file.close();
                  //usleep(1100000);
            }
      
            if(keep_alive == false){
                  cout << "Not waiting for more requests." << endl;
                  break;
            }
      }     
    }
    close(client_socket);
    cout << "Closing the connection." << endl;
}


bool validate_request(char* request){
      if( strncmp("GET", request, 3) == 0){
            return true;
      }
      else return false;
}

bool check_if_persistent(char* request){
      return (bool) strstr(request, "Connection: keep-alive"); 
}

bool get_resource(string filename, std::ifstream &file){
      file.open(filename.c_str());
      return file.is_open();
}

int get_filelength(std::ifstream &file){
      file.seekg (0, file.end);
    int filelength = file.tellg();
    file.seekg (0, file.beg);
    return filelength;        
}

std::string itos(int n){
      // This function is a modified version of: http://stackoverflow.com/a/64817
      std::ostringstream oss;
      oss << n;
      return oss.str();
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

      cout << "Requested resource: " << filename << endl;
      return filename;
}

std::string make_response(std::ifstream &file){
      std::string response = "";
      
      if(file.is_open()){
            // requested file exists, sending it with the response
            response = "HTTP/1.1 200 OK\r\n";
            response.append("Content-length: ");
            response.append(itos(get_filelength(file)));
            response.append("\r\n\r\n");
      }
      else{
            response = make_response(404); 
      }

      return response;
}

std::string make_response(int response_code){
      std::string response = "";
      
      switch(response_code){
            case 400:
                  response = "HTTP/1.1 400 Bad Request";
                  response.append("\r\n\r\n");
                  break;
            case 404:
                  // IE (perhaps Edge too) seems to be the only browser
                  // which displays its own error page to the user in case it receives a 404
                  // Appending a basic HTML page to notify the user in case of an error
                  cout << "Not found." << endl;
                  response = "HTTP/1.1 404 Not Found\r\n\r\n";
                  response.append("<html><head><title>Error 404</title>"
                        "Error 404: Not Found</head></html>\n");
                  break;
            default:
                  response = "HTTP/1.1 400 Bad Request";
                  response.append("\r\n\r\n");
                  break;      
      }
      
      return response;
}

void set_timeout(struct timeval &timeout, int seconds, int useconds){
      timeout.tv_sec = seconds;
      timeout.tv_usec = useconds;
}

