/* 
 * Skeleton Code for a Linux C++ Server
 */

#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/epoll.h>

#define PORT 12345
#define MAX_EVENTS 64

int main() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0); // 1. Create a TCP socket
    if (server_fd < 0) {
        perror("socket failed");
        return 1;
    }

    // 2. Allow socket address reuse (avoids "address already in use" after restart)
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // 3. Bind the socket to a port
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;  // Listen on all interfaces
    addr.sin_port = htons(PORT);
    if (bind(server_fd, (sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind failed");
        close(server_fd);
        return 1;
    }

    // 4. Start listening
    if (listen(server_fd, SOMAXCONN) < 0) {
        perror("listen failed");
        close(server_fd);
        return 1;
    }

    // 5. Create epoll instance
    int epoll_fd = epoll_create1(0);
    if (epoll_fd < 0) {
        perror("epoll_create1 failed");
        close(server_fd);
        return 1;
    }

    
    epoll_event event{};
    event.events = EPOLLIN;          // interested in readable events
    event.data.fd = server_fd;       // store FD
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &event) < 0) { // 6. Add socket to epoll interest list
        perror("epoll_ctl failed");
        close(server_fd);
        close(epoll_fd);
        return 1;
    }

    
    epoll_event events[MAX_EVENTS];       // space for up to 64 events at once
    while (true) {
        int n = epoll_wait(epoll_fd, events, MAX_EVENTS, -1); // blocks script until something happens
        if (n < 0) {
            perror("epoll_wait failed");
            break;
        }

        for (int i = 0; i < n; i++) {
            int fd = events[i].data.fd;

            if (fd == server_fd) { // Incoming connection
                sockaddr_in client_addr{};
                socklen_t client_len = sizeof(client_addr);
                int client_fd = accept(server_fd, (sockaddr*)&client_addr, &client_len);
                if (client_fd < 0) {
                    perror("accept failed");
                    continue;
                }

                // Add new client FD to epoll interest list
                epoll_event client_event{};
                client_event.events = EPOLLIN;
                client_event.data.fd = client_fd;
                epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &client_event);
                std::cout << "New client connected: FD=" << client_fd << "\n";
            } else { // Existing client sent data
                char buffer[1024]; // Max packet size
                int numBytes = read(fd, buffer, sizeof(buffer));
                if (numBytes <= 0) {
                    // client closed or error
                    close(fd);
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, nullptr);
                    std::cout << "Client disconnected: FD=" << fd << "\n";
                } else {
                    write(fd, buffer, numBytes); // Echo the data back to the client
                    // PLACE SERVER PAYLOAD CODE HERE!
                }
            }
        }
    }

    // Cleanup
    close(server_fd);
    close(epoll_fd);
    return 0;
}
