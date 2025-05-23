# TCP_SERVER_MULTIPLEX_THREADS

---Unordered_map---
I use unordered map instead of map, because of:
* its quicker avarage lookup time O(1), Worst (n)
    map has O(log n)
    
---Poll setup---
Regarding the poll setup, i use vectors as it allows for dynamic resizing and avoids manual tracking of nfds
The following could have been used:

struct pollfd fds[MAX_CLIENTS]; 
fds[0].fd = listen_fd;
fds[0].events = POLLIN;
int ndfs = 1;
---
