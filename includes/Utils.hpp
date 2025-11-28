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
#define BOLDYELLOW "\033[1;33m"

#define RESET "\033[0m"

/* MODE
/mode #channel +i
/mode #channel -i
/mode #channel +t
/mode #channel +o nick
/mode #channel -o nick
/mode #channel +k password
/mode #channel -k
/mode #channel +l 10
/mode #channel -l
*/

/* OPERATOR COMMANDS

Always check if the client is an operator before executing these commands.

KICK - Eject a client from the channel
INVITE - Invite a client to a channel
TOPIC - Change or view the channel topic
MODE - Change the channel’s mode:
- i: Set/remove Invite-only channel
- t: Set/remove the restrictions of the TOPIC command to channeloperators
- k: Set/remove the channel key (password)
- o: Give/take channel operator privilege
- l: Set/remove the user limit to channel
*/