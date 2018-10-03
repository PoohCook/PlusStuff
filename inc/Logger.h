/*
 * File:   Logger.h
 * Author: Pooh
 *
 * Created on September 28, 2018, 4:41 PM
 */

 #ifndef Logger_H
 #define Logger_H

#ifdef DIAG_MESSAGES

#define LogMessage(msg) (cout << string(string(msg) + "\n"))

#else

#define LogMessage(msg)  {}

#endif


#define LogMessageX(msg) (cout << string(msg) << "\n")

#endif   // Logger_H

