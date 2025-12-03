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

#define LOGS "sabli"

/*
================================================================================
                            IRSSI TEST COMMANDS CHEATSHEET
================================================================================

1. CONNECTION
	/connect localhost 8888 pa           (Connect with password 'pa')
	/connect localhost 8888 pa myNick    (Connect with specific nickname)

2. CHANNEL OPERATIONS
	/join #general                       (Join a channel)
	/join #secret myPassword             (Join a channel with password/key)
	/part #general                       (Leave a channel)

3. MESSAGING
	Hello everyone!                      (Public Message - type in channel)
	/msg otherUser Hello there!          (Private Message / DM)
	/notice otherUser This is a notice   (Notice)

4. OPERATOR COMMANDS (Requires @ status)
	/kick #general badUser               (Kick a user)
	/kick #general badUser Get out!      (Kick with reason)
	/invite otherUser #general           (Invite user - useful for +i)
	/topic #general New Topic            (Change Topic)
	/topic #general                      (View Topic)

5. CHANNEL MODES
	/mode #general +o otherUser          (Make Operator)
	/mode #general -o otherUser          (Remove Operator)
	/mode #general +k secretpass         (Set Channel Password)
	/mode #general -k                    (Remove Channel Password)
	/mode #general +l 5                  (Set User Limit)
	/mode #general -l                    (Remove User Limit)
	/mode #general +i                    (Set Invite Only)
	/mode #general -i                    (Remove Invite Only)
	/mode #general +t                    (Set Topic Protection)
	/mode #general -t                    (Remove Topic Protection)

6. FILE TRANSFER (DCC)
	/dcc send otherUser /path/to/file    (Send a file)
	/dcc get myNick file.txt             (Receive a file)




   * How to create a dummy file for testing:
    echo hello > /tmp/hi.txt
    dd if=/dev/zero of=/tmp/big.bin bs=1M count=10

7. BOT COMMANDS (If implemented)
	/msg MEE6 !hello
	/msg MEE6 !42

8. QUIT
   /quit Bye everyone!
================================================================================
*/

