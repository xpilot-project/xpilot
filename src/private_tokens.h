#ifndef PRIVATE_TOKENS_H
#define PRIVATE_TOKENS_H

#include <QString>

namespace Xpilot
{
    class PrivateTokens
    {
    public:
        static int VatsimClientId();
        static const QString &VatsimPrivateKey();
        static const QString &SentryToken();
    };
}

#endif
