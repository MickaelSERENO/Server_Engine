#ifndef  UTILS_INC
#define  UTILS_INC

#include <iostream>
#include <fstream>
#include <string>
#include <cstring>

//Color
#ifndef RED
#define RED   "\x1B[31m"
#endif
#ifndef GRN
#define GRN   "\x1B[32m"
#endif
#ifndef YEL
#define YEL   "\x1B[33m"
#endif
#ifndef BLU
#define BLU   "\x1B[34m"
#endif
#ifndef MAG
#define MAG   "\x1B[35m"
#endif
#ifndef CYN
#define CYN   "\x1B[36m"
#endif
#ifndef WHT
#define WHT   "\x1B[37m"
#endif
#ifndef BOLD
#define BOLD  "\x1B[1m"
#endif
#ifndef RESET
#define RESET "\x1B[0m"
#endif

//Stringification
#ifndef STR
#define STR_(x)  #x
#define STR(x)   STR_(x)
#endif

//Cat
#ifndef CAT
#define CAT_(x, y) x##y
#define CAT(x, y)  CAT_(x, y)
#endif

//Filename
#ifndef __FILENAME__
#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#endif

//Log system
#ifndef ERROR
#define ERROR   (std::cerr << BOLD RED "Error : "   RESET GRN  << __FILENAME__ << ":" STR(__LINE__) RESET " ")
#endif

#ifndef INFO
#define INFO    (std::cout << BOLD WHT "Info : "    RESET GRN  << __FILENAME__ << ":" STR(__LINE__) RESET " ")
#endif

#ifndef WARNING
#define WARNING (std::cout << BOLD YEL "Warning : " RESET GRN  << __FILENAME__ << ":" STR(__LINE__) RESET " ")
#endif

#endif
