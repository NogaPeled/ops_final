// ==================== client.cpp ====================
// Simple TCP client for the Euler server.
// Sends exactly one line command and prints the reply.
//
// Usage examples:
//   ./client RANDOM 8 12 1
//   ./client RANDOM 8 12 1 --directed
//   ./client MANUAL 5 : 0-1 1-2 2-3 3-4 4-0
//   ./client QUIT
// ====================================================

#include <arpa/inet.h>      // inet_pton, htons
#include <sys/socket.h>     // socket, connect, send, recv, shutdown
#include <unistd.h>         // close
#include <cstring>          // std::strlen
#include <iostream>         // std::cout, std::cerr
#include <sstream>          // std::ostringstream
#include <string>           // std::string
#include <vector>           // std::vector

// Match the server config (no header):
static constexpr const char* kIP   = "127.0.0.1"; // server address
static constexpr const char* kPort = "5555";      // server port (as number string)
static constexpr int         kBuf  = 4096;        // recv buffer

// Build the one-line command string from argv.
static std::string build_command(int argc, char** argv) {
    if (argc < 2) {                                             // need at least the command
        return "";                                              // return empty → will trigger usage
    }
    std::string cmd = argv[1];                                  // first token

    if (cmd == "RANDOM") {                                      // RANDOM <V> <E> <SEED> [--directed]
        if (argc != 5 && argc != 6) return "";                  // must be 4 or 5 args after program
        std::ostringstream oss;                                 // compose the line
        oss << "RANDOM " << argv[2] << ' ' << argv[3] << ' ' << argv[4]; // required fields
        if (argc == 6) oss << ' ' << argv[5];                   // optional --directed
        oss << "\n";                                            // terminate line
        return oss.str();                                       // ready
    }

    if (cmd == "MANUAL") {                                      // MANUAL <V> : u-v u-v ...
        if (argc < 5) return "";                                // need at least MANUAL V : edge
        std::ostringstream oss;                                 // build the line
        oss << "MANUAL " << argv[2] << " : ";                   // prefix "MANUAL V : "
        for (int i = 4; i < argc; ++i) {                        // edges start at argv[4]
            oss << argv[i];                                     // append token
            if (i + 1 < argc) oss << ' ';                       // spaces between tokens
        }
        oss << "\n";                                            // newline terminator
        return oss.str();                                       // return composed line
    }

    if (cmd == "QUIT") {                                        // QUIT
        return std::string("QUIT\n");                           // simple
    }

    return "";                                                  // unknown command → usage
}

int main(int argc, char** argv) {
    std::string line = build_command(argc, argv);               // build command string
    if (line.empty()) {                                         // if invalid usage
        std::cout << "Usage:\n"                                 // print usage and exit
                  << "  " << argv[0] << " RANDOM <V> <E> <SEED> [--directed]\n"
                  << "  " << argv[0] << " MANUAL <V> : u-v u-v ...\n"
                  << "  " << argv[0] << " QUIT\n";
        return 1;                                               // failure exit code
    }

    int fd = ::socket(AF_INET, SOCK_STREAM, 0);                 // create TCP socket
    if (fd < 0) { std::perror("socket"); return 1; }            // guard

    sockaddr_in sa{};                                           // server address struct
    sa.sin_family = AF_INET;                                    // IPv4
    sa.sin_port   = htons(static_cast<uint16_t>(std::stoi(kPort))); // port -> network order
    if (::inet_pton(AF_INET, kIP, &sa.sin_addr) <= 0) {         // convert IP string to binary
        std::perror("inet_pton"); ::close(fd); return 1;        // fail if invalid
    }

    if (::connect(fd, (sockaddr*)&sa, sizeof(sa)) < 0) {        // connect to server
        std::perror("connect"); ::close(fd); return 1;          // fail if cannot connect
    }

    // send the command line
    const char* p = line.c_str();                               // pointer to bytes
    std::size_t left = line.size();                             // bytes left to send
    while (left) {                                              // loop until sent
        ssize_t n = ::send(fd, p, left, 0);                     // send chunk
        if (n <= 0) { std::perror("send"); ::close(fd); return 1; } // fail
        p    += n;                                              // advance
        left -= (std::size_t)n;                                 // reduce remaining
    }

    ::shutdown(fd, SHUT_WR);                                    // signal end-of-request

    // receive response (until server closes or no more data)
    char buf[kBuf];                                             // read buffer
    while (true) {                                              // read loop
        ssize_t n = ::recv(fd, buf, sizeof(buf)-1, 0);          // read some
        if (n < 0) { std::perror("recv"); ::close(fd); return 1; } // error
        if (n == 0) break;                                      // EOF from server
        buf[n] = '\0';                                          // make it a string
        std::cout << buf;                                       // print to stdout
    }

    ::close(fd);                                                // close socket
    return 0;                                                   // success
}
