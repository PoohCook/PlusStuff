/*
 * File:   EeyoreClient.cpp
 * Author: Pooh
 *
 * Created on October 3, 2018, 4:18 PM
 */


#include <string>
#include <iostream>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/string.hpp>
#include <boost/program_options.hpp>
#include "ChannelClient.h"
#include "PoohCommand.h"

namespace po = boost::program_options;


bool running = true;
PoohOptions options;


class EeyoreHandler{
public:
    string process( int client_id, PoohCommand command ){
        switch(command.type){
            case POOH_COMMAND_HELLO:
                return "Hello Pooh";

            case POOH_COMMAND_QUESTION:
                if( command.arg[0] == "name"){
                    return "Eeyore";
                }
                else if( command.arg[0] == "secret"){
                    return options.secret;
                }
                else if( command.arg[0] == "client_id"){
                    return to_string(client_id);
                }
                else {
                    return "oh bother";
                }


            case POOH_COMMAND_GOODBYE:
                running = false;
                return "Bye now";

            default:
                return "unknown command";

        }
    }
};


int main(int argc, const char *argv[]){

    if( !options.parse_options( argc, argv)){
        return -1;
    }

    ChannelClient<PoohCommand,string,EeyoreHandler> myChannel(options.client_id, options.port);

    while( running ){
        sleep(1);
    }

    return 0;

}


