/*
 * File:   MessageBuffer.h
 * Author: Pooh
 *
 * Created on September 15, 2018, 11:39 AM
 */

#ifndef MessageBuffer_H
#define MessageBuffer_H

#include <pthread.h>
#include <stdbool.h>


#define MESSAGE_QUEUE_SIZE 100

template <class T>
class MessageBuffer{

    pthread_mutex_t lock;
    T buff[MESSAGE_QUEUE_SIZE];
    T* inP;
    T* outP;

    void increment_pointer(T** pp);
    T get_next();
    void delete_message_ahead();
    void add_next (T msg);

public:
    MessageBuffer( );
    ~MessageBuffer( );

    void Push( T message);
    T Pop( );
    bool HasMessage();

};


// Implementations of template methods
template <class T>
void MessageBuffer<T>::increment_pointer(T** pp){
    if( *pp == buff + MESSAGE_QUEUE_SIZE -1 ) *pp = buff;
    else (*pp)++;
}

template <class T>
T MessageBuffer<T>::get_next (){

    T retMsg;
    if( outP != inP ) {
        retMsg = *(outP);
        increment_pointer( &(outP));
    }
    return retMsg;
}

//  if a message is in the way of the next push, then release it
template <class T>
void MessageBuffer<T>::delete_message_ahead(){

    T* nextIp = inP;
    increment_pointer( &(nextIp));
    if( nextIp == outP){
        // overwrite about to happen must kill next OP
        T msg = get_next( );
        //delete msg;
    }


}

//  The design of this buffer is to loose any old messages in favor of new ones
//  This should never happen given the particular thread structure using these
//  Pipe buffers
template <class T>
void MessageBuffer<T>::add_next ( T msg){

    delete_message_ahead();

    *(inP) = msg;
    increment_pointer( &(inP));

}


//  public routines
template <class T>
MessageBuffer<T>::MessageBuffer( ){

    lock = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
    inP = outP = buff;


}


template <class T>
MessageBuffer<T>::~MessageBuffer( ){

    //pthread_mutex_lock(&(lock));

    T msg;
    while(outP != inP  ){
        msg = get_next();
        //delete msg;
    }


    //pthread_mutex_unlock(&(lock));


}

template <class T>
void MessageBuffer<T>::Push( T message){

    pthread_mutex_lock(&(lock));

    add_next( message );

    pthread_mutex_unlock(&(lock));

}

template <class T>
T MessageBuffer<T>::Pop( ){

    pthread_mutex_lock(&(lock));

    T msg = get_next( );

    pthread_mutex_unlock(&(lock));

    return msg;

}

template <class T>
bool MessageBuffer<T>::HasMessage( ){

    pthread_mutex_lock(&(lock));

    bool isReady = (outP != inP);

    pthread_mutex_unlock(&(lock));

    return isReady;

}


#endif   ///  MessageBuffer_H


