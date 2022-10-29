#include "dependencyinjector.h"

namespace QInjection {

Injecter Inject;

Injecter::Injecter() : _key{nullptr} {}

Injecter::Injecter(const char *key) : _key(key) {}

} // namespace QInjection
