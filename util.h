/*
   util.h
   mperron (2022)
*/

#ifndef KWS3_UTIL_H
#define KWS3_UTIL_H

#include <string>

// If a string contains \r or \n, replace the first instance of either with 0 (null-terminator).
std::string chomp(const std::string &str);

// Leading and trailing spaces are removed.
std::string trim(const std::string &str);

#endif
