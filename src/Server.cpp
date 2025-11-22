#include "../includes/Server.hpp"

bool Server::_signal = false;
// std::vector<Client> Server::_clients;

Server::Server() : _port(0), _serverSocketFd(-1), _password("")
{
	//! REMOVE THE BS
	std::cout << "Server constructor called" << std::endl;
	std::cout << "Port: " << _port << std::endl;
	std::cout << "Password: " << _password << std::endl;
}

Server::Server(int port, std::string password) : _port(port), _password(password)
{
	std::cout << "Port: " << _port << std::endl;
	std::cout << "Password: " << _password << std::endl;

	serverInit();
}

Server::~Server() {}

void Server::SignalHandler(int signum)
{
	std::cout << BLUE << "Signal received: " << signum << RESET << std::endl;
	_signal = true;
}

void Server::serverInit()
{
	std::cout << GREEN << "[LOGS]" << RESET << "serverInit called" << std::endl;
	createSocket();

	while (this->_signal == false)
	{
		int ret = poll(this->_pollFds.data(), this->_pollFds.size(), -1); //! Explain poll what it does in the program
		if (ret == -1 && this->_signal == false)
		{
			throw std::runtime_error("poll() failed");
		}
		for (size_t i = 0; i < this->_pollFds.size(); i++)
		{
			if (this->_pollFds[i].revents & POLLIN)
			{
				if (this->_pollFds[i].fd == this->_serverSocketFd)
				{
					acceptClient();
				}
				else
				{
					Client *client = getClientByFd(this->_pollFds[i].fd);
					if (client != NULL)
					{
						handleMessage(this->_pollFds[i].fd, client);
						std::cout << "WE ARE HERE 1" << std::endl;
					}
					else
					{
						std::cout << "Unknown client fd: " << this->_pollFds[i].fd << std::endl;
					}
				}
			}
		}
	}
	closeSockets(); //! Need to code this
}

void Server::sendNonBlockingCommand(int fd, const std::string &message)
{
	ssize_t totalSent = 0;
	ssize_t messageLength = message.length();
	const char *msgPtr = message.c_str();

	while (totalSent < messageLength) {
		ssize_t bytesSent = send(fd, msgPtr + totalSent, messageLength - totalSent, 0);
		if (bytesSent == -1) {
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
				// Socket is not ready for sending, try again later
				continue;
			} else {
				std::cout << "send() failed in sendNonBlockingCommand: " << strerror(errno) << std::endl;
				//! Exit program cleanly
				return;
			}
		}
		totalSent += bytesSent;
	}
}

void Server::clearClients(int fd)
{
	std::cout << GREEN << "[LOGS]" << RESET << "clearClients() called" << RESET << std::endl;
	for (size_t i = 0; i < this->_clients.size(); i++)
	{
		if (this->_clients[i].getSocketFd() == fd)
		{
			this->_clients.erase(this->_clients.begin() + i);
			break;
		}
	}
	for (size_t i = 0; i < this->_pollFds.size(); i++)
	{
		if (this->_pollFds[i].fd == fd)
		{
			this->_pollFds.erase(this->_pollFds.begin() + i);
			break;
		}
	}
	std::cout << "Client [" << fd << "] cleared" << std::endl;
}

// void Server::parseCommand(Client *client, const std::string &line)
// {
// 	// std::cout << GREEN << "parseCommand called" << RESET << std::endl;

// 	// std::cout << "[parseCommand] fd=" << client->getSocketFd() << " line=" << line << std::endl;
// 	(void)client;

// 	// (void)line;
// 	std::cout << line << std::endl;
// }

struct CommandHandler {
	std::string command;
	void (Server::*handler)(Client *, const std::string &);
};

