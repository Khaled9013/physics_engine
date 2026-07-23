#include "ballistics/math/vector3.h"

#include <math.h>
#include <stddef.h>

bool ballistics_vector3_is_finite(const BallisticsVector3 *vector)
{
    return vector != NULL && isfinite(vector->x) && isfinite(vector->y) && isfinite(vector->z);
}

BallisticsStatus ballistics_vector3_add(const BallisticsVector3 *left,
                                        const BallisticsVector3 *right,
                                        BallisticsVector3 *out_result)
{
    if (!ballistics_vector3_is_finite(left) || !ballistics_vector3_is_finite(right) ||
        out_result == NULL)
    {
        return BALLISTICS_STATUS_INVALID_ARGUMENT;
    }
    out_result->x = left->x + right->x;
    out_result->y = left->y + right->y;
    out_result->z = left->z + right->z;
    return ballistics_vector3_is_finite(out_result) ? BALLISTICS_STATUS_OK
                                                    : BALLISTICS_STATUS_NUMERICAL_ERROR;
}

BallisticsStatus ballistics_vector3_subtract(const BallisticsVector3 *left,
                                             const BallisticsVector3 *right,
                                             BallisticsVector3 *out_result)
{
    if (!ballistics_vector3_is_finite(left) || !ballistics_vector3_is_finite(right) ||
        out_result == NULL)
    {
        return BALLISTICS_STATUS_INVALID_ARGUMENT;
    }
    out_result->x = left->x - right->x;
    out_result->y = left->y - right->y;
    out_result->z = left->z - right->z;
    return ballistics_vector3_is_finite(out_result) ? BALLISTICS_STATUS_OK
                                                    : BALLISTICS_STATUS_NUMERICAL_ERROR;
}

BallisticsStatus ballistics_vector3_scale(const BallisticsVector3 *vector,
                                          double scalar,
                                          BallisticsVector3 *out_result)
{
    if (!ballistics_vector3_is_finite(vector) || !isfinite(scalar) || out_result == NULL)
    {
        return BALLISTICS_STATUS_INVALID_ARGUMENT;
    }
    out_result->x = vector->x * scalar;
    out_result->y = vector->y * scalar;
    out_result->z = vector->z * scalar;
    return ballistics_vector3_is_finite(out_result) ? BALLISTICS_STATUS_OK
                                                    : BALLISTICS_STATUS_NUMERICAL_ERROR;
}

BallisticsStatus ballistics_vector3_dot(const BallisticsVector3 *left,
                                        const BallisticsVector3 *right,
                                        double *out_result)
{
    if (!ballistics_vector3_is_finite(left) || !ballistics_vector3_is_finite(right) ||
        out_result == NULL)
    {
        return BALLISTICS_STATUS_INVALID_ARGUMENT;
    }
    *out_result = (left->x * right->x) + (left->y * right->y) + (left->z * right->z);
    return isfinite(*out_result) ? BALLISTICS_STATUS_OK : BALLISTICS_STATUS_NUMERICAL_ERROR;
}

BallisticsStatus ballistics_vector3_magnitude(const BallisticsVector3 *vector,
                                              double *out_magnitude)
{
    if (!ballistics_vector3_is_finite(vector) || out_magnitude == NULL)
    {
        return BALLISTICS_STATUS_INVALID_ARGUMENT;
    }
    *out_magnitude = hypot(hypot(vector->x, vector->y), vector->z);
    return isfinite(*out_magnitude) ? BALLISTICS_STATUS_OK : BALLISTICS_STATUS_NUMERICAL_ERROR;
}

BallisticsStatus ballistics_vector3_normalize(const BallisticsVector3 *vector,
                                              BallisticsVector3 *out_normalized)
{
    double magnitude;
    BallisticsStatus status;

    if (out_normalized == NULL)
    {
        return BALLISTICS_STATUS_INVALID_ARGUMENT;
    }
    status = ballistics_vector3_magnitude(vector, &magnitude);
    if (status != BALLISTICS_STATUS_OK)
    {
        return status;
    }
    if (magnitude == 0.0)
    {
        return BALLISTICS_STATUS_INVALID_ARGUMENT;
    }
    return ballistics_vector3_scale(vector, 1.0 / magnitude, out_normalized);
}
