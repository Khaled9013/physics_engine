#ifndef BALLISTICS_REGISTRY_BUILTIN_REGISTRY_H
#define BALLISTICS_REGISTRY_BUILTIN_REGISTRY_H

#include "ballistics/export.h"
#include "ballistics/registry/equation_registry.h"
#include "ballistics/registry/force_model_registry.h"

/** Register all statically linked Phase One equations. */
BALLISTICS_API BallisticsStatus
ballistics_register_builtin_equations(BallisticsEquationRegistry *registry);

/** Register all statically linked Phase One force models. */
BALLISTICS_API BallisticsStatus
ballistics_register_builtin_force_models(BallisticsForceModelRegistry *registry);

#endif
