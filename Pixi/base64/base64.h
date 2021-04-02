#ifndef _BASE64_H_
#define _BASE64_H_
#include <string>
#include <vector>
std::string base64_encode(unsigned char const*, unsigned int len);
std::vector<uint8_t> base64_decode(const std::string& s);
#endif