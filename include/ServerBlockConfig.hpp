/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ServerBlockConfig.hpp                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: fmorenil <fmorenil@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/06 21:49:29 by fvizcaya          #+#    #+#             */
/*   Updated: 2025/12/06 17:26:43 by fmorenil         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef _SERVERBLOCKCONFIG_HPP_
#define _SERVERBLOCKCONFIG_HPP_

#include <string>
#include <vector>
#include <map>
#include <stdexcept>

class LocationBlock;

class ServerBlock {
	
	private:
    	std::vector<std::string>		serverName;
		int								listeningPort;
		std::string						ifaceAddress;
		std::vector<LocationBlock*>		locationBlocks;
		int								numLocationBlocks;
	
	protected:
		std::string						documentRoot;
		std::string						indexPath;
		std::vector<std::string>		allowMethods;
		std::map<int, std::string>		errorPageMap;
		std::string						clientMaxBodySize;
		std::string						cgiPass;
		bool							autoIndex;
		bool							uploadEnable;
		std::string						uploadStore;
	public:
		ServerBlock();
		ServerBlock(const ServerBlock& other);
		virtual ~ServerBlock();
		ServerBlock& operator=(const ServerBlock& other);

		void								setServerName(std::vector<std::string> _serverName);
		std::vector<std::string>			getServerName(void) const;
		void								setListeningPort(int _listeningPort);
		int									getListeningPort(void) const;
		void								setIfaceAddress(std::string _ifaceAddress);
		std::string							getIfaceAddress(void) const;
		void								setDocumentRoot(std::string _documentRoot);
		std::string							getDocumentRoot(void) const;
		void								setIndexPath(std::string _indexPath);
		std::string							getIndexPath(void) const;
		void								setErrorPage(int errorCode, std::string path);
		std::map<int, std::string>			getErrorPageMap(void) const;
		void								setAllowMethods(std::vector<std::string> _allowMethods);
		std::vector<std::string>			getAllowMethods(void) const;
		void								setClientMaxBodySize(std::string _clientMaxBodySize);
		std::string							getClientMaxBodySize(void) const;
		void								setAutoIndex(bool _autoIndex);
		bool								getAutoIndex(void) const;
		void								setUploadEnable(bool _uploadEnable);
		bool								getUploadEnable(void) const;
		void								setUploadStore(std::string _uploadStore);
		std::string							getUploadStore(void) const;
		void								setCgiPass(std::string _cgiPass);
		std::string							getCgiPass(void) const;
		std::vector<LocationBlock*>&		getLocationBlocks(void);
		LocationBlock&						getLocationBlocksIndex(int index);
		const LocationBlock&				getLocationBlocksIndex(int index) const;
		void								setNumLocationBlocks(int numBlocks);
		int									getNumLocationBlocks(void) const;
		void								setLocationBlocksSize(int size);
		void								setLocationBlockPathIndex(int index, std::string path);
		const LocationBlock*				findBestLocationMatch(const std::string& path) const;
};

#endif
