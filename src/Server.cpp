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
    // 1. Find the client pointer
    Client* clientToRemove = getClientByFd(fd);
    if (!clientToRemove) return;

    // 2. Remove this client from ALL channels
    for (size_t i = 0; i < this->_channels.size(); i++)
    {
        // You need a method in Channel to force remove a client by pointer
        // regardless of whether they sent a PART command
        this->_channels[i].removeClient(clientToRemove); 
        this->_channels[i].removeOperator(clientToRemove);
        this->_channels[i].removeInvitedClient(clientToRemove);
    }

    // 3. Remove from pollfds
    for (size_t i = 0; i < this->_pollFds.size(); i++)
    {
        if (this->_pollFds[i].fd == fd)
        {
            this->_pollFds.erase(this->_pollFds.begin() + i);
            break;
        }
    }

    // 4. Remove from clients list
	this->_clients.erase(fd);
	close(fd);
}

// void Server::parseCommand(Client *client, const std::string &line)
// {
// 	// std::cout << GREEN << "parseCommand called" << RESET << std::endl;

// 	// std::cout << "[parseCommand] fd=" << client->getSocketFd() << " line=" << line << std::endl;
// 	(void)client;

// 	// (void)line;
// 	std::cout << line << std::endl;
// }


/* Checking before adding to a channel
1 check if +i mode is set and if the client is invited
2 check if +k mode is set and if the client provided the correct password
3 check if +l mode is set and if the channel is full
*/
bool Server::canJoinChannel(Channel &channel, Client *client, const std::string &password)
{
	// +i mode
	if (channel.isInviteOnly() && !channel.isInvited(client)) {
		std::string errorMsg = "473 " + client->getNickname() + " " + channel.getName() + " :Cannot join channel (+i)\r\n";
		sendNonBlockingCommand(client->getSocketFd(), errorMsg);
		return false;
	}

	// +k mode
	if (channel.isPasswordProtected() && channel.getPassword() != password) {
		std::string errorMsg = "475 " + client->getNickname() + " " + channel.getName() + " :Cannot join channel (+k)\r\n";
		sendNonBlockingCommand(client->getSocketFd(), errorMsg);
		return false;
	}

	// +l mode
	if (channel.isUserLimitEnabled() && channel.getClientCount() >= channel.getMaxUsers()) {
		std::string errorMsg = "471 " + client->getNickname() + " " + channel.getName() + " :Cannot join channel (+l)\r\n";
		sendNonBlockingCommand(client->getSocketFd(), errorMsg);
		return false;
	}
	return true;
}

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
    std::string password = "";
    size_t passPos = channelName.find(' ');
    if (passPos != std::string::npos)
    {
        password = channelName.substr(passPos + 1);
        channelName = channelName.substr(0, passPos);
    }
    
    // Check if channel already exists
    for (size_t i = 0; i < this->_channels.size(); i++)
    {
        if (this->_channels[i].getName() == channelName)
        {
            if (!canJoinChannel(this->_channels[i], client, password)) {
                return;
            }
            this->_channels[i].addClient(client);
            std::string joinMsg = ":" + client->getNickname() + " JOIN " + channelName + "\r\n";
            this->_channels[i].broadcast(joinMsg, NULL); // Broadcast to all including the joiner
            
            // Send names list to the joining client
            sendNamesList(client, &this->_channels[i]);
            return;
        }
    }
    
    // If channel does not exist, create a new one and add the client
    Channel newChannel(channelName);
    newChannel.addClient(client);
    newChannel.addOperator(client);
    this->_channels.push_back(newChannel);
    
    std::string joinMsg = ":" + client->getNickname() + " JOIN " + channelName + "\r\n";
    this->_channels.back().broadcast(joinMsg, NULL);
    
    // Send names list to the joining client
    sendNamesList(client, &this->_channels.back());
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
                this->_channels[i].broadcast(privmsg, client);
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

			if (this->_channels[i].getClients().empty())
			{
				this->_channels.erase(this->_channels.begin() + i);
			}
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

