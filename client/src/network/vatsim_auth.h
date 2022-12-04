#ifndef VatsimAuth_h
#define VatsimAuth_h

#include <string>

std::string GenerateAuthResponse(const std::string& challenge, const unsigned short publicKey, const std::string& privateKey);
std::string GenerateAuthChallenge();

#endif // !VatsimAuth_h