void Server::handleJoin(Client *client, const std::string &cmd)
{
	std::cout << GREEN << "[LOGS]" << RESET << "handleJoin called: " << cmd << std::endl;

	size_t pos = cmd.find(' ');
	if (pos == std::string::npos)
	{
		std::string errorMsg = "461 " + client->getNickname() + " JOIN :Not enough parameters\r\n";
		sendNonBlockingCommand(client->getSocketFd(), errorMsg);
		return;		
	}
	std::string channelName = cmd.substr(pos + 1);
	// Check if channel already exists
	for (size_t i = 0; i < this->_channels.size(); i++)
	{
		if (this->_channels[i].getName() == channelName)
		{
			if (this->_channels[i].getMaxUsers() > 0 &&
				static_cast<int>(this->_channels[i].getClients().size()) >= this->_channels[i].getMaxUsers())
			{
				std::string errorMsg = "471 " + client->getNickname() + " " + channelName + " :Channel is full\r\n";
				sendNonBlockingCommand(client->getSocketFd(), errorMsg);
				return;
			}
			this->_channels[i].addClient(client);
			std::string joinMsg = ":" + client->getNickname() + " JOIN " + channelName + "\r\n";
			sendNonBlockingCommand(client->getSocketFd(), joinMsg);
			return;
		}
	}
	// If channel does not exist, create a new one and add the client
	Channel newChannel(channelName);
	newChannel.addClient(client);
	std::string joinMsg = ":" + client->getNickname() + " JOIN " + channelName + "\r\n";
	this->_channels.push_back(newChannel);

	sendNonBlockingCommand(client->getSocketFd(), joinMsg);
}

void Server::handlePrivmsg(Client *client, const std::string &cmd)
{
    std::cout << GREEN << "[LOGS]" << RESET << "handlePrivmsg called: " << cmd << std::endl;

    // Parse: PRIVMSG <target> :<message>
    size_t firstSpace = cmd.find(' ');
    if (firstSpace == std::string::npos)
        return;
    size_t secondSpace = cmd.find(' ', firstSpace + 1);
    if (secondSpace == std::string::npos)
        return;

    std::string target = cmd.substr(firstSpace + 1, secondSpace - firstSpace - 1);
    std::string message = cmd.substr(secondSpace + 1);

    // Remove leading ':' from message if present
    if (!message.empty() && message[0] == ':')
        message = message.substr(1);

    // Check if target is a channel (starts with '#')
    if (!target.empty() && target[0] == '#')
    {
        for (size_t i = 0; i < this->_channels.size(); i++)
        {
            if (this->_channels[i].getName() == target)
            {
                std::string privmsg = ":" + client->getNickname() + " PRIVMSG " + target + " :" + message + "\r\n";
                this->_channels[i].broadcast(privmsg);
                return;
            }
        }
        // Channel not found
        std::string errorMsg = "401 " + client->getNickname() + " " + target + " :No such channel\r\n";
        sendNonBlockingCommand(client->getSocketFd(), errorMsg);
    }
}

void Server::handlePart(Client *client, const std::string &cmd)
{
	std::cout << GREEN << "[LOGS]" << RESET << "handlePart called: " << cmd << std::endl;

	size_t pos = cmd.find(' ');
	if (pos == std::string::npos)
	{
        std::string errorMsg = "461 " + client->getNickname() + " PART :Not enough parameters\r\n";
		sendNonBlockingCommand(client->getSocketFd(), errorMsg);
		return;
	}
	std::string channelName = cmd.substr(pos + 1);

	for (size_t i = 0; i < this->_channels.size(); i++)
	{
		if (this->_channels[i].getName() == channelName)
		{
			this->_channels[i].removeClient(client);
			std::string partMsg = ":" + client->getNickname() + " PART " + channelName + "\r\n";
			sendNonBlockingCommand(client->getSocketFd(), partMsg);
			return;
		}
	}
	std::string errorMsg = "403 " + client->getNickname() + " " + channelName + " :No such channel\r\n";
	sendNonBlockingCommand(client->getSocketFd(), errorMsg);
}

