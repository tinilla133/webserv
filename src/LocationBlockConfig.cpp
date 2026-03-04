/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   LocationBlockConfig.cpp                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: fmorenil <fmorenil@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/09 21:21:09 by fvizcaya          #+#    #+#             */
/*   Updated: 2025/12/06 17:26:43 by fmorenil         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <LocationBlockConfig.hpp>

LocationBlock::LocationBlock() { }

LocationBlock::LocationBlock(const LocationBlock& other) 
	: ServerBlock(other), locationPath(other.locationPath) { }

LocationBlock::~LocationBlock() { }

LocationBlock& LocationBlock::operator=(const LocationBlock& otherLocationBlock)
{
	if (this != &otherLocationBlock) {
		ServerBlock::operator=(otherLocationBlock);
		locationPath = otherLocationBlock.locationPath;
	}
	return (*this);
}

void LocationBlock::setLocationPath(std::string _locationPath)
{
	locationPath = _locationPath;
}

std::string LocationBlock::getLocationPath(void) const
{
	return (locationPath);
}