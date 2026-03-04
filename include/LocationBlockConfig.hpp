/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   LocationBlockConfig.hpp                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: fmorenil <fmorenil@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/06 21:49:56 by fvizcaya          #+#    #+#             */
/*   Updated: 2025/12/06 17:26:43 by fmorenil         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef _LOCATIONBLOCKCONFIG_H_
#define _LOCATIONBLOCKCONFIG_H_

#include <ServerBlockConfig.hpp>

class LocationBlock : public ServerBlock {
	
	private:
		std::string					locationPath;

	public:
		LocationBlock();
		LocationBlock(const LocationBlock& other);
		~LocationBlock();
		LocationBlock& operator=(const LocationBlock& otherLocationBlock);

		void						setLocationPath(std::string _locationPath);
		std::string					getLocationPath(void) const;
};

#endif