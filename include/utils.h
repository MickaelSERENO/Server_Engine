#ifndef  UTILS_INC
#define  UTILS_INC

#include <iostream>
#include <fstream>
#include <string>
#include <cstring>

//Color
#define RED   "\x1B[31m"
#define GRN   "\x1B[32m"
#define YEL   "\x1B[33m"
#define BLU   "\x1B[34m"
#define MAG   "\x1B[35m"
#define CYN   "\x1B[36m"
#define WHT   "\x1B[37m"
#define BOLD  "\x1B[1m"
#define RESET "\x1B[0m"

//Stringification
#define STR_(x)  #x
#define STR(x)   STR_(x)

//Cat
#define CAT_(x, y) x##y
#define CAT(x, y)  CAT_(x, y)

//Filename
#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

//Log system
#define ERROR (std::cerr << RED "Error : " GRN  << __FILENAME__ << ":" STR(__LINE__) RESET " ")
#define INFO (std::cout << YEL "Info : " GRN  << __FILENAME__ << ":" STR(__LINE__) RESET " ")

#endif