/* TOPIC
To VIEW - /topic #channel
To SET - /topic #channel New topic here -> SERVER RECEIVES: TOPIC #channel :New topic here
*/
void Server::handleTopic(Client *client, const std::string &cmd)
{
	std::cout << GREEN << "[LOGS]" << RESET << "handleTopic called: " << cmd << std::endl;

	std::istringstream iss(cmd);
	std::string command, channelName, topic;
	iss >> command >> channelName;

	size_t topicPos = cmd.find(" :", command.find(channelName));
	if (topicPos != std::string::npos) {
		topic = cmd.substr(topicPos + 2); // Skip " :"
	} else {
		topic = ""; // No topic provided, just viewing
	}

	Channel* channel = NULL;
	for (size_t i = 0; i < this->_channels.size(); i++) {
		if (this->_channels[i].getName() == channelName) {
			channel = &this->_channels[i];
			break;
		}
	}
	if (!channel) {
		std::string errorMsg = "403 " + client->getNickname() + " " + channelName + " :No such channel\r\n";
		sendNonBlockingCommand(client->getSocketFd(), errorMsg);
		return;
	}

	if (topic.empty()) { //! View only
		std::string reply = "332 " + client->getNickname() + " " + channelName + " :" + channel->getTopic() + "\r\n";
		sendNonBlockingCommand(client->getSocketFd(), reply);
		return;
	}

	// Check +t is set
	if (channel->isTopicProtected() && !channel->isOperator(client)) {
		std::string errorMsg = "482 " + client->getNickname() + " " + channelName + " :You're not channel operator\r\n";
		sendNonBlockingCommand(client->getSocketFd(), errorMsg);
		return;
	}

	channel->setTopic(topic);
	std::string topicMsg = ":" + client->getNickname() + " TOPIC " + channelName + " :" + topic + "\r\n";
	channel->broadcast(topicMsg, NULL);
}

// INVITE nick #channel
void Server::handleInvite(Client *client, const std::string &cmd)
{
	std::cout << GREEN << "[LOGS]" << RESET << "handleInvite called: " << cmd << std::endl;

	std::istringstream iss(cmd);
	std::string command, targetNick, channelName;
	iss >> command >> targetNick >> channelName;

	Channel* channel = NULL;
	for (size_t i = 0; i < this->_channels.size(); i++) {
		if (this->_channels[i].getName() == channelName) {
			channel = &this->_channels[i];
			break;
		}
	}
	if (!channel) {
		std::string errorMsg = "403 " + client->getNickname() + " " + channelName + " :No such channel\r\n";
		sendNonBlockingCommand(client->getSocketFd(), errorMsg);
		return;
	}

	if (!channel->isOperator(client)) {
		std::string errorMsg = "482 " + client->getNickname() + " " + channelName + " :You're not channel operator\r\n";
		sendNonBlockingCommand(client->getSocketFd(), errorMsg);
		return;
	}

	Client* targetClient = NULL;
	for (std::map<int, Client>::iterator it = _clients.begin(); it != _clients.end(); ++it) {
		if (it->second.getNickname() == targetNick) {
			targetClient = &(it->second);
			break;
		}
	}
	if (!targetClient) {
		std::string errorMsg = "401 " + client->getNickname() + " " + targetNick + " :No such nick/channel\r\n";
		sendNonBlockingCommand(client->getSocketFd(), errorMsg);
		return;
	}

	channel->addInvitedClient(targetClient);
	std::string inviteMsg = ":" + client->getNickname() + " INVITE " + targetNick + " " + channelName + "\r\n";
	sendNonBlockingCommand(targetClient->getSocketFd(), inviteMsg);
}



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

struct OperatorCommandHandler {
	std::string command;
	void (Server::*handler)(Client *, const std::string &);
};

static OperatorCommandHandler operatorHandlers[] = {
	{"KICK",   &Server::handleKick},
	{"INVITE", &Server::handleInvite},
	{"TOPIC",  &Server::handleTopic},
	{"MODE",   &Server::handleChannelMode}, // For channel modes only
};

void Server::operatorCommandRouter(Client *client, const std::string &cmd)
{
	for (size_t i = 0; i < sizeof(operatorHandlers)/sizeof(operatorHandlers[0]); i++)
	{
		if (strncmp(cmd.c_str(), operatorHandlers[i].command.c_str(), operatorHandlers[i].command.length()) == 0) {
			(this->*operatorHandlers[i].handler)(client, cmd);
			return;
		}
	}
	std::string errorMsg = "421 " + client->getNickname() + " " + cmd.substr(0, cmd.find(' ')) + " :Unknown command\r\n";
	sendNonBlockingCommand(client->getSocketFd(), errorMsg);																		
}


