/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: zmoumen <zmoumen@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/05/20 19:21:01 by yowazga           #+#    #+#             */
/*   Updated: 2024/05/23 16:59:55 by zmoumen          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/server.hpp"



void arguments_validator(std::string port, std::string password)
{
	if (port.empty() || password.empty())
		throw (std::runtime_error("Error: port and password are required"));
	if (port.find_first_not_of("0123456789") != std::string::npos)
		throw (std::runtime_error("Error: port must be a number"));
	if (port.size() > 5 || std::stoi(port) < 1024 || std::stoi(port) > 65535)
		throw (std::runtime_error("Error: port must be between 1024 and 65535"));
	if (password.size() < 8)
		throw (std::runtime_error("Error: password must be at least 8 characters"));
}


int main(int ac, char **av)
{
	if (ac != 3)
	{
        std::cerr << "Usage: " << av[0] << " <port> <password>" << std::endl;
        return 1;
    }
	try
	{
		if (signal(SIGPIPE, SIG_IGN) == SIG_ERR)
			throw (std::runtime_error("Error: signal"));
		arguments_validator(av[1], av[2]);
		int port = std::atoi(av[1]);
		std::string password = av[2];

		Server serv(port, password);
		serv.run();
	}
	catch(std::exception &e)
	{
		std::cerr << e.what() << std::endl;
	}
	return 0;
}