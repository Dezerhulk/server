#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <mutex>
#include <map>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

#define PORT 8080
#define BUFFER_SIZE 1024

std::mutex clients_mutex;
std::map<int, int> clients;

void broadcast_message(const std::string &message, int sender_socket)
{
        std::lock_guard<std::mutex> lock(clients_mutex);
        for (const auto &[id, socket] : clients)
        {
                if (socket != sender_socket)
                {
                        send(socket, message.c_str(), message.size(), 0);
                }
        }
}

void handle_client(int client_socket)
{
        char buffer[BUFFER_SIZE];
        std::string client_name = "Client " + std::to_string(client_socket);

        clients_mutex.lock();
        clients[client_socket] = client_socket;
        clients_mutex.unlock();

        std::string welcome_message = client_name + " подключился к чату\n";
        broadcast_message(welcome_message, client_socket);

        while (true)
        {
                memset(buffer, 0, BUFFER_SIZE);
                int bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0);
                if (bytes_received <= 0)
                {
                        break;
                }

                std::string message = client_name + ": " + buffer;
                broadcast_message(message, client_socket);
        }

        clients_mutex.lock();
        clients.erase(client_socket);
        clients_mutex.unlock();

        std::string exit_message = client_name + " покинул чат\n";
        broadcast_message(exit_message, client_socket);
        close(client_socket);
}

int main()
{
        int server_socket, client_socket;
        struct sockaddr_in server_addr, client_addr;
        socklen_t client_len = sizeof(client_addr);

        server_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (server_socket < 0)
        {
                exit(EXIT_FAILURE);
        }

        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = INADDR_ANY;
        server_addr.sin_port = htons(PORT);

        if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
        {
                close(server_socket);
                return 1;
        }

        if (listen(server_socket, 10) < 0)
        {
                close(server_socket);
                exit(EXIT_FAILURE);
        }

        while (true)
        {
                client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_len);
                if (client_socket < 0)
                {
                        continue;
                }

                std::thread(handle_client, client_socket).detach();
        }

        close(server_socket);
        return 0;
}