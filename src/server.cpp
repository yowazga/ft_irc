/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: yowazga <yowazga@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/05/20 19:20:58 by yowazga           #+#    #+#             */
/*   Updated: 2024/05/23 18:06:11 by yowazga          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/server.hpp"
#include <netdb.h>
#include "../include/Channel.hpp"


void Server::_initCommandHandlers(void)
{
	_commandHandlers["PASS"] = &Server::PASS;
	_commandHandlers["NICK"] = &Server::NICK;
	_commandHandlers["USER"] = &Server::USER;
	_commandHandlers["PING"] = &Server::PING;
	_commandHandlers["PONG"] = &Server::PING;
	
	_commandHandlers["LIST"] = &Server::LIST;
	_commandHandlers["JOIN"] = &Server::JOIN;
	
	_commandHandlers["PRIVMSG"] = &Server::PRIVMSG;
	
	_commandHandlers["WHO"] = &Server::WHO;

	_commandHandlers["WHOIS"] = &Server::WHOIS;
	
	_commandHandlers["PART"] = &Server::PART;
	
	_commandHandlers["QUIT"] = &Server::QUIT;
	_commandHandlers["KICK"] = &Server::KICK;

	_commandHandlers["TOPIC"] = &Server::TOPIC;
	_commandHandlers["INVITE"] = &Server::INVITE;
	_commandHandlers["NOTICE"] = &Server::PRIVMSG;
	_commandHandlers["ISON"] = &Server::ISON;
	_commandHandlers["MODE"] = &Server::MODE;
}


Server::Server(int port, const std::string &password) : _port(port), _password(password)
{
	init_server();
	_initCommandHandlers();
}

Server::~Server()
{
	close(_server_fd);
	for (size_t i = 0; i < _pollfds.size(); ++i)
		close(_pollfds[i].fd);
}

void Server::init_server()
{
	_server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (_server_fd < 0)
		throw (std::runtime_error("Failed to create socket: " + std::string(strerror(errno))));
		
	int opt = 1;
	if (setsockopt(_server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
		throw (std::runtime_error("Failed to set socket options to reuse address: " + std::string(strerror(errno))));

	if (fcntl(_server_fd, F_SETFL, O_NONBLOCK) == -1)
		throw std::runtime_error("Failed to set socket to non-blocking: " + std::string(strerror(errno)));
		
	struct sockaddr_in address;
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(_port);

	if (bind(_server_fd, (struct sockaddr*)&address, sizeof(address)) < 0)
		throw std::runtime_error("Failed to bind socket: " + std::string(strerror(errno)));

	if (listen(_server_fd, SOMAXCONN) < 0)
		throw std::runtime_error("Failed to listen on socket: " + std::string(strerror(errno)));
	
	struct pollfd server_poll_fd;
	server_poll_fd.fd = _server_fd;
	server_poll_fd.events = POLLIN | POLLERR | POLLHUP;
	_pollfds.push_back(server_poll_fd);
	
	std::cout << "Server started on " << "0.0.0.0" << ":" << _port << std::endl;
}

void Server::run()
{
	while (true)
	{
		int pollCount = poll(_pollfds.data(), _pollfds.size(), -1);
		if (pollCount < 0)
			throw (std::runtime_error("poll"));
		if (_pollfds[0].revents & POLLIN)
			handleNewConnection();
		for (size_t i = 1; i < _pollfds.size(); ++i)
		{
			if (_pollfds[i].revents & (POLLERR | POLLHUP))
				QUIT(_pollfds[i].fd, "Client disconnected");
			else if (_pollfds[i].revents & POLLIN)
				handleClientMessage(_pollfds[i].fd);
			else if (_pollfds[i].revents & POLLOUT)
				writeToClient(_pollfds[i].fd);
			
		}
	}
}

void Server::handleNewConnection()
{
	struct sockaddr_in clientAdd;
	socklen_t clientLen = sizeof(clientAdd);
	int client_fd = accept(_server_fd, (struct sockaddr *)&clientAdd, &clientLen);
	if (client_fd < 0)
		throw (std::runtime_error("accept"));

	if (fcntl(client_fd, F_SETFL, O_NONBLOCK) == -1)
	{
		close(client_fd);
		throw (std::runtime_error("Failed to set client socket to non-blocking: " + std::string(strerror(errno))));
	}
	
	struct pollfd clientPoll_fd;
	clientPoll_fd.fd = client_fd;
	clientPoll_fd.events = POLLIN | POLLERR | POLLHUP;
	_pollfds.push_back(clientPoll_fd);
	std::string clinet_ip = inet_ntoa(clientAdd.sin_addr);
	_clients[client_fd] = new Client(client_fd, clinet_ip, clinet_ip);
	std::cout << CMD_YELLOW << "New connection from " << clinet_ip << CMD_RESET << std::endl;
}


Client &Server::getClient(int socket)
{
	std::map<int, Client *>::iterator it = _clients.find(socket);
	if (it == _clients.end())
		throw std::runtime_error("Client not found in getClient");
	return *it->second;
}

Client &Server::getClient(std::string nickname)
{
	std::map<int, Client *>::iterator it = _clients.begin();
	for (; it != _clients.end() && it->second->getNickname() != nickname; it++)
		;
	if (it == _clients.end())
		throw std::runtime_error("Client not found in getClient");
	return *it->second;
}

void Server::removeClient(int socket)
{
	std::map<int, Client *>::iterator it = _clients.find(socket);
	if (it != _clients.end())
	{
		std::vector<Channel *> channels = getClientChannels(socket);
		for (size_t i = 0; i < channels.size(); i++)
		{
			setBackupOperator(*channels[i], getClient(socket), *this);
			channels[i]->removeClient(socket);
		}
		delete it->second;
		_clients.erase(it);
	}
	std::vector<pollfd>::iterator it2 = _pollfds.begin();
	for (; it2 != _pollfds.end() && it2->fd != socket; it2++);
	if (it2 != _pollfds.end())
		_pollfds.erase(it2);
	std::cout << CMD_YELLOW << "Client disconnected from socket " << socket << CMD_RESET << std::endl;
	close(socket);
}

/**
 * @brief Handles the incoming message from a client.
 * 
 * This function receives the message from the client specified by the file descriptor `client_fd`.
 * It reads the message into a buffer and appends it to the client's inbound buffer.
 * If the client's inbound buffer is ready with complete commands, it processes those commands.
 * 
 * @param client_fd The file descriptor of the client.
 */
