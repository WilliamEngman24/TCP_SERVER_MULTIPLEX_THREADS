#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 8080
#define IP "127.0.0.1"
#define BUFFER_SIZE 1024

using namespace std;

int main()
{
    //get message from user
    string userInput;
    cout << "Enter message sent to server: ";
    getline(cin, userInput);

    //create client socket
    int client_fd = socket (AF_INET, SOCK_STREAM, 0);
    if(client_fd < 0)
    {
        cout << "Failed to create client socket" << endl;
        close(client_fd);
        return 0;
    }

    //configuring server address
    struct sockaddr_in server;
    memset(&server, 0 , sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);

    //setting server address
    if (inet_pton(AF_INET, IP, &server.sin_addr) <= 0) 
    {
        cout << "Failed to assign IP address" << endl;
        close(client_fd);
        return 0;
    }

    //connect client socket to server address
    if (connect(client_fd, (struct sockaddr*)&server, sizeof(server)) < 0) 
    {
        cout << "failed to connect to server" << endl;
        close(client_fd);
        return 0;
    }

    //enter into constant feed
    bool connection = true;
    while(connection) 
    {

        //send first message to server
        int status = send(client_fd,userInput.c_str(), userInput.size(), 0);
        if (status < 0) 
        {
            cout << "Failed to send first message" << endl;
        }
        else 
        {
            //create buffer for in comming message from server
            char buffer[BUFFER_SIZE];
            memset(buffer, 0, BUFFER_SIZE); // zero-initilaizing

            int bytes = recv(client_fd, buffer, BUFFER_SIZE - 1, 0);
            if (bytes <= 0) 
            {
                cout << "No data received!" << endl;
            }
            else 
            {
                string message(buffer, bytes); //takes buffer and bytes to make a new string
                cout << message << endl; //print server response
            }
        }

        sleep(2);

    }
    close(client_fd);
}