#include <iostream>
#include <cstring> //sending over correct data
#include <vector> //poll
#include <chrono> //keeping track of last act by client 
#include <queue> //creating thread safe task queue
#include <thread>
#include <mutex>
#include <condition_variable>
#include <unordered_map>

#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <poll.h>
#include <fcntl.h>

#define PORT 8080
#define IP "127.0.0.1"
#define BUFFER_SIZE 1024
#define THREAD_POOL 4

using namespace std;
using namespace chrono;

//class that keeps data of a client
class ClientState 
{
    private:
    int fd;
    string buffer;
    steady_clock::time_point last_activity; //last time used

    public:

    //default constructor
    ClientState() 
    : fd(-1), buffer(""), last_activity(steady_clock::now()) {}

    //constructor
    ClientState(int new_fd) 
    : fd(new_fd), buffer(""), last_activity(steady_clock::now()) {} //when creating new object take time now

    void update_activity() 
    {
        last_activity = steady_clock::now();
    }
};

//file descriptor passed in argument becomes non-blocking
void non_blocking_fd(int fd) 
{
    //Current flags
    int flags = fcntl(fd, F_GETFL, 0); 

    //set the requested fd to non-blocking
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
};

//datatype for a task
//representing a peice of data, also used in thread pool
struct Task 
{
    int client_fd;
    string data; //actual data message received from the client
};

//used to share work tasks between threads
class TaskQueue 
{
    queue<Task> task_queue; //task container
    mutex mutex_Var; //synchronize access
    condition_variable condition; //block worker threads

    public:
    void push(Task task) 
    {
        { // extra braces to ensure unlock before notifying

            //ensures exclusive access to queue.
            lock_guard<mutex> lock(mutex_Var); //prevents data races.
        
            task_queue.push(task); //adds new task to front of queue
        }

        //wake up/notify a waiting thread
        condition.notify_one();
    }

    //wait till available, safely removes task from queue and returns removed task
    Task pop() 
    {
        //gets mutex, condition.wait needss it
        unique_lock<mutex> lock(mutex_Var);

        //puts thread to sleepuntil condition met
        condition.wait(lock, [&]() {return !task_queue.empty();});
        
        //get task from front
        Task task = task_queue.front();

        //removes front most task
        task_queue.pop();

        //returns task recently removed
        return task;
    } 
};

//Manages a number of worker threads
//Pulls tasks from a shared synchronized queue
class ThreadPool 
{
    vector<thread> workers; //worker threads

    // referance to TaskQeue object in main.
    TaskQueue& task_queue; //doesn't own queue, just passes the one used in it

    bool stop = false; //flag to stop

    public:
    //constructor
    ThreadPool(TaskQueue& new_task_queue)
    : task_queue(new_task_queue) 
    {
        //launch each worker thread
        for(int i = 0; i < THREAD_POOL; ++i) 
        {
            //adds new thread to workers
            //"this" isnthe ThreadPool instance, runs a lambda
            workers.emplace_back([this]() 
            {
                //worker thread loop
                while(!stop) 
                {
                    //removes front most task and returns it
                    Task task = task_queue.pop();

                    //the message sent to client with task data
                    string response = "Echo: " + task.data;

                    //send said message to client 
                    int status = send(task.client_fd, response.c_str(), response.size(), 0); 
                    {
                        if(status <0) 
                        {
                            cout << "Failed to send echo to client, in thread pool" << endl;
                        }
                    }
                }
            });
        }
    }

    //destructor
    //shuts down all worker threads
    ~ThreadPool() 
    {
        stop = true; //will exit loop after exiting this

        //unblock all threads that might be stuck on pop()
        for (int i = 0; i < THREAD_POOL; ++i) 
        {
            //signals worker thread to exit
            task_queue.push({-1, ""});
        }
        for(auto& worker : workers)
        {
            //waits for all threads to join, no threads left running 
            worker.join();
        }
    }

};


int main() 
{
    //create server socket
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0)
    {
        cout << "Failed to create socket in server" << endl;
        close(server_fd);
        return 0;
    }

    //set socket options
    int optval = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    //make server file descriptor to non-blocking
    non_blocking_fd(server_fd);

    //zero-initializing socket adress
    struct sockaddr_in server;
    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr(IP);
    server.sin_port = htons(PORT);

    //bind socket to server
    int status = bind(server_fd, (struct sockaddr*)&server, sizeof(server));
    if(status < 0) 
    {
        cout << "Failed to bind socket to server" << endl;
        close(server_fd);
        return 0;
    }

    //put socket on listen mode
    if (listen(server_fd, 5) < 0)
    {
        cout << "Failed to listen to socket" << endl;
        close(server_fd);
        return 0;
    }

    cout << "Server is now listening on port: " << PORT << endl;

    // I/O multiplexing setup using poll

    vector<pollfd> fds; // vector of poll file descriptors
    unordered_map<int, ClientState> clients;
    fds.push_back({server_fd, POLLIN, 0}); // puts the server_fd in the vector to be monitored

    TaskQueue task_queue;
    ThreadPool pool(task_queue);

    while(1) 
    {
        //blocking call with poll
        int poll_status = poll(fds.data(), fds.size(), -1); // -1 stops program till 1 fd in fds becomes ready 

        if(poll_status < 0) 
        {
            cout << "Failed to poll" << endl;
            break;
        }

        steady_clock::time_point now = steady_clock::now();

        //Proccess new connections and incoming data
        for (size_t i = 0; i < fds.size(); ++i) // Iterate over all fds,
        {
            //Extracts file descriptor from vector of a poll that has all file descriptors
            int fd = fds[i].fd;
            if (fds[i].revents & POLLIN) // checks if specific fd is ready to read
            {
                if(fds[i].fd == server_fd) //if a new connection has been made on the server socket
                {
                    //create client
                    sockaddr_in client_addr;
                    socklen_t client_len = sizeof(client_addr);

                    //accept new client
                    int client_fd = accept(server_fd, (sockaddr*)&client_addr, &client_len);
                    if(client_fd < 0) 
                    {
                        cout << "Failed to accept client" << endl;
                        continue;
                    }
                    // un-block the new client_fd
                    non_blocking_fd(client_fd);

                    //add new client to file descriptors on ready to read
                    fds.push_back({client_fd, POLLIN, 0});

                    //adds new client to unordered map
                    clients.emplace(client_fd, ClientState(client_fd));

                    cout << "A new client has connected: " << client_fd << endl;
                }
                else //handle dlient data 
                {
                    char buffer[BUFFER_SIZE];
                    int bytes_read = recv(fd, buffer, sizeof(buffer), 0);
                    if(bytes_read <= 0 ) //data was not received
                    {
                        cout << "Failed to receive from client: " << fd << endl;
                        
                        //closes client socket and removes it from clients map and fds vector
                        close(fd);
                        clients.erase(fd);
                        fds.erase(fds.begin() + i);
                        --i; //decrement so loop doesn't skip next element after erasing
                    }
                    else //data was received
                    {
                        //assigns data from clients map to new object
                        auto&client = clients[fd];

                        //updates activity of client
                        client.update_activity();

                        //
                        string data(buffer, bytes_read);
                        task_queue.push({fd, data});
                    }
                }
            }
        }

        // Idle client removal here

    }

    close(server_fd);
}