// KICK #channel targetNick [reason - optionnal]
void Server::handleKick(Client *client, const std::string &cmd) 
{
	std::cout << GREEN << "[LOGS]" << RESET << "handleKick called: " << cmd << std::endl;

	std::istringstream iss(cmd);
	std::string command, channelName, targetNick, reason;
	iss >> command >> channelName >> targetNick;
	std::getline(iss,reason);
	if (!reason.empty() && reason[0] == ' ') {
		reason = reason.substr(1); // Remove the space
	}

	Channel* channel = NULL;
	for (size_t i = 0; i < this->_channels.size(); i++) {
		if (this->_channels[i].getName() == channelName) {
			channel = &this->_channels[i];
			break;
		}
	}
	if (!channel) {
		std::string errorMsg = "403 " + client->getNickname() + " " + channelName + " :No such channel\r\n";
		sendNonBlockingCommand(client->getSocketFd(), errorMsg);
		return;
	}

	if (!channel->isOperator(client)) {
		std::string errorMsg = "482 " + client->getNickname() + " " + channelName + " :You're not channel operator\r\n";
		sendNonBlockingCommand(client->getSocketFd(), errorMsg);
		return;
	}

	Client* targetClient = NULL;
	std::vector<Client*> channelClients = channel->getClients();
	for (size_t i = 0; i < channelClients.size(); i++) {
		if (channelClients[i]->getNickname() == targetNick) {
			targetClient = channelClients[i];
			break;
		}
	}
	if (!targetClient) {
		std::string errorMsg = "441 " + client->getNickname() + " " + targetNick + " " + channelName + " :They aren't on that channel\r\n";
		sendNonBlockingCommand(client->getSocketFd(), errorMsg);
		return;
	}

	std::string kickMsg = ":" + client->getNickname() + " KICK " + channel->getName() + " " + targetClient->getNickname() + " :" + reason + "\r\n";
	sendNonBlockingCommand(targetClient->getSocketFd(), kickMsg);
	channel->removeClient(targetClient);
	channel->broadcast(kickMsg, NULL);
}


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

void Server::handleChannelMode(Client *client, const std::string &cmd)
{
	std::cout << GREEN << "[LOGS]" << RESET << "handleChannelMode called: " << cmd << std::endl;

	std::istringstream iss(cmd);
	std::string command, channelName, modeStr, param;
	iss >> command >> channelName >> modeStr;
	if (modeStr.find("+o") == 0 || modeStr.find("-o") == 0) {
		iss >> param; // nick
	} else if (modeStr.find("+k") == 0) {
		iss >> param; // password
	} else if (modeStr.find("+l") == 0) {
		iss >> param; // limit
	}

	Channel* channel = NULL;
	for (size_t i = 0; i < this->_channels.size(); i++) {
		if (this->_channels[i].getName() == channelName) {
			channel = &this->_channels[i];
			break;
		}
	}
	if (!channel) {
		std::string errorMsg = "403 " + client->getNickname() + " " + channelName + " :No such channel\r\n";
		sendNonBlockingCommand(client->getSocketFd(), errorMsg);
		return;
	}
	if (!channel->isOperator(client)) {
		std::string errorMsg = "482 " + client->getNickname() + " " + channelName + " :You're not channel operator\r\n";
		sendNonBlockingCommand(client->getSocketFd(), errorMsg);
		return;
	}
	channel->setMode(modeStr, param, client);

	std::string modeMsg = ":" + client->getNickname() + " MODE " + channelName + " " + modeStr;
	if (!param.empty()) {
		modeMsg += " " + param;
	}
	modeMsg += "\r\n";
	channel->broadcast(modeMsg, NULL);
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

void Server::sendNamesList(Client *client, Channel *channel)
{
	std::string namesList = "";
	std::vector<Client*> clients = channel->getClients();

    for (size_t i = 0; i < clients.size(); i++)
    {
        // Add '@' prefix for operators
        if (channel->isOperator(clients[i])) {
            namesList += "@";
        }
        namesList += clients[i]->getNickname();
        
        // Add space between names (not after the last one)
        if (i < clients.size() - 1) {
            namesList += " ";
        }
    }
    
    // 353 RPL_NAMREPLY
    std::string reply = ":ircserv 353 " + client->getNickname() + " = " + channel->getName() + " :" + namesList + "\r\n";
    sendNonBlockingCommand(client->getSocketFd(), reply);
    
    // 366 RPL_ENDOFNAMES
    std::string endReply = ":ircserv 366 " + client->getNickname() + " " + channel->getName() + " :End of /NAMES list\r\n";
    sendNonBlockingCommand(client->getSocketFd(), endReply);
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
            // Since _clients is now a map, we use find(fd)
            if (_clients.find(fd) == _clients.end())
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
		for (std::map<int, Client>::iterator it = this->_clients.begin(); it != this->_clients.end(); ++it)
		{
			if (&(it->second) != &client && it->second.getNickname() == client.getNickname() && it->second.getAuthenticated())
			{
				std::string errorMsg = "433 " + client.getNickname() + " :Nickname is already in use\r\n";
				sendNonBlockingCommand(client.getSocketFd(), errorMsg);
				clearClients(client.getSocketFd());
				close(client.getSocketFd());
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
	for (std::map<int, Client>::iterator it = this->_clients.begin(); it != this->_clients.end(); ++it)
	{
		if (&(it->second) != client && it->second.getNickname() == nickname)
		{
			std::string errorMsg = "433 " + nickname + " :Nickname is already in use\r\n";
			sendNonBlockingCommand(client->getSocketFd(), errorMsg);
			clearClients(fd);
			close(fd);
			return;
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
	std::map <int, Client>::iterator it = _clients.find(fd);
	if (it != _clients.end()) {
		return &(it->second);
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
	this->_clients.insert(std::make_pair(cli.getSocketFd(), cli));
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
