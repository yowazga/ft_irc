/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: yowazga <yowazga@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/05/21 19:40:15 by yowazga           #+#    #+#             */
/*   Updated: 2024/05/23 18:04:31 by yowazga          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */


#pragma once

#include <iostream>     // For input/output operations
#include <string>       // For string manipulation
#include <vector>       // For dynamic arrays
#include <map>          // For key-value pairs
#include <sys/types.h>  // For system data types
#include <sys/socket.h> // For socket operations
#include <netinet/in.h> // For internet address family
#include <arpa/inet.h>  // For internet operations
#include <unistd.h>     // For POSIX API
#include <fcntl.h>      // For file control operations
#include <poll.h>       // For polling file descriptors
#include <csignal>

#include "../include/Client.hpp"

class Channel;

#define CMD_BLUE "\033[0;34m"
#define CMD_GREEN "\033[0;32m"
#define CMD_RED "\033[0;31m"
#define CMD_YELLOW "\033[0;33m"
#define CMD_RESET "\033[0m"

class Server
{
	private:
		int _server_fd;
		int _port;
		std::string _password;
		std::vector<struct pollfd> _pollfds;
		std::map<int, Client*> _clients;
		std::map<std::string, Channel *>		_channels;

		typedef void (Server::*commandHandler)(int, std::string);
		std::map<std::string, commandHandler>	_commandHandlers;

		void init_server();
		void handleNewConnection();
		void handleClientMessage(int client_fd);
		void writeToClient(int);
		void _initCommandHandlers(void);

	public:
		Server(int port, const std::string &password);
		~Server();
		void run();
		Client&		getClient(int); // by fd
		Client&		getClient(std::string); // by nickname
		pollfd& 	getPollfd(int socket);
		void		removeClient(int);

		void		processCommands(std::vector<std::string> commands, int client_fd);
		std::string prefix(void);
		void		sendMessageToClient(int client_fd, const std::string &message);

		void		createChannel(std::string, std::string, std::string = "No topic"); // "No topic
		Channel&	getChannel(std::string);
		std::vector<Channel *> getClientChannels(int);
		void		sendMessageToClientChannels(int, std::string);
		
		void		registerNewClient(int);
		void		PASS(int, std::string);
		void		NICK(int, std::string);
		void		USER(int, std::string);
		void		PING(int, std::string);
		void		LIST(int, std::string);
		void		JOIN(int, std::string);
		void		PART(int, std::string);
		void		WHO(int, std::string);
		void		WHOIS(int, std::string);
		void		PRIVMSG(int, std::string);
		void		QUIT(int, std::string);
		void		KICK(int, std::string);
		void		TOPIC(int, std::string);
		void		INVITE(int, std::string);
		void		NOTICE(int, std::string);
		void		ISON(int, std::string);
		void		MODE(int, std::string);
};



void setBackupOperator(Channel& channel, Client& target, Server& server);