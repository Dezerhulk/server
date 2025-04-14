#include <iostream>
#include <string>
#include <thread>
#include <mutex>
#include <map>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <vector>

#define PORT 8080

std::mutex clients_mutex;
std::map<int, int> users; // ID клиента -> сокет

void receive_messages(int sock)
{
    char buffer[1024] = {0};
    while (true)
    {
        int bytes_received = recv(sock, buffer, sizeof(buffer), 0);
        if (bytes_received <= 0)
        {
            std::cout << "⚠️  Соединение с сервером прервано.\n";
            break;
        }
        buffer[bytes_received] = '\0';
        std::cout << buffer << std::endl;
    }
}

int main()
{
    int sock;
    struct sockaddr_in server_addr;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1)
    {
        perror("Ошибка создания сокета");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    if (inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr) <= 0)
    {
        perror("Неверный адрес");
        exit(EXIT_FAILURE);
    }

    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Ошибка подключения");
        exit(EXIT_FAILURE);
    }

    std::cout << "✅ Подключено к серверу. Введите сообщения:\n";

    std::thread receive_thread(receive_messages, sock);
    receive_thread.detach();

    std::thread input_thread([&]
                             {
        std::string message;
        while (std::getline(std::cin, message)) {
            if (message == "exit") {
                std::cout << "🚪 Выход из чата\n";
                break;
            }
            send(sock, message.c_str(), message.size(), 0);
        }
        close(sock); });

    input_thread.join();
    return 0;
}
