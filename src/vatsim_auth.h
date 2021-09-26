#ifndef VATSIMAUTH_H
#define VATSIMAUTH_H

#ifdef __cplusplus
extern "C" {
#endif

char* GenerateAuthResponse(const char* challenge, const char* key, unsigned short clientId);
char* GenerateAuthChallenge();
char* GetMd5Digest(const char* value);

#ifdef __cplusplus
}
#endif

#endif
