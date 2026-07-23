#ifndef BALLISTICS_REGISTRY_BUILTIN_REGISTRY_H
#define BALLISTICS_REGISTRY_BUILTIN_REGISTRY_H

#include "ballistics/export.h"
#include "ballistics/registry/equation_registry.h"
#include "ballistics/registry/force_model_registry.h"
#include "ballistics/registry/integrator_registry.h"
#include "ballistics/registry/writer_registry.h"

/** Register all statically linked Phase One equations. */
BALLISTICS_API BallisticsStatus
ballistics_register_builtin_equations(BallisticsEquationRegistry *registry);

/** Register all statically linked Phase One force models. */
BALLISTICS_API BallisticsStatus
ballistics_register_builtin_force_models(BallisticsForceModelRegistry *registry);

/** Register all statically linked Phase One integrators. */
BALLISTICS_API BallisticsStatus
ballistics_register_builtin_integrators(BallisticsIntegratorRegistry *registry);

/** Register all statically linked Phase One trajectory writers. */
BALLISTICS_API BallisticsStatus
ballistics_register_builtin_writers(BallisticsWriterRegistry *registry);

#endif
