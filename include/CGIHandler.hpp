/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CGIHandler.hpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: fernando <fernando@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/20 15:51:33 by fmorenil          #+#    #+#             */
/*   Updated: 2025/11/26 16:43:48 by fernando         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CGIHANDLER_HPP
#define CGIHANDLER_HPP

#include <string>
#include <vector>
#include <map>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <cstdlib>
#include <cstring>
#include <cerrno>

class Request;

class CGIHandler
{
    private:
        const Request&   					request;
        std::string							scriptPath;
        std::string							cgiExecutor;
        std::map<std::string, std::string>	_env;

		void						setupEnvironment();
		void						setupPipes(int pipeFd[2]);
		pid_t						forkAndExec(int pipeFd[2]);
		std::vector<char*>			buildEnvp() const;
		std::vector<char*>			buildArgv() const;
		

    public:
		CGIHandler(const Request& _request, const std::string& _scriptPath, const std::string& _cgiExecutor);
		~CGIHandler();

		std::string			execute();
		static bool			isCGIScript(const std::string& path);
		static std::string	getInterpreter(const std::string& path);
};

#endif
