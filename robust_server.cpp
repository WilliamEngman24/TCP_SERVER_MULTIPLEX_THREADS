#include <iostream>
#include <cstring> //sending over correct data
#include <vector> //poll
#include <chrono> //keeping track of last act by client 
#include <queue> //creating thread safe task queue
#include <thread>
#include <mutex>
#include <condition_variable>

#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <poll.h>
#include <fcntl.h>

#define PORT 8080
#define IP "127.0.0.1"
#define BUFFER_SIZE 1024
#define THREAD_POOL_SIZE 4

using namespace std;
using namespace chrono;

//class that keeps data of a client
class ClientState 
{
    private:
    int fd;
    string buffer;
    steady_clock::time_point last_activity; //last time used

    //constructor
    clientState(int new_fd) 
    : fd(new_fd), buffer(""), last_activity(steady_clock::now()) {} //when creating new object take time now
    
    public:
    int get_fd() 
    {
        return fd;
    }
    void update_activity() 
    {
        last_activity = steady_clock::now();
    }

    void close_client()
    {
        close(fd);
    }
};

//file descriptor passed in argument becomes non-blocking
void non_blocking_fd(int fd) 
{
    //Current flags
    int flags = fcntl(fd, F_GETL, 0); 

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
    queue<Task> queue; //task container
    mutex mutex_Var; //synchronize access
    condition_variable condition; //block worker threads

    public:
    push(Task& task) 
    {
        { // extra braces to ensure unlock before notifying

            //ensures exclusive access to queue.
            lock_guard<mutex> lock(mutex_Var); //prevents data races.
        
            queue.push(task); //adds new task to front of queue
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
        condition.wait(lock, [&]() {return !queue.empty();});
        
        //get task from front
        Task task = queue.front();

        //removes front most task
        queue.pop();

        //returns task recently removed
        return task;
    } 
};

//Manages a number of worker threads
//Pulls tasks from a shared synchronized queue
class ThreahPool 
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
        for(int i = 0; i < THREAD_POOL_SIZE; ++i) 
        {
            //adds new thread to workers
            //"this" isnthe ThreadPool instance, runs a lambda
            workers.emplace_back([this]() 
            {
                //worker thread loop
                while(!stop) 
                {

                    Task task = task_queue.pop();
                    string response = "Echo: " + task.data;
                    send(task.client_fd, response.c_str(), response.size(), 0);
                }
            });
        }
    }

    //deconstructor
    ~ThreadPool() 
    {
        stop = true;
        for (int i = ; i < THREAD_POOL_SIZE; ++i) 
        {
            task_queue.push({-1, ""});
        }
        for(auto& worker : workers)
        {
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

    //make server file descriptor to non-blobking
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

    //put pocket on listen mode
    if (listen(server_fd, 5) < 0)
    {
        cout << "Failed to listen to socket" << endl;
        close(server_fd);
        return 0;
    }

    cout << "Server is now listening on port: " << PORT << endl;

    // I/O multiplexing setup using poll

    vector<pollfd> fds; // vector of poll file descriptors
    unordered_map<int, string> cl
    fds.pushback({server_fd, POLLIN, 0}); // puts the server_fd in the vector to be monitored


    while(1) 
    {
        //blocking call with poll
        int poll_status = poll(fds.data(), fds.size(), -1); // -1 stops program till 1 fd in fds becomes ready 

        if(poll_status < 0) 
        {
            cout "Failed to poll" << endl;
        }

        //Proccess new connections and incoming data
        for (size_t i = 0; i < fds.size(); ++i) // Iterate over all fds,
        {
            if (fds[i].revents & POLLIN) // checks if specific fd is ready to read
            {
                if(fds[i].fd == server_fd) //if a new connection has been made on the server socket
                {
                    //create client
                    sockaddr_in client;
                    socklen_t client_len = sizeof(client);

                    //accept new client
                    int client_fd = accept(server_fd, (sockaddr*)client, client_len);
                    if(client_fd < 0) 
                    {
                        cout << "Failed to accept client" << endl;
                        continue;
                    }
                    // un-block the new client_fd
                    non_blocking_fd(client_fd);

                    //add new client to file descriptors on ready to read
                    fds.push_back({client_fd, POLLIN, 0});

                    cout << "A new client has connected: " << client_fd << endl;
                }
                else //client sent data 
                {
                    //current index in fds assigns it's file descriptor to variable 
                    int client_fd = fds[i].fd;
                    
                }
            }
        }
    }

    close(server_fd);
}