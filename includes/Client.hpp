#include "Utils.hpp"

class Client {
	public:
		Client(int fd);
		~Client();

		// Getters
		int getSocketFd();
		std::string getIp();
		std::string getNickname();
		std::string getUsername();
		bool getAuthenticated();
		std::vector<std::string> getChannels();
		std::vector<std::string> getCommandArgs();
		std::string getReadbuffer();
		std::vector<std::string> getPendingCommands();

		// Setters
		void setNickname(std::string nickname);
		void setUsername(std::string username);
		void setAuthenticated(bool authenticated);
		void addChannel(std::string channel);
		void removeChannel(std::string channel);
		void setIp(std::string ip);
		void addPendingCommand(std::string command);
		void setReadbuffer(std::string readbuffer);

	private:
		int _fd;
		std::string _ip;
		std::string _nickname;
		std::string _hostname;
		std::string _username;
		bool _authenticated;
		std::vector<std::string> _channels;
		std::vector<std::string> _pending_commands;

		std::vector<std::string> _command_args;
		std::string _readbuffer;
};
