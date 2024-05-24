/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   IRCLogic.cpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: yowazga <yowazga@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/05/16 17:15:13 by yowazga           #+#    #+#             */
/*   Updated: 2024/05/23 18:04:02 by yowazga          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/server.hpp"
#include "../include/Client.hpp"
#include "../include/Channel.hpp"

/**
 * Authenticates a client by checking the provided password against the server's password.
 * If the client is already authenticated, it sends an error message.
 * If the provided password is incorrect, it sends an error message.
 * Otherwise, it sets the client as authenticated.
 *
 * @param socket The socket of the client.
 * @param password The password provided by the client.
 */
void Server::PASS(int socket, std::string password)
{
	Client &client = getClient(socket);
	if (client.isAuthenticated())
		sendMessageToClient(socket, prefix() + "462 You may not reregister");
	else if (password != _password)
		sendMessageToClient(socket, prefix() + "464 Invalid password");
	else
		client.setAuthenticated(true);
}

/**
 * Registers a new client with the server.
 * Sets the client's registered flag to true and sends welcome messages to the client.
 *
 * @param socket The socket of the new client.
 */
void Server::registerNewClient(int socket)
{
	Client& client = getClient(socket);
	client.setRegistered(true);
	sendMessageToClient(socket, prefix() + "001 " + client.getNickname() + " : Welcome " + client.getNetworkIdentifier());
	sendMessageToClient(socket, prefix() + "002 " + client.getNickname() + " : Your host is " + "ircserv" + ", running version 1.0");
	sendMessageToClient(socket, prefix() + "003 " + client.getNickname() + " : This server was created 1970/01/01 00:00:00");
	sendMessageToClient(socket, prefix() + "004 " + client.getNickname() + " : ircserv 1.0 o o");
	sendMessageToClient(socket, prefix() + "005 " + client.getNickname() + " : CHANMODES=s,k,l,i,t :are supported by this server"); // mandatory only
	sendMessageToClient(socket, prefix() + "375 " + client.getNickname() + " : - " + "ircserv" + " MOTD - ");
	sendMessageToClient(socket, prefix() + "372 " + client.getNickname() + " : - Welcome to the Internet Relay Network!");
	sendMessageToClient(socket, prefix() + "372 " + client.getNickname() + " : - Please remember to respect the members and follow the rules.");
	sendMessageToClient(socket, prefix() + "372 " + client.getNickname() + " : - Enjoy your stay!");
	sendMessageToClient(socket, prefix() + "376 " + client.getNickname() + " : End of /MOTD command.");
}

/**
 * Handles the NICK command for the server.
 * 
 * @param socket The socket of the client.
 * @param nickname The desired nickname for the client.
 */
