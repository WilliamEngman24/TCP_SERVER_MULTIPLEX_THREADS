# TCP_SERVER_MULTIPLEX_THREADS

* --- Purpose ---
The server acts as an echo server.

It will:
* Say on which port its on
* Mention when clients connect and disconnect
* Post the message to send back and to which client it will send it to
* Send back the message from the client

* --- Libraries ---
cstring             //for sending over correct data
vector              //poll
chrono              //keeping track of last act by client 
queue               //creating thread safe task queue
thread              //allows creation, joinage and detachment of threads
mutex               //ensures one thread at a time access the task queue 
condition_variable  //synchronizes threads, makes them wait instead of constatnly checking
unordered_map       //hash-table based, stores information about clients, based on their fd
unistd.h            //used for closing sockets
sys/socket.h        //provides declarations for essential socket creation and usage
netinet/in.h        //provides definitions for internet protocol, IPv4 socket addresses 
arpa/inet.h         //provides functions for convertion IP adresses
poll.h          //used to monitor multiple file descriptors, handles milti client connections and avoiding blocking 
fcntl.h             //gives access to fd control options, sets sockets to non-blocking

* --- class ClientState ---
A class that manages the state and data for each connected client

* new client is created with fd
* its state stored in the unordered map
* read, manage partial data received, handle client independantly

* --- void non_blocking_fd(int fd) ---
A function that sets a fd to non-blocking mode, using fcntl library

Takes an argument, which is the fd to un-block.

Makes so socket operations reutrn immediately instead of waiting for/blocking data.

Used with poll.

* --- struct Task ---
A struct that holds information about client socket and the data associated with it.

Used in TaskQueue, to create a queue of Task structs.

Threads will pull Task objects.

* --- class TaskQueue ---
A class that manages Tasks between server thread and worker threads. it is a thread safe queue.

The sheduler.

Will push() and pop() Task objects.

* Stores tasks in queue
* Uses mutex to ensure one thread at a time is accesses the queue and the data stored in it
* uses condition variable waits efficiently instead of constantly polling

* --- class ThreadPool ---
Current size: 4

A class that manages worker threads, which in turn processes Task objects from the TaskQueue. Handles many client connections without creating new threads.

Resues a pool of threads.

* Holds all the worker threads
* References to TaskQueue
* Stops threads when server shuts down

* --- Unordered_map---
I use unordered map instead of map, because of:
* its quicker avarage lookup time O(1), Worst (n)
    map has O(log n)
    
* ---Poll setup---
Regarding the poll setup, i use vectors as it allows for dynamic resizing and avoids manual tracking of nfds
The following could have been used:

struct pollfd fds[MAX_CLIENTS]; 
fds[0].fd = listen_fd;
fds[0].events = POLLIN;
int ndfs = 1;
