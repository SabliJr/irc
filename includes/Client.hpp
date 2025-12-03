#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <iostream>
#include "Utils.hpp"

class Client {
	public:
		Client(int fd);
		~Client();

		// Getters
		int 						getSocketFd();
		std::string 				getIp();
		std::string 				getNickname();
		std::string 				getUsername();
		bool 						getAuthenticated();
		std::vector<std::string> 	getChannels();
		std::vector<std::string> 	getCommandArgs();
		std::string 				getReadbuffer();
		std::vector<std::string> 	getPendingCommands();
		bool						getHasNick();
		bool						getHasUser();
		bool						getHasPass();

		// Setters
		void 						setNickname(std::string nickname);
		void 						setUsername(std::string username);
		void 						setAuthenticated(bool authenticated);
		void 						addChannel(std::string channel);
		void 						removeChannel(std::string channel);
		void 						setIp(std::string ip);
		void 						addPendingCommand(std::string command);
		void 						setReadbuffer(std::string readbuffer);
		void 						setHasNick(bool hasNick);
		void 						setHasUser(bool hasUser);
		void 						setHasPass(bool hasPass);

	private:
		int 						_fd;
		std::string 				_ip;
		std::string 				_nickname;
		std::string 				_username;
		bool						_hasNick;
		bool						_hasUser;
		bool						_hasPass;
		bool 						_authenticated;
		std::vector<std::string> 	_channels;
		std::vector<std::string> 	_pending_commands;

		std::vector<std::string> 	_command_args;
		std::string 				_readbuffer;
};


#endif
