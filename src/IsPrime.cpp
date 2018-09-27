/*
 * File:   IsPrime.cpp
 * Author: Pooh
 *
 * Created on September 18, 2018, 4:20 PM
 */

#include "IsPrime.h"

 bool IsPrime( int candidate){

     bool isPrime = true;

     for(int divCandidate = 2; divCandidate<= candidate/2; ++divCandidate)
     {
       if(candidate % divCandidate == 0)
       {
           isPrime = false;
           break;
       }
     }

    return isPrime;

 }