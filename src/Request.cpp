/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Request.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aurodrig <aurodrig@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/24 18:47:07 by aurodrig          #+#    #+#             */
/*   Updated: 2025/12/06 14:04:16 by aurodrig         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <Request.hpp>
#include <utils.hpp>

Request::Request() {}

void Request::parse(const std::string &raw_request) {
    std::istringstream stream(raw_request);
    std::string line;

    if (std::getline(stream, line) && !line.empty()) {
        if (!line.empty() && line[line.size() - 1] == '\r')
            line.erase(line.size() - 1);
        parseRequestLine(line);
    }

    //Headers
    while (std::getline(stream, line)) {
        if (line == "\r" || line == "" || line == "\n") break;
        if (!line.empty() && line[line.size() - 1] == '\r')
            line.erase(line.size() - 1);
        parseHeaderLine(line);
    }



    //Body (if present)
    std::string bodyContent;
    while (std::getline(stream, line)) {
        bodyContent += line + "\n";
    }
    body = bodyContent;
}

void Request::parseRequestLine(const std::string &line) {
    std::istringstream iss(line);
    iss >> method >> path >> version;
}

void Request::parseHeaderLine(const std::string &line) {
    size_t pos = line.find(':');
    if (pos == std::string::npos)
        return;

    std::string key = line.substr(0, pos);
    std::string value = line.substr(pos + 1);

    // remove spaces or tabs at the beginning of the value
    while (!value.empty() && (value[0] == ' ' || value[0] == '\t'))
        value.erase(0, 1);

    // remove \r or \n at the end of the value
    while (!value.empty() &&
           (value[value.size() - 1] == '\r' || value[value.size() - 1] == '\n'))
        value.erase(value.size() - 1);

    // convert key to lowercase to avoid differences
    headers[toLower(key)] = value;
}

std::string Request::getMethod() const {
    return (this->method);
}