void Server::handleMode(Client *client, const std::string &cmd)
{
	std::cout << GREEN << "[LOGS]" << RESET << "handleMode called: " << cmd << std::endl;
    // Silently acknowledge MODE, e.g. MODE <nick> +i
    // You can send a reply or do nothing
    // Example: send current mode
	(void)cmd; // To avoid unused parameter warning
    std::string reply = ":ircserv 324 " + client->getNickname() + " " + client->getNickname() + " +i\r\n";\

	//! Use sendNonBlockingCommand instead?
	sendNonBlockingCommand(client->getSocketFd(), reply);
}

void Server::handlePing(Client *client, const std::string &cmd)
{
	std::cout << GREEN << "[LOGS]" << RESET << "handlePing called: " << cmd << std::endl;
    // Reply with PONG
    size_t pos = cmd.find(' ');
    std::string param = (pos != std::string::npos) ? cmd.substr(pos + 1) : "";
    std::string pongMsg = "PONG " + param + "\r\n";
    sendNonBlockingCommand(client->getSocketFd(), pongMsg);
}

void Server::commandRouter(Client *client, const std::string &cmd)
{
	std::cout << GREEN << "[LOGS]" << RESET << "commandRouter() called: " << cmd << std::endl;
	if (cmd.empty())
		return;

	static CommandHandler handlers[] = {
		{"JOIN", &Server::handleJoin},
		{"PRIVMSG", &Server::handlePrivmsg},
		{"PART", &Server::handlePart},
		//? Optionnal to silently ignore the commands we dont need to handle
		{"MODE", &Server::handleMode},
		{"PING", &Server::handlePing},
	};

	for (size_t i = 0; i < sizeof(handlers)/sizeof(handlers[0]); i++)
	{
		if (strncmp(cmd.c_str(), handlers[i].command.c_str(), handlers[i].command.length()) == 0) {
			(this->*handlers[i].handler)(client, cmd);
			return;
		}
	}

	std::string errorMsg = "421 " + client->getNickname() + " " + cmd.substr(0, cmd.find(' ')) + " :Unknown command\r\n";
	sendNonBlockingCommand(client->getSocketFd(), errorMsg);
}


void Server::handleMessage(int fd, Client *client)
{
    std::cout << GREEN << "[LOGS]" << RESET << "handleMessage called" << std::endl;

    char buffer[1024];
    memset(buffer, 0, sizeof(buffer));
    ssize_t bytesReceived = recv(fd, buffer, sizeof(buffer) - 1, 0);

    if (bytesReceived <= 0)
    {
        if (bytesReceived == 0)
        {
            std::cout << "Client " << fd << " disconnected" << std::endl;
        }
        else if (errno != EAGAIN && errno != EWOULDBLOCK)
        {
            std::cout << "recv() error on client " << fd << ": " << strerror(errno) << std::endl;
        }
        clearClients(fd);
        close(fd);
        return;
    }

    if (bytesReceived > 512)
    {
        std::cout << "Message is too long" << std::endl;
        return;
    }

    buffer[bytesReceived] = '\0';

    client->setReadbuffer(client->getReadbuffer() + buffer);

    while (1)
    {
        size_t pos = client->getReadbuffer().find("\r\n");
        if (pos == std::string::npos)
        {
            break;
        }

        std::string command = client->getReadbuffer().substr(0, pos);
        client->setReadbuffer(client->getReadbuffer().substr(pos + 2));

        if (!client->getAuthenticated())
        {
            handleRegistrationMessage(*client, command);
            
            // Check if client still exists after registration message
            bool clientExists = false;
            for (size_t i = 0; i < this->_clients.size(); i++)
            {
                if (&this->_clients[i] == client)
                {
                    clientExists = true;
                    break;
                }
            }
            if (!clientExists)
            {
                return; // <-- EXIT if client was cleared
            }
        }
        else
        {
            std::cout << RED << "Authenticated command: " << command << RESET << std::endl;
            commandRouter(client, command);
        }
    }
}

// void Server::handleMessage(int fd, Client *client)
// {
// 	std::cout << GREEN << "[LOGS]" << RESET << "handleMessage called" << std::endl;

