#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
  if (argc != 3) {
    std::cout << "usage: " << argv[0] << " + ip address + port number"
              << std::endl;
    return 0;
  }
  // Create a socket
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd == -1) {
    std::cerr << "Socket creation failed: " << strerror(errno) << std::endl;
    return 1;
  }

  sockaddr_in server_addr;
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  std::string port(argv[2]);
  server_addr.sin_port = htons(std::stoi(port));
  inet_pton(AF_INET, argv[1], &server_addr.sin_addr);

  if (connect(sockfd, (sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
    std::cerr << "Connection failed: " << strerror(errno) << std::endl;
    close(sockfd);
    return 1;
  }

  while (true) {
    std::string message;
    std::cin >> message;
    ssize_t bytes_sent = send(sockfd, message.c_str(), message.length(), 0);
    if (bytes_sent == -1) {
      std::cerr << "Send failed: " << strerror(errno) << std::endl;
      close(sockfd);
      return 1;
    }

    char buffer[1024];
    memset(buffer, 0, sizeof(buffer));
    ssize_t bytes_received = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received == -1) {
      std::cerr << "Receive failed: " << strerror(errno) << std::endl;
      close(sockfd);
      return 1;
    }

    std::cout << "Received from server: " << buffer << std::endl;
  }

  close(sockfd);

  return 0;
}
