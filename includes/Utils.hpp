#include <iostream>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <arpa/inet.h>
#include <signal.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>
#include <sys/socket.h>
#include <errno.h>
#include <sstream>
#include <map>


//Colors
#define RED "\033[31m"
#define GREEN "\033[32m"
#define YELLOW "\033[33m"
#define BLUE "\033[34m"
#define MAGENTA "\033[35m"
#define CYAN "\033[36m"
#define WHITE "\033[37m"
#define BOLD "\033[1m"
#define BOLDGREEN "\033[1;32m"
#define BOLDMAGENTA "\033[1;35m"
#define BOLDBLUE "\033[1;34m"

#define RESET "\033[0m"
