/*
	CN Assignment 1 - TCP based web client
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
#define TEMP_FILE ".filelist"
#define MAX_FILENAME_LENGTH 255
/* 255 is maximum on ext4 */

using namespace std;

string itos(int);
string make_request(string, bool, char*, int);
void get_resource(int, string, bool, char[], int);
void get_filelist(int, string, bool, char[], int);

int main(int argc, char* argv[]) {
    char buffer[BUFFER_SIZE];

    int tcp_socket, portno, n;
    struct sockaddr_in server_addr;
    struct hostent *server;

    // checking all parameters
    if (argc < 5) {
        std::cout << "Parameters: hostname port connection-type[p/np] /filename\n";
        return 1;
    }

    char* hostname = argv[1];

    portno = atoi(argv[2]);
    tcp_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (tcp_socket < 0) {
        std::cout << "Error opening socket\n";
        return 1;
    }
    server = gethostbyname(argv[1]);
    if (server == NULL) {
        std::cout << "Host not found\n";
        return 1;
    }

    // check if a persistent connection is needed
    bool conn_persistent;
    std::string conn_type = argv[3];
    if(conn_type == "p") {
        conn_persistent = true;
    } else if(conn_type == "np") {
        conn_persistent = false;
    } else {
        std::cout << "Incorrect connection type\n";
        return 1;
    }

    std::string filename = argv[4];
    if(filename.at(0) != '/') {
        std::cout << "Filename should begin with a \'/\'\n";
        return 1;
    }

    bzero((char *) &server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&server_addr.sin_addr.s_addr, server->h_length);
    server_addr.sin_port = htons(portno);
    if (connect(tcp_socket,(struct sockaddr *) &server_addr,sizeof(server_addr)) < 0) {
        std::cout << "Error connecting\n";
        return 1;
    }

    timeval time_start, time_end, time_diff;
    gettimeofday(&time_start, NULL);
    // The initial (or the only) request
    if(conn_persistent) {

        get_filelist(tcp_socket, filename, conn_persistent, hostname, portno);
    } else {

        get_resource(tcp_socket, filename, conn_persistent, hostname, portno);
    }

    
    if(conn_persistent){
        ifstream filestoget(TEMP_FILE);
        char filename_buffer[MAX_FILENAME_LENGTH];
        bzero(filename_buffer, MAX_FILENAME_LENGTH);

        char next_filename_buffer[MAX_FILENAME_LENGTH];
        bzero(next_filename_buffer, MAX_FILENAME_LENGTH);

        filestoget.getline(filename_buffer, MAX_FILENAME_LENGTH, '\n');

        char delim = '\n';
        do{
            filestoget.getline(next_filename_buffer, MAX_FILENAME_LENGTH, '\n');
            conn_persistent = (bool) strlen(next_filename_buffer);
            cout << "Filename is: " << filename_buffer << endl;

            get_resource(tcp_socket,
                        filename_buffer,
                        conn_persistent,
                        hostname,
                        portno);

            strcpy(filename_buffer, next_filename_buffer);
        } while(conn_persistent == true);
    }
    
    gettimeofday(&time_end, NULL);

    timersub(&time_end, &time_start, &time_diff);
    cout << "Total time elapsed: " << time_diff.tv_sec << "s " << time_diff.tv_usec << "us\n";

    close(tcp_socket);
    return 0;
}

std::string itos(int n) {
    // This function is a modified version of: http://stackoverflow.com/a/64817
    std::ostringstream oss;
    oss << n;
    return oss.str();
}

std::string make_request(std::string filename, bool conn_persistent, char* hostname, int portno) {
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
    request.append(itos(portno));
    request.append("\r\n");
    if(conn_persistent == true) {
        request.append("Connection: keep-alive\r\n");
    } else
        request.append("Connection: close\r\n");
    request.append("Accept: text/html\r\n\r\n");

    return request;
}