void Server::NICK(int socket, std::string nickname)
{
	Client &client = getClient(socket);
	if (nickname.size() < 1 || nickname.size() > 9
	|| nickname.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789[]\\`_^{|}-") != std::string::npos
	|| nickname.find_first_of("0123456789-", 0, 1) == 0)
	{
		sendMessageToClient(socket, prefix() + "432 " + nickname + " : Erroneous nickname");
		return;
	}
	try
	{
		getClient(nickname);
		sendMessageToClient(socket, prefix() + "433 " + nickname + " : Nickname is already in use");
	}
	catch (std::runtime_error &)
	{
		std::stringstream broadcast;
		broadcast << client.prefix() << "NICK " << nickname;
		if (client.getUsername() != "" && !client.isRegistered())
			registerNewClient(socket);
		client.setNickname(nickname);
		sendMessageToClientChannels(socket, broadcast.str());
	}
}

/**
 * Handles the USER command from the client.
 * This command is used to specify the user's username and realname.
 * The format of the command is: USER <username> 0 * :<realname>
 * 
 * @param socket The socket of the client.
 * @param args The arguments passed with the command.
 */
void Server::USER(int socket, std::string args)
{
	Client &client = getClient(socket);
	std::string username;
	std::string skip;
	std::string realname;
	// parse arguments here
	// format: <username> 0 * :<realname>
	std::stringstream ss(args);
	ss >> username >> std::ws;
	std::getline(ss,skip, ':'); // skip the 0 and * (unused fields)
	ss >> std::ws; // skip whitespace
	std::getline(ss, realname, '\0');
	if (username.size() < 1 || username.size() > 12
		|| username.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789") != std::string::npos
		|| username.find_first_of("0123456789", 0, 1) == 0)
	{
		sendMessageToClient(socket, prefix() + "432 " + username + " : Erroneous username");
		return;
	}
	if (!realname.empty() 
		&& (realname.size() > 50 
			|| realname.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789[]\\`_^{|}- ") != std::string::npos))
	{
		sendMessageToClient(socket, prefix() + "501 " + realname + " : Invalid realname");
		return;
	}
	std::stringstream broadcast;
	broadcast << client.prefix() << "USER " << args;

	client.setUsername(username);
	client.setRealname(realname);
	if (client.getNickname() != "" && !client.isRegistered())
		registerNewClient(socket);
	sendMessageToClientChannels(socket, broadcast.str());
}

/**
 * Sends a PONG message to the client.
 * 
 * @param socket The socket of the client.
 * @param args The arguments received from the client.
 */
void Server::PING(int socket, std::string args)
{
	sendMessageToClient(socket, prefix() + "PONG " + args);
}

/**
 * Sends a list of channels and their information to the client.
 *
 * @param socket The socket of the client.
 * @param param Unused parameter.
 */
void Server::LIST(int socket, std::string)
{
	Client &client = getClient(socket);

	sendMessageToClient(socket, prefix() + "321 " + client.getNickname() + " Channel : Users Name");
	std::map<std::string, Channel *>::iterator it = _channels.begin();
	for (; it != _channels.end(); it++)
	{
		std::stringstream ss;
		Channel& cn = *it->second;
		if (cn.getMode(ChanSecret) && !cn.hasClient(socket))
			continue; // skip secret channels if not a member of it
			
		ss << prefix() << "322 " << client.getNickname() << " " << cn.getName() << " " << cn.getClientCount()  << " : " << cn.getTopic();
		sendMessageToClient(socket, ss.str());
	}
	sendMessageToClient(socket, prefix() + "323 " + client.getNickname() + " : End of /LIST");
}

/**
 * Joins a client to a channel.
 * 
 * @param socket The socket of the client.
 * @param args The arguments passed to the JOIN command.
 */
void Server::JOIN(int socket, std::string args)
{
	Client &client = getClient(socket);
	std::stringstream ss(args);
	std::string channel_name;
	std::string channel_pass;
	ss >> channel_name >> std::ws;
	ss >> channel_pass;
	if (channel_name.empty())
	{
		sendMessageToClient(socket, prefix() + "461 JOIN : Not enough parameters");
		return;
	}
	if (channel_name[0] == '#')
		channel_name = channel_name.substr(1);
	// validate channel name
	if (channel_name.size() < 1 || channel_name.size() > 20
		|| channel_name.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_") != std::string::npos
		|| channel_name.find_first_of("0123456789_", 0, 1) == 0)
	{
		sendMessageToClient(socket, prefix() + "403 " + channel_name + " : No such channel");
		return;
	}
	channel_name = "#" + channel_name;
	try
	{
		Channel &channel = getChannel(channel_name);
		channel_name = channel.getName();
		if (channel.getMode(ChannelKey) && (channel_pass.empty() || channel_pass != channel.getPass()))
		{
			sendMessageToClient(socket, prefix() + "475 " + channel_name + " : Cannot join channel (+k)");
			return;
		}
		if (channel.getMode(ChanInviteOnly) && !channel.hasInvite(socket))
		{
			sendMessageToClient(socket, prefix() + "473 " + channel_name + " : Cannot join channel (+i)");
			return;
		}
		if (channel.getMode(ChanLimit) && channel.getClientCount() >= channel.getLimit())
		{
			sendMessageToClient(socket, prefix() + "471 " + channel_name + " : Cannot join channel (+l)");
			return;
		}
		channel.addClient(socket);
		channel.removeInvite(socket);
		// if the first client to join the channel, set the channel operator
		if (channel.getClientCount() == 1)
			channel.addOperator(socket);
		channel.broadcast(client.prefix() + "JOIN " + channel_name);
		// send channel topic, names list, and channel modes
		sendMessageToClient(socket, prefix() + "332 " + client.getNickname() + " " + channel_name + " : " + channel.getTopic());
		sendMessageToClient(socket, prefix() + "353 " + client.getNickname() + " = " + channel_name + " : " + channel.getclientsNicknames());
		sendMessageToClient(socket, prefix() + "324 " + client.getNickname() + " " + channel_name + " " + channel.getModeString());
		
	}
	catch (std::runtime_error &)
	{
		createChannel(channel_name, channel_pass);
		Channel &channel = getChannel(channel_name);
		channel.addClient(socket);
		channel.addOperator(socket);
		channel.broadcast(client.prefix() + "JOIN " + channel_name);
		sendMessageToClient(socket, prefix() + "331 " + client.getNickname() + " " + channel_name + " :No topic is set");
		sendMessageToClient(socket, prefix() + "353 " + client.getNickname() + " = " + channel_name + " : " + channel.getclientsNicknames());
		sendMessageToClient(socket, prefix() + "324 " + client.getNickname() + " " + channel_name + " " + channel.getModeString());

	}
}

/**
 * Sends a private message to a target client or channel.
 * 
 * @param socket The socket of the client sending the message.
 * @param args The arguments containing the target and message.
 */
void Server::PRIVMSG(int socket, std::string args)
{
	Client &client = getClient(socket);

	std::string target;
	std::string message;
	std::stringstream ss(args);

	ss >> target >> std::ws;
	std::getline(ss, message, '\0');
	if (target.empty())
	{
		sendMessageToClient(socket, prefix() + "411 PRIVMSG : No recipient given");
		return;
	}
	if (message.empty())
	{
		sendMessageToClient(socket, prefix() + "412 PRIVMSG : No text to send");
		return;
	}
	try
	{
		if (target[0] == '#')
		{
			Channel &channel = getChannel(target);
			if (!channel.hasClient(socket)
			|| (channel.getMode(ChanModerated) && (!channel.isOperator(socket) && !channel.hasVoice(socket))))
			{
				sendMessageToClient(socket, prefix() + "404 " + target + " : Cannot send to channel");
				return;
			}
			channel.broadcast(client.prefix() + "PRIVMSG " + target + " " + message, socket);
		}
		else
		{
			Client &target_client = getClient(target);
			sendMessageToClient(target_client.getSocket(), client.prefix() + "PRIVMSG " + target + " " + message);
		}
	}
	catch (...)
	{
		sendMessageToClient(socket, prefix() + "401 PRIVMSG : No such target");
		return;
	}
}

/**
 * Sends a WHO command response to the client.
 * 
 * @param socket The socket of the client.
 * @param args The arguments passed with the WHO command.
 */
void Server::WHO(int socket, std::string args)
{
	Client &client = getClient(socket);
	std::stringstream ss(args);
	std::string target;
	ss >> target;
	if (target.empty())
	{
		sendMessageToClient(socket, prefix() + "431 WHO : No target given");
		return;
	}
	try {
		Channel &channel = getChannel(args);
		const std::vector<int>& clients = channel.getClients();
		for (size_t i = 0; i < clients.size(); i++) {
			Client& c = getClient(clients[i]);
			std::string flags = "H";
			if (channel.isOperator(c.getSocket())) {
    			flags += "@";
			}
			std::stringstream msgline;
			msgline << prefix() << "352 ";
			msgline << client.getNickname() << " " << channel.getName() << " ";
			msgline << c.getUsername() << " " << c.getHostname() << " ";
			msgline << "*" << " " << c.getNickname() << " " << flags << " ";
			msgline << ":0 " << c.getRealname();
			sendMessageToClient(socket, msgline.str());
		}
		sendMessageToClient(socket, prefix() + "315 " + client.getNickname() + " " + channel.getName() + " : End of /WHO list");
	}
	catch (...) {
		sendMessageToClient(socket, prefix() + "403 " + target + " : No such channel");
	}
}

/**
 * Sends a WHOIS response to the client.
 * 
 * @param socket The socket of the client.
 * @param args The arguments passed to the WHOIS command.
 */
void Server::WHOIS(int socket, std::string args)
{
	Client &client = getClient(socket);
	std::stringstream ss(args);
	std::string target;
	ss >> target;
	if (target.empty())
	{
		sendMessageToClient(socket, prefix() + "431 WHOIS : No target given");
		return;
	}
	try {
		Client &target_client = getClient(target);
		std::stringstream msgline;
		msgline << prefix() << "311 " << client.getNickname() << " " << target << " " << target_client.getUsername() << " " << target_client.getHostname() << " * : " << target_client.getRealname();
		sendMessageToClient(socket, msgline.str());
	}
	catch (...) {
		sendMessageToClient(socket, prefix() + "401 " + target + " : No such target");
	}
}


void setBackupOperator(Channel& channel, Client& target, Server& server)
{
	int socket = target.getSocket();
	if (channel.isOperator(socket) && channel.getOperatorCount() == 1)
	{
		// set another admin if the last one leaves
		std::vector<int> clients = channel.getClients();
		for (size_t i = 0; i < clients.size(); i++)
		{
			if (clients[i] != socket)
			{
				channel.addOperator(clients[i]);
				channel.broadcast(target.prefix() + "MODE " + channel.getName() + " +o " + server.getClient(clients[i]).getNickname());
				break;
			}
		}
	}
}


/**
 * @brief Handles the PART command for a client on the server.
 * 
 * This function is responsible for processing the PART command, which allows a client to leave a channel.
 * It checks if the channel exists and if the client is currently a member of the channel.
 * If the channel and client are valid, it broadcasts a PART message to all other clients in the channel
 * and removes the client from the channel.
 * If the channel does not exist, it sends an error message to the client.
 * If the command is not provided with enough parameters, it sends an error message to the client as well.
 * 
 * @param socket The socket of the client.
 * @param args The arguments provided with the PART command.
 */
void Server::PART(int socket, std::string args)
{
	Client &client = getClient(socket);
	std::stringstream ss(args);
	std::string channel_name;
	ss >> channel_name;
	if (channel_name.empty())
	{
		sendMessageToClient(socket, prefix() + "461 PART : Not enough parameters");
		return;
	}
	try
	{
		Channel &channel = getChannel(channel_name);
		if (!channel.hasClient(socket))
		{
			sendMessageToClient(socket, prefix() + "442 " + channel_name + " : You're not on that channel");
			return;
		}
		setBackupOperator(channel, client, *this);
		channel.broadcast(client.prefix() + "PART " + args);
		channel.removeClient(socket);
	}
	catch (std::runtime_error &)
	{
		sendMessageToClient(socket, prefix() + "403 " + channel_name + " : No such channel");
		return;
	}
}

/**
 * @brief Handles the QUIT command for a client.
 * 
 * This function is responsible for processing the QUIT command received from a client.
 * It broadcasts a QUIT message to all channels the client is a member of and removes the client from the server.
 * 
 * @param socket The socket of the client.
 * @param args The arguments provided with the QUIT command.
 */
void Server::QUIT(int socket, std::string args)
{
	Client &client = getClient(socket);
	std::stringstream broadcast;
	broadcast << client.prefix() << "QUIT : " << args;
	sendMessageToClientChannels(socket, broadcast.str());
	removeClient(socket);
}

/**
 * Sets or retrieves the topic of a channel.
 * 
 * @param socket The socket of the client.
 * @param args The arguments passed to the command.
 */
void Server::TOPIC(int socket, std::string args)
{
	Client &client = getClient(socket);
	std::stringstream ss(args);
	std::string channel_name;
	std::string topic;
	ss >> channel_name >> std::ws;
	std::getline(ss, topic, '\0');
	if (channel_name.empty())
	{
		sendMessageToClient(socket, prefix() + "461 TOPIC : Not enough parameters");
		return;
	}
	try
	{
		Channel &channel = getChannel(channel_name);
		if (!channel.hasClient(socket))
		{
			sendMessageToClient(socket, prefix() + "442 " + channel_name + " : You're not on that channel");
			return;
		}
		if (topic.empty())
		{
			sendMessageToClient(socket, prefix() + "331 " + client.getNickname() + " " + channel_name + " : " + channel.getTopic());
			return;
		}
		if (channel.getMode(ChanTopicProtected) && !channel.isOperator(socket))
		{
			sendMessageToClient(socket, prefix() + "482 " + channel_name + " : You're not channel operator");
			return;
		}
		channel.setTopic(topic); 
		channel.broadcast(client.prefix() + "TOPIC " + channel_name + " : " + topic);
	}
	catch (std::runtime_error &)
	{
		sendMessageToClient(socket, prefix() + "403 " + channel_name + " : No such channel");
		return;
	}
}

/**
 * Kicks a client from a channel.
 * 
 * @param socket The socket of the client performing the kick.
 * @param args The arguments passed to the KICK command.
 */
void Server::KICK(int socket, std::string args)
{
	Client &client = getClient(socket);
	std::stringstream ss(args);
	std::string channel_name;
	std::string target;
	std::string reason;
	ss >> channel_name >> target >> std::ws;
	std::getline(ss, reason, '\0');
	if (channel_name.empty() || target.empty())
	{
		sendMessageToClient(socket, prefix() + "461 KICK : Not enough parameters");
		return;
	}
	try
	{
		Channel &channel = getChannel(channel_name);
		if (!channel.hasClient(socket))
		{
			sendMessageToClient(socket, prefix() + "442 " + channel_name + " : You're not on that channel");
			return;
		}
		if (!channel.isOperator(socket))
		{
			sendMessageToClient(socket, prefix() + "482 " + channel_name + " : You're not channel operator");
			return;
		}
		Client &target_client = getClient(target);
		if (!channel.hasClient(target_client.getSocket()))
		{
			sendMessageToClient(socket, prefix() + "441 " + target + " " + channel_name + " : They aren't on that channel");
			return;
		}
		std::stringstream broadcast;
		broadcast << client.prefix() << "KICK " << channel_name << " " << target << " : " << reason;
		channel.broadcast(broadcast.str());
		channel.removeClient(target_client.getSocket());
	}
	catch (std::runtime_error &)
	{
		sendMessageToClient(socket, prefix() + "403 " + channel_name + " : No such channel");
		return;
	}
}

/**
 * Sends an invitation to a target client to join a specified channel.
 * 
 * @param socket The socket of the client sending the invitation.
 * @param args The arguments containing the target client and channel name.
 */
void Server::INVITE(int socket, std::string args)
{
	Client &client = getClient(socket);
	std::stringstream ss(args);
	std::string target;
	std::string channel_name;
	ss >> target >> channel_name;
	if (target.empty() || channel_name.empty())
	{
		sendMessageToClient(socket, prefix() + "461 INVITE : Not enough parameters");
		return;
	}
	try
	{
		Client &target_client = getClient(target);
		Channel &channel = getChannel(channel_name);
		if (!channel.hasClient(socket))
		{
			sendMessageToClient(socket, prefix() + "442 " + channel_name + " : You're not on that channel");
			return;
		}
		if (channel.hasClient(target_client.getSocket()))
		{
			sendMessageToClient(socket, prefix() + "443 " + target + " " + channel_name + " : is already on channel");
			return;
		}
		sendMessageToClient(target_client.getSocket(), client.prefix() + "INVITE " + target + " " + channel_name);
		channel.addInvite(target_client.getSocket());
	}
	catch (std::runtime_error &)
	{
		sendMessageToClient(socket, prefix() + "403 " + channel_name + " : NO such channel/User");
		return;
	}
}

/**
 * Checks if a given nickname is currently online.
 * Sends a response to the client with the list of online nicknames.
 *
 * @param socket The socket of the client making the request.
 * @param args   The arguments passed to the command.
 */
void Server::ISON(int socket, std::string args)
{
	Client &client = getClient(socket);
	std::stringstream ss(args);
	std::string nickname;
	ss >> nickname;
	if (nickname.empty())
	{
		sendMessageToClient(socket, prefix() + "461 ISON : Not enough parameters");
		return;
	}
	std::vector<std::string> nicks;
	std::stringstream response;
	response << prefix() << "303 " << client.getNickname();
	while (!ss.eof())
	{
		try
		{
			Client &target = getClient(nickname);
			response << " " << target.getNickname();
		}
		catch (std::runtime_error &)
		{
			response << " " << nickname;
		}
		ss >> nickname;
	}
	sendMessageToClient(socket, response.str());
}

/**
 * Sets the mode of a channel or performs mode-related operations for a client.
 * 
 * @param socket The socket of the client.
 * @param args The arguments passed to the MODE command.
 */
void Server::MODE(int socket, std::string args)
{
	Client &client = getClient(socket);
	std::stringstream ss(args);
	std::string target;
	std::string mode;
	std::string mode_args;
	ss >> target >> mode >> mode_args;
	if (target.empty())
	{
		sendMessageToClient(socket, prefix() + "461 MODE : Not enough parameters");
		return;
	}
	Channel *channel;
	try
	{
		channel = &getChannel(target);
	}
	catch (std::runtime_error &)
	{
		sendMessageToClient(socket, prefix() + "403 MODE : No such channel");
		return;
	}
	if (mode.empty())
	{
		sendMessageToClient(socket, prefix() + "324 " + client.getNickname() + " " + channel->getName() + " " + channel->getModeString());
		return;
	}
	if (!channel->hasClient(socket) || !channel->isOperator(socket))
	{
		sendMessageToClient(socket, prefix() + "482 MODE : You're not channel operator");
		return;
	}
	
	bool add = true;
	if (mode[0] == '+' || mode[0] == '-')
	{
		add = mode[0] == '+';
		mode = mode.substr(1);
	}
	if (mode.length() != 1)
	{
		sendMessageToClient(socket, prefix() + "472 MODE : malformatted mode");
		return;
	}
	switch (mode[0])
	{
		case 'i': // invite only
			channel->setMode(ChanInviteOnly, add);
			break;
		case 'm': // moderated: only ops and voiced users can write
			channel->setMode(ChanModerated, add);
			break;
		case 't': // topic protected
				channel->setMode(ChanTopicProtected, add);
			break;
		case 's': // secret: not shown in list
			channel->setMode(ChanSecret, add);
			break;
		case 'k': // channel key
			if (mode_args.empty() && add)
			{
				sendMessageToClient(socket, prefix() + "461 MODE : Not enough parameters");
				return;
			}
			channel->setMode(ChannelKey, add);
			if (add)
				channel->setPass(mode_args);
			break;
		case 'v': // voice permission: can talk in moderated cahannels
			if (mode_args.empty())
			{
				sendMessageToClient(socket, prefix() + "461 MODE : Not enough parameters");
				return;
			}
			try
			{
				Client &target_client = getClient(mode_args);
				if (add)
					channel->addVoice(target_client.getSocket());
				else
					channel->removeVoice(target_client.getSocket());
			}
			catch (std::runtime_error &)
			{
				sendMessageToClient(socket, prefix() + "401 MODE : No such target");
				return;
			}
			break;
		case 'o': // grant/revoke operator status
			if (mode_args.empty())
			{
				sendMessageToClient(socket, prefix() + "461 MODE : Not enough parameters");
				return;
			}
			try
			{
				Client &target_client = getClient(mode_args);
				if (add)
					channel->addOperator(target_client.getSocket());
				else
					channel->removeOperator(target_client.getSocket());
			}
			catch (std::runtime_error &)
			{
				sendMessageToClient(socket, prefix() + "401 MODE : No such target");
				return;
			}
			break;
		case 'l': // channel users limit
			if (mode_args.empty() && add)
			{
				sendMessageToClient(socket, prefix() + "461 MODE : Not enough parameters");
				return;
			}
			if (mode_args.find_first_not_of("0123456789") != std::string::npos)
			{
				sendMessageToClient(socket, prefix() + "472 MODE : malformatted mode");
				return;
			}
			if (add)
				channel->setLimit(std::stoi(mode_args));
			else
				channel->setLimit(0);
			break;
		default:
			sendMessageToClient(socket, prefix() + "472 MODE : Unknown mode");
			return;
	}
	channel->broadcast(client.prefix() + "MODE " + channel->getName() + " " + (add ? "+" : "-") + mode + " " + (mode == "k" ? "********" : mode_args));
}