// 	char buffer[1024];
// 	memset(buffer, 0, sizeof(buffer));
// 	ssize_t bytesReceived = recv(fd, buffer, sizeof(buffer) - 1, 0);

// 	if (bytesReceived <= 0)
// 	{
// 		if (bytesReceived == 0)
// 		{
// 			std::cout << "Client " << fd << " disconnected" << std::endl;
// 		}
// 		else if (errno != EAGAIN && errno != EWOULDBLOCK)
// 		{
// 			std::cout << "recv() error on client " << fd << ": " << strerror(errno) << std::endl;
// 		}
// 		clearClients(fd);
// 		close(fd);
// 		return;
// 	}

// 	if (bytesReceived > 512)
// 	{
// 		std::cout << "Message is too long" << std::endl;
// 		return;
// 	}

// 	buffer[bytesReceived] = '\0';

// 	client->setReadbuffer(client->getReadbuffer() + buffer);

// 	while (1)
// 	{
// 		size_t pos = client->getReadbuffer().find("\r\n");
// 		if (pos == std::string::npos)
// 		{
// 			break;
// 		}

// 		std::string command = client->getReadbuffer().substr(0, pos);
// 		client->setReadbuffer(client->getReadbuffer().substr(pos + 2));

// 		if (!client->getAuthenticated())
// 		{
// 			handleRegistrationMessage(*client, command);
// 			std::cout << "WE ARE HERE 2" << std::endl;
// 		}
// 		else
// 		{
// 			std::cout << RED << "Authenticated command: " << command << RESET << std::endl;
// 			commandRouter(client, command);
// 		}
// 	}
	
// 	//$ removed because no flood control
// 	// const std::vector<std::string> &pendingCommands = client->getPendingCommands();
// 	// for (size_t i = 0; i < pendingCommands.size(); i++)
// 	// {
// 	// 	parseCommand(client, pendingCommands[i]);
// 	// }
// 	//! Do this function later clearPendingCommands
// 	// client->clearPendingCommands();
// }

void Server::handleRegistrationMessage(Client &client, const std::string &cmd)
{
	std::cout << GREEN << "[LOGS]" << RESET << "handleRegistrationMessage() called: " << cmd << std::endl;

	if (strncmp(cmd.c_str(), "PASS", 4) == 0 && !client.getHasPass())
	{
		handlePass(&client, cmd);
	}
	else if (strncmp(cmd.c_str(), "NICK", 4) == 0 && !client.getHasNick())
	{
		handleNick(&client, cmd);
	}
	else if (strncmp(cmd.c_str(), "USER", 4) == 0 && !client.getHasUser())
	{
		handleUser(&client, cmd);
	}
	else if (strncmp(cmd.c_str(), "CAP", 3) == 0)
	{
		handleCap(&client, cmd);
	}

	if (client.getHasPass() && client.getHasNick() && client.getHasUser())
    {
        // Check if nickname is unique among authenticated clients
        for (size_t i = 0; i < this->_clients.size(); i++)
        {
            if (&this->_clients[i] != &client && 
                this->_clients[i].getNickname() == client.getNickname() &&
                this->_clients[i].getAuthenticated())
            {
                // Nickname is not unique, do not authenticate
                std::cout << GREEN << "[LOGS]" << RESET << "Nickname conflict: " << client.getNickname() << std::endl;
                return;
            }
        }
        client.setAuthenticated(true);
        std::string welcomeMsg = "001 " + client.getNickname() + " :Welcome to the IRC server\r\n";
        sendNonBlockingCommand(client.getSocketFd(), welcomeMsg);
    }
}

void Server::handleCap(Client *client, const std::string &cmd)
{
    std::cout << GREEN << "[LOGS]" << RESET << "handleCap called: " << cmd << std::endl;
    if (cmd == "CAP LS" || cmd.find("CAP LS") == 0)
    {
        std::string response = ": ircserv CAP " + client->getNickname() + " LS :\r\n";
        sendNonBlockingCommand(client->getSocketFd(), response);
    }
    else
    {
        std::string response = "CAP * ACK :capabilities acknowledged\r\n";
        sendNonBlockingCommand(client->getSocketFd(), response);
    }
}

