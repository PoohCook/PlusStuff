/**
 * @file ChannelMessageHeader.h
 * @author  Pooh Cook
 * @version 1.0
 * @created September 30, 2018, 1:41 PM
 *
 * @section LICENSE
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details at
 * https://www.gnu.org/copyleft/gpl.html
 *
 */

#ifndef ChannelMessageHeader_H
#define ChannelMessageHeader_H

#include <iostream>
#include <string>
#include <sstream>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include "Logger.h"

using namespace std;

/**
 * @brief Channel Message Types
 *
 */
typedef enum{
    MESSAGE_TYPE_WAITING = 0,   /// No message currently in play. used as initial condition when waiting on a message
    MESSAGE_TYPE_COMMAND,       /// Message for sending a Command object
    MESSAGE_TYPE_RESPONSE,      /// Message for replying to a command with a Response object
    MESSAGE_TYPE_ATTACH,        /// Message for a client to attempt attach to a session
    MESSAGE_TYPE_ATTACHED       /// Message Response granting attachment to a session
} MessageType;


/**
 * @class ChannelMessageHeader
 * @brief Header for all Channel messages
 *
 * This class is used by the ChannelSendProcessor as the container for information about each message being exchanged.
 * It is also used by Client and Provider session objects in establishing session connection
 *
 */
class ChannelMessageHeader{
public:
    MessageType type;       /// Type of this message
    int id;                 /// Id of the message  (also used to carry client_id in the case of Attach messages

    /**
     * Default Constructor
     *
     * creates an empty ChannelMessageHeader with type  MESSAGE_TYPE_WAITING
     *
     */
    ChannelMessageHeader(){
        type = MESSAGE_TYPE_WAITING;
        id = 0;
    }

    /**
     * Constructor for creating a specific ChannelMessageHeader
     *
     * creates an ChannelMessageHeader with specified type and id
     *
     * @param type MessageType for the header
     * @param id message id for the header
     *
     */
    ChannelMessageHeader( MessageType type, int id){
        this->type = type;
        this->id = id;
    }

    /**
     * @brief serialize / deserialize method for the ChannelMessageHeader
     *
     * This object is utilized by the Boost serialization object adn is not intended for direct consumption
     * @param ar  boost serialization archive object
     * @param version version id for the archive object
     *
     */
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        ar & type;
        ar & id;
    }

    /**
     * @brief String representation of the Header
     *
     * useful for diagnostic output
     *
     * @return an std::string that represents the header as type:id
     *
     */
    string str(){
        return  to_string(type) + ":" + to_string(id);
    }

};

#endif  // ChannelMessageHeader_H


