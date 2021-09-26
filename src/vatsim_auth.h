#ifndef VATSIMAUTH_H
#define VATSIMAUTH_H

#ifdef __cplusplus
extern "C" {
#endif

char* GenerateAuthResponse(const char* challenge, const char* key);
char* GenerateAuthChallenge();
unsigned short GetClientId();
char* GetSystemUid();

#ifdef __cplusplus
}
#endif

#endif
