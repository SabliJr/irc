#ifndef SERVER_HPP
#define SERVER_HPP

#include "Client.hpp"
#include "Channel.hpp"
#include "Utils.hpp"

class Client;

class Channel {
	public:
		Channel(std::string name);
		~Channel();

		//! PUBLIC METHODS
		void 					addClient(Client *client);
		void 					removeClient(Client *client);
		void 					addOperator(Client *client);
		void 					removeOperator(Client *client);
		void 					broadcast(std::string message);

		//! SETTERS
		void 					setName(std::string name);
		void 					setTopic(std::string topic);
		void 					setPassword(std::string password);
		void 					setMaxUsers(int maxUsers);
		void 					setClientCount(int clientCount);

		//! GETTERS
		std::string 			getName();
		std::string 			getTopic();
		std::string 			getPassword();
		int						getMaxUsers();
		std::vector <Client *>	getClients();
		std::vector <Client *>	getOperators();
		int						getClientCount();

		

	private:
		std::string				_name;
		std::string				_password;
		std::string				_topic;
		int 					_clientCount;
		int						_maxUsers;
		std::vector <Client *>	_clients;
		std::vector <Client *>	_operators;
};

#endif
