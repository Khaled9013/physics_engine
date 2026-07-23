#ifndef BALLISTICS_MATH_VECTOR3_H
#define BALLISTICS_MATH_VECTOR3_H

#include "ballistics/export.h"
#include "ballistics/status.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Three-dimensional vector using the project SI coordinate convention. */
typedef struct
{
    double x;
    double y;
    double z;
} BallisticsVector3;

BALLISTICS_API BallisticsStatus ballistics_vector3_add(const BallisticsVector3 *left,
                                                       const BallisticsVector3 *right,
                                                       BallisticsVector3 *out_result);
BALLISTICS_API BallisticsStatus ballistics_vector3_subtract(const BallisticsVector3 *left,
                                                            const BallisticsVector3 *right,
                                                            BallisticsVector3 *out_result);
BALLISTICS_API BallisticsStatus ballistics_vector3_scale(const BallisticsVector3 *vector,
                                                         double scalar,
                                                         BallisticsVector3 *out_result);
BALLISTICS_API BallisticsStatus ballistics_vector3_dot(const BallisticsVector3 *left,
                                                       const BallisticsVector3 *right,
                                                       double *out_result);
BALLISTICS_API BallisticsStatus ballistics_vector3_magnitude(const BallisticsVector3 *vector,
                                                             double *out_magnitude);
BALLISTICS_API BallisticsStatus ballistics_vector3_normalize(const BallisticsVector3 *vector,
                                                             BallisticsVector3 *out_normalized);
BALLISTICS_API bool ballistics_vector3_is_finite(const BallisticsVector3 *vector);

#ifdef __cplusplus
}
#endif

#endif