void Server::handleClientMessage(int client_fd)
{
	char buffer[4096];
	int read_bytes;
	Client &client = getClient(client_fd);
	if ((read_bytes = recv(client_fd, buffer, 4096, 0)) <= 0)
		return;
	client.appendToInboundBuffer(std::string(buffer, read_bytes));
	if (client.inboundReady())
	{
		std::vector<std::string> commands = client.getCompleteCommands();
		processCommands(commands, client_fd);
	}
}

// ctrl +v ctrl +m -> ^M -> \r\n
pollfd& Server::getPollfd(int socket)
{
	for (size_t i = 0; i < _pollfds.size(); ++i)
	{
		if (_pollfds[i].fd == socket)
			return _pollfds[i];
	}
	throw std::runtime_error("Pollfd not found in getPollfd");
}

void Server::writeToClient(int socket)
{
	Client &client = getClient(socket);
	if (!client.outboundReady())
		return;
	std::string data = client.getOutboundBuffer();
	ssize_t bytes_sent;
	if ((bytes_sent = send(socket, data.c_str(), data.size(), 0)) == -1)
		return;
	client.advanceOutboundBuffer(bytes_sent);
	if (!client.outboundReady()) // no more data to send
		getPollfd(socket).events &= ~POLLOUT; //nand: disable POLLOUT -> POLLIN | POLLERR | POLLHUP
}


void Server::sendMessageToClient(int client_fd, const std::string &message)
{
	Client &client = getClient(client_fd);
	client.newMessage(message);
	std::cout << CMD_BLUE << ">>>>> Sending into socket " << client_fd << ": " << CMD_RESET << message << std::endl;
	getPollfd(client_fd).events |= POLLOUT; //or: append
}


std::string Server::prefix(void)
{
	return ":ircserv ";
}

void Server::processCommands(std::vector<std::string> commands, int client_fd)
{
	std::vector<std::string>::iterator it = commands.begin();
	Client &client = getClient(client_fd);
	while (it < commands.end())
	{
		std::cout << CMD_GREEN << "<<<<< Received from socket " << client_fd << ": " << CMD_RESET << *it << std::endl;
		std::string command_name;
		std::string command_args;
		std::stringstream ss(*it);
		ss >> command_name >> std::ws;
		std::transform(command_name.begin(), command_name.end(), command_name.begin(), ::toupper);
		std::getline(ss, command_args, '\0');
		if (command_name != "PASS" && !client.isAuthenticated())
			sendMessageToClient(client_fd, prefix() + "451 : You have not registered");
		else if (command_name != "PASS" && command_name != "NICK" && command_name != "USER" && !client.isRegistered())
			sendMessageToClient(client_fd, prefix() + "451 : You have not registered");
		else if (_commandHandlers.find(command_name) == _commandHandlers.end())
			sendMessageToClient(client_fd, prefix() + "421 " + command_name + " : Unknown command");
		else if (command_name == "QUIT")
			QUIT(client_fd, command_args);
		else
			(this->*_commandHandlers[command_name])(client_fd, command_args);
		++it;
	}
}

void Server::createChannel(std::string name,std::string pass, std::string topic)
{
	std::string key = name;
	if (key[0] == '#')
		key = key.substr(1);
	for (size_t i = 0; i < key.size(); i++)
		key[i] = std::tolower(key[i]); // make channel name case-insensitive
	if (_channels.find(key) != _channels.end())
		throw std::runtime_error("Channel already exists");
	Channel *channel = new Channel(name, pass, this);
	channel->setTopic(topic);
	_channels.insert(std::make_pair(key, channel));
}

Channel &Server::getChannel(std::string name)
{
	if (name[0] == '#')
		name = name.substr(1);
	for (size_t i = 0; i < name.size(); i++)
		name[i] = std::tolower(name[i]); // make channel name case-insensitive
	std::map<std::string, Channel *>::iterator it = _channels.find(name);
	if (it == _channels.end())
		throw std::runtime_error("Channel not found in getChannel");
	return *it->second;
}

std::vector<Channel *> Server::getClientChannels(int socket)
{
	std::vector<Channel *> channels;
	std::map<std::string, Channel *>::iterator it = _channels.begin();
	for (; it != _channels.end(); it++)
		if (it->second->hasClient(socket))
			channels.push_back(it->second);
	return channels;
}

void Server::sendMessageToClientChannels(int socket, std::string message)
{
	std::vector<Channel *> channels = getClientChannels(socket);
	for (size_t i = 0; i < channels.size(); i++)
		channels[i]->broadcast(message, socket);
}

