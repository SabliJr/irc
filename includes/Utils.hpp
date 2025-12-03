#include <arpa/inet.h>
#include <cstdlib>
#include <cstring>
#include <errno.h>
#include <fcntl.h>
#include <iostream>
#include <map>
#include <netinet/in.h>
#include <poll.h>
#include <signal.h>
#include <sstream>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

// Colors
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
#define BOLDRED "\033[1;31m"

#define RESET "\033[0m"

// Below are common irssi user commands

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

//!     /connect localhost 8888 pa

/* Bot command
/msg MEE6 !hello
!hello
!42
!server
*/


/*
* how to send a file between users
echo hello > /tmp/hi.txt

/dcc send koko /tmp/hi.txt
/dcc get momo hi.txt

/ctcp koko VERSION

*BIG FILE
dd if=/dev/zero of=/tmp/big.bin bs=1M count=3000
/dcc send koko /tmp/big.bin
/dcc get momo big.bin
/dcc list
/dcc close SEND 0
*/
