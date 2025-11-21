#include "../includes/Server.hpp"

void Server::handlePass(Client *client, const std::string cmd)
{
	std::cout << GREEN << "handlePass called" << RESET << std::endl;
	if (cmd.size() < 2)
	{
		std::string errorMsg = "461 " + client->getNickname() + " PASS :Not enough parameters\r\n";
		send(client->getSocketFd(), errorMsg.c_str(), errorMsg.length(), 0);
		return;
	}
	if (cmd == this->_password)
	{
		client->setAuthenticated(true);
		std::string successMsg = "001 " + client->getNickname() + " :Welcome to the IRC server\r\n";
		send(client->getSocketFd(), successMsg.c_str(), successMsg.length(), 0);
	}
	else
	{
		std::string errorMsg = "464 " + client->getNickname() + " :Password incorrect\r\n";
		send(client->getSocketFd(), errorMsg.c_str(), errorMsg.length(), 0);
	}
}
