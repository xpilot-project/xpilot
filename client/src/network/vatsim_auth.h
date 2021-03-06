#ifndef VatsimAuth_h
#define VatsimAuth_h

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

char* GenerateAuthResponse(const char* challenge, const unsigned short public_key, const char* private_key);
char* GenerateAuthChallenge();
char* GetSystemUid();

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // !VatsimAuth_h