void get_resource(int tcp_socket, string filename, bool conn_persistent, char hostname[], int portno) {
    int n;
    char buffer[BUFFER_SIZE];
    bzero(buffer,BUFFER_SIZE);
    std::string request = make_request(filename, conn_persistent, hostname, portno);

    n = write(tcp_socket, request.c_str(), request.length());
    if (n < 0) {
        std::cout << "ERROR writing request to socket\n";
        exit(EXIT_FAILURE);
    }
    std::cout << "\n\nSending request:\n";
    std::cout << request;
    std::cout << "End of request.\n";

    int retval;
    fd_set rfds;
    FD_ZERO(&rfds);
    FD_CLR(tcp_socket,&rfds);
    FD_SET(tcp_socket,&rfds);
    struct timeval timeout_interval;
    timeout_interval.tv_sec = 0;
    timeout_interval.tv_usec = 500000; // 0.5 sec

    std::cout << "Reply:\n";

    while(1) {
        timeout_interval.tv_sec = 0;
        timeout_interval.tv_usec = 500000; // 0.5 sec
        
        int retval = 100;
        fd_set rfds;
        FD_ZERO(&rfds);
        FD_CLR(tcp_socket,&rfds);
        FD_SET(tcp_socket,&rfds);

        int flag = 1;
        retval = select(tcp_socket + 1, &rfds, NULL, NULL, &timeout_interval);//set up a timer for the client in case no packet is received
        if(retval==0) { //timeout
            break;
        }
        if(retval==-1) {
            cout << "Error in receiving." << endl;
            break;
        }

        if((FD_ISSET(tcp_socket, &rfds)) && (retval == 1)) {
            bzero(buffer,BUFFER_SIZE);
            n = read(tcp_socket,buffer,BUFFER_SIZE);
            if(n == 0){
                break;
            }
            printf("%s",buffer);
        } 

    }

    std::cout << "\n\nEnd of file contents------------------------------------\n";

}

void get_filelist(int tcp_socket, string filename, bool conn_persistent, char hostname[], int portno) {
    int n;
    char buffer[BUFFER_SIZE];
    bzero(buffer,BUFFER_SIZE);

    std::string request = make_request(filename, conn_persistent, hostname, portno);

    n = write(tcp_socket, request.c_str(), request.length());
    if (n < 0) {
        std::cout << "ERROR writing request to socket\n";
        exit(EXIT_FAILURE);
    }

    std::cout << "\n\nSending request:\n";
    std::cout << request;
    std::cout << "End of request\n";

    // reading response header
    bzero(buffer,BUFFER_SIZE);
    n = read(tcp_socket,buffer,BUFFER_SIZE);
    if (n < 0)  {
        std::cout << "ERROR reading response header.\n";
        exit(EXIT_FAILURE);
    }
    cout << "Got response header:" << endl;
    cout << buffer;
    cout << "End of response header." << endl;

    
    ofstream tempfile(TEMP_FILE);
    if(!tempfile.is_open()) {
        cout << "Cannot write list of files to disk. Ensure CWD is writable. Exiting." << endl;
        exit(EXIT_FAILURE);
    }

    std::cout << "Got list:\n";

    int retval;
    fd_set rfds;
    FD_ZERO(&rfds);
    FD_CLR(tcp_socket,&rfds);
    FD_SET(tcp_socket,&rfds);
    struct timeval timeout_interval;
    timeout_interval.tv_sec = 0;
    timeout_interval.tv_usec = 500000; // 0.5 sec

    while(1) {
        int retval = 1;
        fd_set rfds;
        FD_ZERO(&rfds);
        FD_CLR(tcp_socket,&rfds);
        FD_SET(tcp_socket,&rfds);

        int flag = 1;
        retval = select(tcp_socket + 1, &rfds, NULL, NULL, &timeout_interval);//set up a timer for the client in case no packet is received
        if(retval==0) { //timeout
            //cout << "Timed out.\n";
            break;
        }
        if(retval==-1) {
            cout << "Error in receiving." << endl;
            break;
        }

        if((FD_ISSET(tcp_socket, &rfds)) && (retval == 1)) {
            bzero(buffer,BUFFER_SIZE);
            n = read(tcp_socket,buffer,BUFFER_SIZE);
            tempfile << buffer;
            printf("%s",buffer);
        }
    }


    std::cout << "End of list------------------------------------\n";

    tempfile.close();
}
