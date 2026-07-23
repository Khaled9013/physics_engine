#ifndef BALLISTICS_REGISTRY_BUILTIN_REGISTRY_H
#define BALLISTICS_REGISTRY_BUILTIN_REGISTRY_H

#include "ballistics/export.h"
#include "ballistics/registry/equation_registry.h"

/** Register all statically linked Phase One equations. */
BALLISTICS_API BallisticsStatus
ballistics_register_builtin_equations(BallisticsEquationRegistry *registry);

#endif
