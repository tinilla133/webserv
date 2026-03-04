/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   FileParsing.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: fmorenil <fmorenil@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/15 14:58:08 by fvizcaya          #+#    #+#             */
/*   Updated: 2025/12/06 19:16:43 by fmorenil         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <FileParsing.hpp>
#include <Webserver.hpp>
#include <cctype>

configFileParser::configFileParser()
{
	currentState = FILE_PARSER_STATE_INIT;
	numServerBlocks = 0;
	numLocationBlocks = 0;
	serverConfig = NULL;
}

configFileParser::~configFileParser()
{
    if (configFile.is_open())
        configFile.close();
}

void configFileParser::setCurrentState(configFileParserState_t _currentState)
{
	currentState = _currentState;
}

configFileParserState_t	configFileParser::getCurrentState(void)
{
	return (currentState);
}

void configFileParser::setConfigObject(Config& _serverConfig)
{
	serverConfig = &_serverConfig;
}

void configFileParser::setFilePath(const std::string& _filePath)
{
	filePath = _filePath;
}

bool	configFileParser::parseFile(void)
{
	configFile.open(filePath.c_str());
	if (!configFile.is_open()) {
		setCurrentState(FILE_PARSER_STATE_ERROR);
		std::string errMsg = "Could not open file ";
		errMsg.append(filePath);
		return(handleError(errMsg));
	
	}
	setCurrentContext(FILE_CONTEXT_MAIN_CONTEXT);
	int serverBlocksCount = 0;
	std::vector<int> locationBlocksCounts;
	configFileError_t configFileSanity = configSanityCheckAndBlocksCount(configFile, serverBlocksCount, locationBlocksCounts);
	if (configFileSanity != FILE_ERROR_OK) {
		setCurrentState(FILE_PARSER_STATE_ERROR);
		std::string errMsg;
		switch (configFileSanity) {
			case FILE_ERROR_NO_SERVER_BLOCKS:
				errMsg = "At least one server block must be present in configuration file " + filePath;
				return (handleError(errMsg));
			case FILE_ERROR_NESTED_SERVER_BLOCKS:
				errMsg = "Nested seerver blocks are not allowed in configuration file  " + filePath;
				return (handleError(errMsg));
			case FILE_ERROR_NESTED_LOCATION_BLOCKS:
				errMsg = "Nested location blocks are not allowed in configuration file " + filePath;
				return (handleError(errMsg));
			case FILE_ERROR_TOO_MANY_SERVER_BLOCKS:
				serverBlocksCount = MAX_SERVER_BLOCKS;
				ERR_PRINT("Warning: Server blocks exceed maximum allowed. Truncated to " << MAX_SERVER_BLOCKS);
				break;
			case FILE_ERROR_TOO_MANY_LOCATION_BLOCKS:
				ERR_PRINT("Warning: Location blocks exceed maximum allowed. Truncated to " << MAX_LOCATION_BLOCKS_PER_SERVER);
				break;
			default:
				errMsg = "Unknown error in configuration file ";
				return(handleError(errMsg));
		}
	}
	numServerBlocks = serverBlocksCount;
	serverConfig->setNumServerBlocks(serverBlocksCount);
	// Resize vector for server blocks.
	serverConfig->getServerBlocks().resize(numServerBlocks);
	// Allocate memory for LocationBlocks inside each ServerBlock
	for (int i = 0; i < numServerBlocks; i++) {
        // serverConfig->getServerBlockIndex(i).getLocationBlocks().reserve(locationBlocksCounts[i]);
		for (int j = 0; j < locationBlocksCounts[i]; j++) {
			serverConfig->getServerBlockIndex(i).getLocationBlocks().push_back(new LocationBlock());
		}
		serverConfig->getServerBlockIndex(i).setLocationBlocksSize(locationBlocksCounts[i]);
		serverConfig->getServerBlockIndex(i).setNumLocationBlocks(locationBlocksCounts[i]);
	}
	configFile.clear();
	configFile.seekg(0, std::ios::beg);
	
	std::string currentLine, previousLine, token;
	int currentServerIndex = 0;
	int currentLocationIndex = 0;
	configFileContext_t previousContext = FILE_CONTEXT_MAIN_CONTEXT;
	 enum blockType {
			NONE,
			SERVER,
			LOCATION
	} currentBlock = NONE;

	while (std::getline(configFile, currentLine)) {
		if (getCurrentState() == FILE_PARSER_STATE_ERROR) {
			std::string errMsg = "Error in configuration file " + previousLine;
			return(handleError(errMsg));
		}
		
		// Remove comments from the line
		std::string cleanLine = removeComments(currentLine);
		
		// Skip empty lines and comment-only lines
		if (cleanLine.empty() || isWhitespaceOnly(cleanLine)) {
			continue;
		}
		
		bool isControlLine = false;
		std::istringstream iss(cleanLine);
		while (iss >> token) {
			previousContext = getCurrentContext();
			handleContext(token);
            if (token == "server" && previousContext == FILE_CONTEXT_MAIN_CONTEXT) {
				currentBlock = SERVER;
				currentLocationIndex = 0;
				isControlLine = true;
            }
			else if (token == "location" && previousContext == FILE_CONTEXT_SERVER) {
				currentBlock = LOCATION;
				std::string _locationPath;
				if (!(iss >> _locationPath)) {
					std::string errMsg = "Error in configuration file. Location must be followed by path: " + currentLine + " File: " + filePath;
					return handleError(errMsg);
				}
				serverConfig->getServerBlockIndex(currentServerIndex).setLocationBlockPathIndex(currentLocationIndex, _locationPath);
				currentLocationIndex++;
				isControlLine = true;
			}
			else if (token == "}") {
				if (currentBlock == LOCATION) {
					currentBlock = SERVER;
				}
				else if (currentBlock == SERVER) {
					currentBlock = NONE;
					currentServerIndex++;
				}
				isControlLine = true;
			}
        }
		
		// Call tokenizer once per line for content lines (not control lines)
		if (!isControlLine) {
			if (getCurrentContext() == FILE_CONTEXT_SERVER && currentBlock == SERVER) {
				tokenizeLineServerBlock(currentLine, serverConfig->getServerBlockIndex(currentServerIndex));
			}
			else if (getCurrentContext() == FILE_CONTEXT_LOCATION && currentBlock == LOCATION) {
				tokenizeLineLocationBlock(currentLine, serverConfig->getServerBlockIndex(currentServerIndex).getLocationBlocksIndex(currentLocationIndex - 1));
			}
		}
		
		previousLine = currentLine;
	}
	return (getCurrentState() != FILE_PARSER_STATE_ERROR);
}

static bool checkTokenVector(std::vector<std::string>& tokenVector)
{
	std::string aux;
	size_t		index;
	
	if (tokenVector.back() == ";")
		return (true);

	index = tokenVector[tokenVector.size() - 1].find(';');
	if (index != std::string::npos) {
		tokenVector[tokenVector.size() - 1] = tokenVector[tokenVector.size() - 1].substr(0, index);
		tokenVector.push_back(";");
		return (true);
	}
	return (false);
}

void configFileParser::tokenizeLineServerBlock(const std::string& line, ServerBlock& currentServerBlock)
{
	// Remove comments from the line
	std::string cleanLine = removeComments(line);
	
	// Skip empty lines after comment removal
	if (cleanLine.empty() || isWhitespaceOnly(cleanLine)) {
		return;
	}
	
	std::istringstream 			iss(cleanLine);
	std::string 				token, auxPath;
	std::vector<std::string> 	tokenVector;

	while (iss >> token) {
		tokenVector.push_back(token);
		handleContext(token);
	}
	
	if (tokenVector.size() >= 2 && checkTokenVector(tokenVector)) {
		currentKeyword = getKeywordType(tokenVector[0]);
		switch(currentKeyword) {
			case FILE_KEYWORD_LISTEN:
				currentServerBlock.setListeningPort(stringToInt(tokenVector[1]));
				break;
			case FILE_KEYWORD_IFACE:
				currentServerBlock.setIfaceAddress(tokenVector[1]);
				break;
			case FILE_KEYWORD_SERVER_NAME:
				currentServerBlock.setServerName(getSubVector(tokenVector, 1, tokenVector.size() - 1));
				break;
			case FILE_KEYWORD_ROOT:
				currentServerBlock.setDocumentRoot(tokenVector[1]);
				break;
			case FILE_KEYWORD_INDEX:
				currentServerBlock.setIndexPath(tokenVector[1]);
				break;
			case FILE_KEYWORD_ERROR_PAGE:
				if (tokenVector.size() >= 3) {
					int errorCode = stringToInt(tokenVector[1]);
					std::string errorPage = tokenVector[2];
					currentServerBlock.setErrorPage(errorCode, errorPage);
				}
				break;
			case FILE_KEYWORD_ALLOW_METHODS:
				currentServerBlock.setAllowMethods(getSubVector(tokenVector, 1, tokenVector.size() - 1));
				break;
			case FILE_KEYWORD_CLIENT_MAX_BODY_SIZE:
				currentServerBlock.setClientMaxBodySize(tokenVector[1]);
				break;
			case FILE_KEYWORD_AUTO_INDEX:
				if (tokenVector[1] != "on" && tokenVector[1] != "off") {
					setCurrentState(FILE_PARSER_STATE_ERROR);
					break;
				}
				currentServerBlock.setAutoIndex(tokenVector[1] == "on");
				break;
			case FILE_KEYWORD_CGI_PASS:
				currentServerBlock.setCgiPass(tokenVector[1]);
				break;
			case FILE_KEYWORD_UPLOAD_ENABLE:
				if (tokenVector[1] != "on" && tokenVector[1] != "off") {
					setCurrentState(FILE_PARSER_STATE_ERROR);
					break;
				}
					currentServerBlock.setUploadEnable(tokenVector[1] == "on");
				break;
			case FILE_KEYWORD_UPLOAD_STORE:
				currentServerBlock.setUploadStore(tokenVector[1]);
				break;
			case FILE_KEYWORD_HTTP:
				break;
			default:
				setCurrentState(FILE_PARSER_STATE_ERROR);
				break;
		}
	}
}

void configFileParser::tokenizeLineLocationBlock(const std::string& line, LocationBlock& currentLocationBlock)
{
	// Remove comments from the line
	std::string cleanLine = removeComments(line);
	
	// Skip empty lines after comment removal
	if (cleanLine.empty() || isWhitespaceOnly(cleanLine)) {
		return;
	}
	
	std::istringstream 			iss(cleanLine);
	std::string 				token, auxPath;
	std::vector<std::string> 	tokenVector;

	while (iss >> token) {
		tokenVector.push_back(token);
		handleContext(token);
	}
	
	if (tokenVector.size() >= 2 && checkTokenVector(tokenVector)) {
		currentKeyword = getKeywordType(tokenVector[0]);
		switch(currentKeyword) {
			case FILE_KEYWORD_ROOT:
				currentLocationBlock.setDocumentRoot(tokenVector[1]);
				break;
			case FILE_KEYWORD_INDEX:
				currentLocationBlock.setIndexPath(tokenVector[1]);
				break;
			case FILE_KEYWORD_ERROR_PAGE:
				if (tokenVector.size() >= 3) {
					int errorCode = stringToInt(tokenVector[1]);
					std::string errorPage = tokenVector[2];
					currentLocationBlock.setErrorPage(errorCode, errorPage);
				}
				break;
			case FILE_KEYWORD_ALLOW_METHODS:
				currentLocationBlock.setAllowMethods(getSubVector(tokenVector, 1, tokenVector.size() - 1));
				break;
			case FILE_KEYWORD_CLIENT_MAX_BODY_SIZE:
				currentLocationBlock.setClientMaxBodySize(tokenVector[1]);
				break;
			case FILE_KEYWORD_AUTO_INDEX:
				if (tokenVector[1] != "on" && tokenVector[1] != "off") {
					setCurrentState(FILE_PARSER_STATE_ERROR);
					break;
				}
				currentLocationBlock.setAutoIndex(tokenVector[1] == "on");
				break;
			case FILE_KEYWORD_CGI_PASS:
				currentLocationBlock.setCgiPass(tokenVector[1]);
				break;
			case FILE_KEYWORD_UPLOAD_ENABLE:
				if (tokenVector[1] != "on" && tokenVector[1] != "off") {
					setCurrentState(FILE_PARSER_STATE_ERROR);
					break;
				}
					currentLocationBlock.setUploadEnable(tokenVector[1] == "on");
				break;
			case FILE_KEYWORD_UPLOAD_STORE:
				currentLocationBlock.setUploadStore(tokenVector[1]);
				break;
			case FILE_KEYWORD_HTTP:
				break;
			default:
				setCurrentState(FILE_PARSER_STATE_ERROR);
				break;
		}
	}
}


void configFileParser::handleContext(const std::string &token)
{
	configFileToken_t tokenType = getTokenType(token);
	tokenVector.push_back(tokenType);

	switch(tokenType) {
		case FILE_TOKEN_CLOSE:
			if (!contextStack.empty()) {
				contextStack.pop();
				if (!contextStack.empty()) {
					setCurrentContext(contextStack.top());
				}
				else {
					setCurrentContext(FILE_CONTEXT_MAIN_CONTEXT);
					ERR_PRINT("Unexpected '}'");
					currentState = FILE_PARSER_STATE_ERROR;
				}
			}
			break;
		case FILE_TOKEN_KEYWORD:
			currentState = FILE_PARSER_STATE_CONTEXT;
			if (token == "server") {
				contextStack.push(FILE_CONTEXT_SERVER);
			}
			else if (token == "location") {
				contextStack.push(FILE_CONTEXT_LOCATION);
			}
			break;
		case FILE_TOKEN_OPEN:
		case FILE_TOKEN_SEMICOLON:
			break;
		default:
			break;
	}
	// DBG_PRINT(getCurrentContext());
}

void configFileParser::setCurrentContext(configFileContext_t context)
{
	if (!contextStack.empty()) {
		contextStack.pop();
	}
	contextStack.push(context);
}

configFileContext_t configFileParser::getCurrentContext(void)
{
	if (!contextStack.empty())
		return (contextStack.top());
	return (FILE_CONTEXT_MAIN_CONTEXT);
}

configFileToken_t configFileParser::getTokenType(const std::string& token)
{
	if (token == "{")
		return (FILE_TOKEN_OPEN);
	else if (token == "}")
		return (FILE_TOKEN_CLOSE);
	else if (token == ";")
		return (FILE_TOKEN_SEMICOLON);
	else if (token == "#")
		return (FILE_TOKEN_SHARP);
	else {
		currentKeyword = getKeywordType(token);
		
	}
	return (FILE_TOKEN_KEYWORD);
}

configFileKeyword_t configFileParser::getKeywordType(const std::string& token)
{
	if (token == "listen") {
		return (FILE_KEYWORD_LISTEN);
	}
	if (token == "iface") {
		return (FILE_KEYWORD_IFACE);
	}
	else if (token == "root") {
		return (FILE_KEYWORD_ROOT);
	}
	else if (token == "index") {
		return (FILE_KEYWORD_INDEX);
	}
	else if (token == "error_page") {
		return (FILE_KEYWORD_ERROR_PAGE);
	}
	else if (token == "server_name") {
		return (FILE_KEYWORD_SERVER_NAME);
	}
	else if (token == "allow_methods") {
		return (FILE_KEYWORD_ALLOW_METHODS);
	}
	else if (token == "http") {
		return (FILE_KEYWORD_HTTP);
	}
	else if (token == "client_max_body_size") {
		return (FILE_KEYWORD_CLIENT_MAX_BODY_SIZE);
	}
	else if (token == "auto_index") {
		return (FILE_KEYWORD_AUTO_INDEX);
	}
	else if (token == "cgi_pass") {
		return (FILE_KEYWORD_CGI_PASS);
	}
	else if (token == "upload_enable") {
		return (FILE_KEYWORD_UPLOAD_ENABLE);
	}
	else if (token == "upload_store") {
		return (FILE_KEYWORD_UPLOAD_STORE);
	}
	return (FILE_KEYWORD_UNKNOWN);
}

configFileError_t configFileParser::configSanityCheckAndBlocksCount(std::istream &fileStream, int &serverCount, std::vector<int> &locationCounts) {

    enum blockType {
		NONE,
		SERVER,
		LOCATION
	} currentBlock = NONE;

    int currentServerIndex = 0;
    serverCount = 0;

    std::string currentLine;
    while (std::getline(fileStream, currentLine)) {
        // Remove comments from the line
        std::string cleanLine = removeComments(currentLine);
        
        // Skip empty lines and comment-only lines
        if (cleanLine.empty() || isWhitespaceOnly(cleanLine)) {
            continue;
        }
        
        std::istringstream iss(cleanLine);
        std::string token;
        while (iss >> token) {
            if (token == "server") {
                if (currentBlock == SERVER) {
                    return FILE_ERROR_NESTED_SERVER_BLOCKS;
                }
                currentBlock = SERVER;
                serverCount++;
                if (serverCount > MAX_SERVER_BLOCKS) {
                    return FILE_ERROR_TOO_MANY_SERVER_BLOCKS;
                }
                locationCounts.push_back(0);
                currentServerIndex = serverCount;
            } else if (token == "location") {
                if (currentBlock == LOCATION) {
                    return FILE_ERROR_NESTED_LOCATION_BLOCKS;
                }
                currentBlock = LOCATION;
                if (currentServerIndex > 0) {
                    locationCounts[currentServerIndex - 1]++;
                    if (locationCounts[currentServerIndex - 1] > MAX_LOCATION_BLOCKS_PER_SERVER) {
                        return FILE_ERROR_TOO_MANY_LOCATION_BLOCKS;
                    }
                }
            } else if (token == "}") {
                if (currentBlock == LOCATION) {
                    currentBlock = SERVER;
                } else if (currentBlock == SERVER) {
                    currentBlock = NONE;
                    currentServerIndex = -1;
                }
            }
        }
    }
	if (serverCount == 0) {
		return FILE_ERROR_NO_SERVER_BLOCKS;
	}
    return FILE_ERROR_OK;
}

bool configFileParser::handleError(std::string& errorMsg)
{
	configFile.close();
	ERR_PRINT(errorMsg);
	return false;
}

int32_t	configFileParser::getServerBlocks(void)
{
	return (numServerBlocks);
}

std::string configFileParser::removeComments(const std::string& line)
{
	size_t commentPos = line.find('#');
	if (commentPos != std::string::npos) {
		return line.substr(0, commentPos);
	}
	return line;
}

bool configFileParser::isWhitespaceOnly(const std::string& line)
{
	for (size_t i = 0; i < line.length(); i++) {
		if (!std::isspace(line[i])) {
			return false;
		}
	}
	return true;
}
