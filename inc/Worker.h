/*
 * File:   Worker.h
 * Author: Pooh
 *
 * Created on September 16, 2018, 1:20 PM
 */

#ifndef Worker_H
#define Worker_H

#include <string>
#include <queue>
#include <mutex>
#include <thread>

using namespace std;


template<class T>
class Worker
{
    queue<T> fifo;
    mutex worker_mutex;
    condition_variable next;
    std::thread worker_thread;
    bool keep_working = true;
    mutex command_mutex;

    bool waitNext(){
        unique_lock<mutex> lock(worker_mutex);
        next.wait(lock, [this]{return !fifo.empty() || !keep_working;});
        return keep_working;
    }

    T popNext(){
        lock_guard<mutex> lock(worker_mutex);
        T next_to_do = fifo.front();
        fifo.pop();
        return next_to_do;
    }

    void processWorkerObjects(){
        while (waitNext()){

            T to_do = popNext();
            to_do.process();

            next.notify_all();

        }
    }

public:
    Worker(){
        worker_thread = std::thread(&Worker<T>::processWorkerObjects, this);
    }

    ~Worker(){
        shutdown(false);
    }

    void push( T to_do){

        lock_guard<mutex> lock(command_mutex);

        if( !keep_working )  throw  runtime_error("attempt to push a todo onto a shutwon worker");

        {
            lock_guard<mutex> lock(worker_mutex);
            fifo.push(to_do);
        }
        next.notify_all();
    }

    void shutdown(bool wait_complete){

        lock_guard<mutex> lock(command_mutex);
         if( keep_working ){
            if( wait_complete){
                unique_lock<mutex> lock(worker_mutex);
                next.wait(lock, [this]{return fifo.empty();});
             }

            {
                lock_guard<mutex> lock(worker_mutex);
                keep_working = false;
            }
            next.notify_all();
            worker_thread.join();
         }
     }

};


#endif   // Worker_H