void Server::handlePass(Client *client, const std::string &cmd)
{
	std::cout << GREEN << "[LOGS]" << RESET << "handlePass called: " << cmd << std::endl;
	// Extract the password from the command
	size_t pos = cmd.find(' ');
	if (pos == std::string::npos)
	{
		std::string errorMsg = "461 " + client->getNickname() + " PASS :Not enough parameters\r\n";
		sendNonBlockingCommand(client->getSocketFd(), errorMsg);
		return;		
	}
	std::string password = cmd.substr(pos + 1);
	// Here you would check the password against the server's expected password
	int fd = client->getSocketFd();
	if (this->_password == password)
	{
		client->setHasPass(true);
	} else {
		std::string errorMsg = "464 " + client->getNickname() + " :Password incorrect\r\n";
		sendNonBlockingCommand(client->getSocketFd(), errorMsg);
		clearClients(fd);
		close(fd);
		return;
	}
}

void Server::handleNick(Client *client, const std::string &cmd)
{
    std::cout << GREEN << "[LOGS]" << RESET << "handleNick called: " << cmd << std::endl;

    size_t pos = cmd.find(' ');
    if (pos == std::string::npos)
    {
        std::string errorMsg = "431 :No nickname given\r\n";
        sendNonBlockingCommand(client->getSocketFd(), errorMsg);
        return;		
    }
    std::string nickname = cmd.substr(pos + 1);

    // Check if the nickname is already in use (don't check authenticated status)
	int fd = client->getSocketFd();
    for (size_t i = 0; i < this->_clients.size(); i++)
    {
        if (&this->_clients[i] != client && 
            this->_clients[i].getNickname() == nickname)
        {
            std::string errorMsg = "433 " + nickname + " :Nickname is already in use\r\n";
            sendNonBlockingCommand(client->getSocketFd(), errorMsg);
			clearClients(fd);
			close(fd);
            return; // Do NOT set nickname
        }
    }
    client->setNickname(nickname);
    client->setHasNick(true);
}

void Server::handleUser(Client *client, const std::string &cmd)
{
	std::cout << GREEN << "[LOGS]" << RESET << "handleUser() called: " << cmd << std::endl;
	// Extract the username from the command
	size_t pos = cmd.find(' ');
	if (pos == std::string::npos)
	{
		std::string errorMsg = "461 " + client->getNickname() + " USER :Not enough parameters\r\n";
		sendNonBlockingCommand(client->getSocketFd(), errorMsg);
		return;
	}
	std::string username = cmd.substr(pos + 1);
	// Divide into 4 parts: username, hostname, servername, realname and do the checks for real name must start with ':'
	// split by space and check if there are at least 4 parts
	std::vector<std::string> parts;
	size_t start = 0;
	while (start < username.length())
	{
		size_t end = username.find(' ', start);
		if (end == std::string::npos)
		{
			parts.push_back(username.substr(start));
			break;
		}
		parts.push_back(username.substr(start, end - start));
		start = end + 1;
	}
	if (parts.size() < 4)
	{
		std::string errorMsg = "461 " + client->getNickname() + " USER :Not enough parameters\r\n";
		sendNonBlockingCommand(client->getSocketFd(), errorMsg);
		return;
	}
	// check the real name starts with ':'
	if (parts[3][0] != ':')
	{
		std::string errorMsg = "461 " + client->getNickname() + " USER :Not enough parameters\r\n";
		sendNonBlockingCommand(client->getSocketFd(), errorMsg);
		return;
	}
	client->setUsername(parts[0]);
	client->setHasUser(true);
}

Client *Server::getClientByFd(int fd)
{
	for (size_t i = 0; i < _clients.size(); i++)
	{
		if (_clients[i].getSocketFd() == fd)
		{
			return &_clients[i];
		}
	}
	return NULL;
}


