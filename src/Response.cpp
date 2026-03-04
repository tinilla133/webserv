/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Response.cpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aurodrig <aurodrig@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/24 18:53:18 by aurodrig          #+#    #+#             */
/*   Updated: 2025/09/24 18:54:32 by aurodrig         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <Response.hpp>

Response::Response() : status_code(200) {}

std::string Response::getStatusMessage(int code) const {
    switch (code) {
        case 200: return "OK";
        case 201: return "Created";
        case 204: return "No Content";
        case 400: return "Bad Request";
        case 405: return "Method Not Allowed";
        case 403: return "Forbidden";
        case 413: return "Payload Too Large";
        case 404: return "Not Found";
        case 500: return "Internal Server Error";
        default:  return "Unknown";
    }
}

std::string Response::toString() const {
    std::ostringstream ss;
    ss << "HTTP/1.1 " << status_code << " " << getStatusMessage(status_code) << "\r\n";

    for (std::map<std::string, std::string>::const_iterator it = headers.begin(); it != headers.end(); ++it) {
        ss << it->first << ": " << it->second << "\r\n";
    }

    ss << "\r\n" << body;
    return ss.str();
}
