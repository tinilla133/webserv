/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   FileParsing.hpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: fmorenil <fmorenil@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/15 10:00:35 by fvizcaya          #+#    #+#             */
/*   Updated: 2025/12/04 17:22:07 by fmorenil         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef _FILE_PARSING_HPP_
#define _FILE_PARSING_HPP_

#include <Config.hpp>
#include <ServerBlockConfig.hpp>
#include <LocationBlockConfig.hpp>
#include <utils.hpp>
#include <string>
#include <stack>
#include <vector>
#include <fstream>
#include <sstream>
#include <stdlib.h>

#define MAX_SERVER_BLOCKS 256
#define MAX_LOCATION_BLOCKS_PER_SERVER 32

typedef enum configFileParserState_e
{
	FILE_PARSER_STATE_INIT,
	FILE_PARSER_STATE_CONTEXT,
	FILE_PARSER_STATE_SUCCESS,
	FILE_PARSER_STATE_ERROR
} configFileParserState_t;

typedef enum configFileContext_e
{
	FILE_CONTEXT_MAIN_CONTEXT,
	FILE_CONTEXT_SERVER,
	FILE_CONTEXT_LOCATION
} configFileContext_t;

typedef enum configFileToken_e
{
	FILE_TOKEN_KEYWORD,
	FILE_TOKEN_OPEN,
	FILE_TOKEN_CLOSE,
	FILE_TOKEN_SEMICOLON,
	FILE_TOKEN_SHARP,
	FILE_TOKEN_EOF
} configFileToken_t;

typedef enum configFileKeyword_e
{
	FILE_KEYWORD_UNKNOWN,
	FILE_KEYWORD_HTTP,
	FILE_KEYWORD_LISTEN,
	FILE_KEYWORD_IFACE,
	FILE_KEYWORD_ROOT,
	FILE_KEYWORD_INDEX,
	FILE_KEYWORD_ERROR_PAGE,
	FILE_KEYWORD_SERVER_NAME,
	FILE_KEYWORD_ALLOW_METHODS,
	FILE_KEYWORD_CLIENT_MAX_BODY_SIZE,
	FILE_KEYWORD_AUTO_INDEX,
	FILE_KEYWORD_CGI_PASS,
	FILE_KEYWORD_UPLOAD_ENABLE,
	FILE_KEYWORD_UPLOAD_STORE
} configFileKeyword_t;

typedef enum configFileError_e
{
	FILE_ERROR_OK,
	FILE_ERROR_NO_SERVER_BLOCKS,
	FILE_ERROR_NESTED_SERVER_BLOCKS,
	FILE_ERROR_NESTED_LOCATION_BLOCKS,
	FILE_ERROR_TOO_MANY_SERVER_BLOCKS,
	FILE_ERROR_TOO_MANY_LOCATION_BLOCKS
} configFileError_t;

class configFileParser
{
	private:
		Config*							serverConfig;
		configFileParserState_t 		currentState;
		configFileKeyword_t				currentKeyword;
		std::stack<configFileContext_t> contextStack;
		std::vector<configFileToken_t> 	tokenVector;
		std::ifstream 					configFile;
		int32_t 						numServerBlocks;
		int32_t 						numLocationBlocks;
		std::string						filePath;
		void 							tokenizeLineServerBlock(const std::string& line, ServerBlock& currentServerBlock);
		void 							tokenizeLineLocationBlock(const std::string& line, LocationBlock& currentLocationBlock);
		void 							handleContext(const std::string& token);
		void 							setCurrentContext(configFileContext_t context);
		configFileToken_t 				getTokenType(const std::string& token);
		configFileKeyword_t 			getKeywordType(const std::string& token);
		void 							setCurrentState(configFileParserState_t _currentState);
		configFileParserState_t 		getCurrentState(void);
		bool 							handleError(std::string& errorMsg);
		configFileError_t				configSanityCheckAndBlocksCount(std::istream &fileStream, int &serverCount, std::vector<int> &locationCounts);
		std::string						removeComments(const std::string& line);
		bool							isWhitespaceOnly(const std::string& line);

	public:
		configFileParser();
		~configFileParser();
		
		void					setConfigObject(Config& _serverConfig);
		void 					setFilePath(const std::string& path);
		bool 					parseFile(void);
		configFileContext_t 	getCurrentContext(void);
		int32_t					getServerBlocks(void);

		const Config&			getConfig(int serverBlockIndex) const;
};

#endif /* _FILE_PARSING_HPP_ */