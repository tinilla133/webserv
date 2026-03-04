/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   utils.hpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aurodrig <aurodrig@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/02 16:00:43 by fmorenil          #+#    #+#             */
/*   Updated: 2025/12/06 14:24:40 by aurodrig         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef UTILS_HPP
#define UTILS_HPP

#include <string>
#include <map>
#include <sstream>
#include <vector>

std::string getContentType(const std::string& path);

std::string toLower(const std::string &str);
std::string decodeChunkedBody(const std::string &chunked);

void parseRequest(const std::string& raw,
                  std::string& method,
                  std::string& path,
                  std::map<std::string, std::string>& headers,
                  std::string& body);

int stringToInt(const std::string& str);
std::vector<std::string> getSubVector(std::vector<std::string>& vec, size_t start, size_t end);

// Non-blocking I/O functions
std::string readFileNonBlocking(const std::string& filepath);
bool writeFileNonBlocking(const std::string& filepath, const std::string& content);
bool fileExistsNonBlocking(const std::string& filepath);

#endif
