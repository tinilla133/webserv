/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aurodrig <aurodrig@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/10 20:06:31 by aurodrig          #+#    #+#             */
/*   Updated: 2025/12/06 14:23:37 by aurodrig         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SERVER_HPP
#define SERVER_HPP

#include <netinet/in.h>
#include <string>
#include <iostream>
#include <unistd.h>

class Server {
private:
    int         listen_fd;   // listening socket
    sockaddr_in address;     // local address
    int         port;        // server port

public:
    explicit Server(int port);
    ~Server();

    int  getFd() const;
    int  getPort() const;

    int  acceptClient() const;
};

#endif
