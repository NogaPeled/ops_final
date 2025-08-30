// ==================== client.cpp (part 7) ====================
// Sends 1 command line to the server and prints the reply.
// Usage examples:
//   ./client ALGO SCC RANDOM 8 12 7 --directed
//   ./client ALGO MST MANUAL 4 : 0-1 1-2 2-3 3-0
// =============================================================

#include <arpa/inet.h>    // inet_pton
#include <netinet/in.h>   // sockaddr_in
#include <sys/socket.h>   // socket/connect/send/recv
#include <unistd.h>       // close, shutdown

#include <cstdlib>        // std::atoi
#include <iostream>       // std::cout, std::cerr
#include <sstream>        // std::ostringstream
#include <string>         // std::string

static constexpr const char* kIP   = "127.0.0.1"; // server IP
static constexpr const char* kPort = "5555";      // server port (string)

int main(int argc, char** argv) {
    // Require at least one token after program name.
    if (argc < 2) {
        std::cout
          << "Usage:\n"
          << "  " << argv[0] << " ALGO <MST|SCC|MAXFLOW|HAMILTON> RANDOM <V> <E> <SEED> [--directed]\n"
          << "  " << argv[0] << " ALGO <MST|SCC|MAXFLOW|HAMILTON> MANUAL <V> : u-v u-v ... [--directed]\n";
        return 1;
    }

    // Reconstruct the exact line the server expects, terminated by '\n'.
    std::ostringstream oss;
    for (int i = 1; i < argc; ++i) {
        if (i > 1) oss << ' ';
        oss << argv[i];
    }
    oss << '\n';
    const std::string line = oss.str();

    // --- connect to server ---
    const int fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) { perror("socket"); return 1; }

    sockaddr_in sa{};
    sa.sin_family = AF_INET;
    sa.sin_port   = htons(std::atoi(kPort));        // convert port string -> number -> network byte order
    if (inet_pton(AF_INET, kIP, &sa.sin_addr) <= 0) { perror("inet_pton"); ::close(fd); return 1; }
    if (connect(fd, (sockaddr*)&sa, sizeof(sa)) < 0) { perror("connect"); ::close(fd); return 1; }

    // --- send request ---
    if (::send(fd, line.c_str(), (int)line.size(), 0) != (ssize_t)line.size()) {
        perror("send"); ::close(fd); return 1;
    }

    // --- receive single reply (up to 4 KB) ---
    char buf[4096];
    const ssize_t n = ::recv(fd, buf, sizeof(buf) - 1, 0);
    if (n < 0) perror("recv");
    if (n > 0) { buf[n] = '\0'; std::cout << buf; }

    ::shutdown(fd, SHUT_RDWR);
    ::close(fd);
    return 0;
}
