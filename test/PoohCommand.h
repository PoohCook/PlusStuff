/*
 * File:   PoohCommand.h
 * Author: Pooh
 *
 * Created on October 3, 2018, 4:18 PM
 */


#ifndef PoohCommand_H
#define PoohCommand_H

#include "Options.h"

typedef enum{
    POOH_COMMAND_NONE = 0,
    POOH_COMMAND_HELLO,
    POOH_COMMAND_QUESTION,
    POOH_COMMAND_GOODBYE
} POOH_COMMAND_TYPE;

class PoohCommand{
public:
    POOH_COMMAND_TYPE type;
    std::vector<string> arg;

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        ar & type;
        ar & arg;
    }
};


class PoohOptions : public process_params::Options{

public:
    int port;
    int client_id;
    string secret;

    PoohOptions(){

        add_option<int>("port", &port, "set Port to bind on");
        add_option<int>("client", &client_id, "set client_id to be used in bind");
        add_option<string>("secret", &secret, "set secret");

    }

};



#endif   // PoohCommand_H