/*
1 Check the PASS
2 Check the NICK if not already used
3 Check the USER 
*/
void Server::acceptClient()
{
	std::cout << GREEN << "[LOGS]" << RESET << "acceptClient() called" << std::endl;



	struct sockaddr_in clientAddr;
	socklen_t clientAddrLen = sizeof(clientAddr);

	int clientFd = accept(_serverSocketFd, (struct sockaddr *)&clientAddr, &clientAddrLen);
	if (clientFd == -1)
	{
		throw std::runtime_error("Failed to accept client: accept() failed");
	}

	Client cli(clientFd);
	cli.setIp(inet_ntoa(clientAddr.sin_addr));
	if (cli.getIp() == "")
	{
		throw std::runtime_error("Failed to get client IP: inet_ntoa() failed");
	}

	if (fcntl(cli.getSocketFd(), F_SETFL, O_NONBLOCK) == -1)
	{
		throw std::runtime_error("Failed to set socket option to non-blocking: fcntl() failed");
	}

	struct pollfd clientPollFd;
	clientPollFd.fd = cli.getSocketFd();
	clientPollFd.events = POLLIN;
	clientPollFd.revents = 0;

	this->_pollFds.push_back(clientPollFd);
	this->_clients.push_back(cli);
	std::cout << "Client accepted: " << cli.getSocketFd() << std::endl;
}


void Server::closeSockets()
{
	std::cout << GREEN << "[LOGS]" << RESET << "closeSockets() called" << std::endl;
	for (size_t i = 0; i < this->_pollFds.size(); i++)
	{
		close(this->_pollFds[i].fd);
	}
	this->_pollFds.clear();
}

void Server::createSocket()
{
	std::cout << GREEN << "[LOGS]" << RESET << "createSocket() called" << std::endl;
	int en = 1;
	struct sockaddr_in add;
	struct pollfd serverPollFd;

	add.sin_family = AF_INET;
	add.sin_addr.s_addr = INADDR_ANY;
	add.sin_port = htons(this->_port);

	this->_serverSocketFd = socket(AF_INET, SOCK_STREAM, 0);
	if (this->_serverSocketFd == -1)
	{
		throw std::runtime_error("Failed to create socket: socket() failed");
	}

	if (setsockopt(this->_serverSocketFd, SOL_SOCKET, SO_REUSEADDR, &en, sizeof(en)) == -1)
	{
		throw std::runtime_error("Failed to set socket options: setsockopt() failed");
	}

	if (fcntl(this->_serverSocketFd, F_SETFL, O_NONBLOCK) == -1)
	{
		throw std::runtime_error("Failed to set socket option to non-blocking: fcntl() failed");
	}

	if (bind(this->_serverSocketFd, (struct sockaddr *)&add, sizeof(add)) == -1)
	{
		throw std::runtime_error("Failed to bind socket: bind() failed");
	}

	if (listen(this->_serverSocketFd, SOMAXCONN) == -1)
	{
		throw std::runtime_error("Failed to listen on socket: listen() failed");
	}

	//! Re explain this
	serverPollFd.fd = this->_serverSocketFd;
	serverPollFd.events = POLLIN;
	serverPollFd.revents = 0;

	this->_pollFds.push_back(serverPollFd);
	std::cout << "Socket created and listening on port " << this->_port << "\n\n"
			  << std::endl;
}

/*
struct sockaddr_in {
	sa_family_t     sin_family;
	in_port_t       sin_port;
	struct  in_addr sin_addr;
	char            sin_zero[8];
};
*/

/*
struct pollfd {
	int fd;
	short events;
	short revents;
};
*/

/*
1. socket()      → Create socket
2. setsockopt()  → Set SO_REUSEADDR (allow port reuse)
3. fcntl()       → Set O_NONBLOCK (non-blocking mode)
4. bind()        → Bind to address (IP + port)
5. listen()      → Start listening for connections
6. accept()      → Accept incoming connections (next step)
*/
