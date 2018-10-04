/**
 * @file Options.h
 * @author  Pooh Cook
 * @version 1.0
 * @created October 4, 2018, 8:26 AM
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

#ifndef Options_H
#define Options_H

#include <string>
#include <iostream>
#include <boost/program_options.hpp>

namespace po = boost::program_options;

namespace process_params{


/**
 * @class OptionRendererBase
 * @brief Abstract base class for option render template
 *
 * Not intended for consumer use.
 *
 */
class OptionRendererBase{

public:
    virtual std::vector<std::string> get_options() =0;

};


/**
 * @class OptionRendererBase
 * @brief template class for option render
 *
 * Not intended for consumer use. This template is used create render objects for each of the options assigned in options class
 *
 *
 */
template<typename T>
class OptionRenderer:  public OptionRendererBase{
    T* member;
    std::string name;

public:
    /**
     * Constructor to create a render given the option name and pointer the associated meber
     *
     */
    OptionRenderer(const char* name, T* member)
    : member(member), name(name){

    }

    /**
     * @brief get the options which can be used in command line arguments for the associated member
     *
     */
    std::vector<std::string> get_options(){

        std::stringstream ss;
        ss << *member;

        std::vector<std::string> opts;
        opts.push_back( "--" + name);
        opts.push_back( ss.str());

        return opts;
    }

};


/**
 * @class Options
 * @brief used to creating sharable parameter collections
 *
 * base class to be used for creating sharable parameter collections that can be used by parent and child processes.
 *
 */
class Options{

private:
    po::options_description desc;
    std::vector<std::shared_ptr<OptionRendererBase>> renders;

protected:

    /**
     * @brief add an option
     *
     * this method is used ot add an option ot the list of members to bee parsed or rendered. The template type name
     * provides the type of the member.
     *
     */
    template<typename T>
    void add_option( const char* name, T* member, const char* description ){
        desc.add_options()
            (name, po::value<T>(member), description);

        renders.push_back(std::shared_ptr<OptionRendererBase>(new OptionRenderer<T>(name, member)));
    }


public:
    /**
     * Default Constructor
     *
     */
    Options()
    : desc("Allowed options"){
    }

    /**
     * @brief parse command line arguments collection into the derived structure
     *
     * the arguments will be parsed into assigned variables based upon the setup provided by calls ot add_option
     *
     * @param argc integer count of argents form main call parameters
     * @param argv array of char* string arguments from main call parameters
     *
     * @return true if parse succeeded
     *
     */
    bool parse_options( int argc, const char * argv[]){
        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);

        return true;
    }

    /**
     * @brief get collection of command line args that are needed to pass the derived members to a child process call
     *
     */
    std::vector<std::string> get_option_args(){

        std::vector<std::string> opts;

        for ( auto it = renders.begin() ; it != renders.end() ; it++){
            auto it_ops = (*it)->get_options();
            opts.insert( opts.end(), it_ops.begin(), it_ops.end() );
        }

        return opts;
    }

};

}



#endif // Options_H

