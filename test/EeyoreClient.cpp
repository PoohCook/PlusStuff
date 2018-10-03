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
int pooh_port;
int my_client_id;
string init_var;

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
                else if( command.arg[0] == "init_var"){
                    return init_var;
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


static bool parse_args(int argc, char *argv[]){

    // Declare the supported options.
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help", "produce help message")
        ("port", po::value<int>(), "set port")
        ("client_id", po::value<int>(), "set client_id")
        ("init_var", po::value<string>(), "set init_var")

    ;

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help")) {
        cout << desc << "\n";
        return false;
    }

    if (vm.count("port")) {
        pooh_port = vm["port"].as<int>();
    } else {
        cout << "port was not set.\n";
        return false;
    }

    if (vm.count("client_id")) {
        my_client_id = vm["client_id"].as<int>();
    } else {
        cout << "client_id was not set.\n";
        return false;
    }

    if (vm.count("init_var")) {
        init_var = vm["init_var"].as<string>();
    } else {
        cout << "init_var was not set.\n";
        return false;
    }



    return true;
}

int main(int argc, char *argv[]){

    if( !parse_args( argc, argv)){
        return -1;
    }

    ChannelClient<PoohCommand,string,EeyoreHandler> myChannel(my_client_id, pooh_port);

    while( running ){
        sleep(1);
    }

    return 0;

}


