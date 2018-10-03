/*
 * File:   PoohCommand.h
 * Author: Pooh
 *
 * Created on October 3, 2018, 4:18 PM
 */


#ifndef PoohCommand_H
#define PoohCommand_H


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


#endif   // PoohCommand_H
