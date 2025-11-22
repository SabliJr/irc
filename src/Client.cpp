#include "../includes/Client.hpp"

Client::Client(int fd) : _fd(fd), _ip(""), _nickname(""), _username(""), _hasNick(false), \
						_hasUser(false), _hasPass(false), _authenticated(false), _channels(std::vector<std::string>()), \
						_readbuffer("") {}

Client::~Client() {}


//! GETTERS
int Client::getSocketFd() {
	return this->_fd;
}

std::string Client::getIp() {
	return this->_ip;
}

std::string Client::getNickname() {
	return this->_nickname;
}

std::string Client::getUsername() {
	return this->_username;
}

bool Client::getAuthenticated() {
	return this->_authenticated;
}

std::vector<std::string> Client::getChannels() {
	return this->_channels;
}

std::string Client::getReadbuffer() {
	return this->_readbuffer;
}

bool Client::getHasNick() {
	return this->_hasNick;
}

bool Client::getHasUser() {
	return this->_hasUser;
}

bool Client::getHasPass() {
	return this->_hasPass;
}

//! SETTERS

void Client::setReadbuffer(std::string readbuffer) {
	this->_readbuffer = readbuffer;
}

void Client::setNickname(std::string nickname) {
	this->_nickname = nickname;
}

void Client::setUsername(std::string username) {
	this->_username = username;
}

void Client::setAuthenticated(bool authenticated) {
	this->_authenticated = authenticated;
}

void Client::setIp(std::string ip) {
	this->_ip = ip;
}

void Client::setHasNick(bool hasNick) {
	this->_hasNick = hasNick;
}

void Client::setHasUser(bool hasUser) {
	this->_hasUser = hasUser;
}

void Client::setHasPass(bool hasPass) {
	this->_hasPass = hasPass;
}

//! OTHER METHODS

void Client::addChannel(std::string channel) {
	this->_channels.push_back(channel);
}

std::vector<std::string> Client::getPendingCommands() {
	return this->_pending_commands;
}

void Client::addPendingCommand(std::string command) {
	this->_pending_commands.push_back(command);
}
