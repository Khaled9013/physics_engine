#include "ballistics/interfaces/equation_interface.h"

#include <stddef.h>

BallisticsStatus ballistics_equation_initialize(BallisticsEquation *equation)
{
    if (equation == NULL || equation->vtable == NULL || equation->vtable->evaluate == NULL ||
        equation->vtable->destroy == NULL)
    {
        return BALLISTICS_STATUS_INVALID_ARGUMENT;
    }
    if (equation->vtable->initialize == NULL)
    {
        return BALLISTICS_STATUS_OK;
    }
    return equation->vtable->initialize(equation);
}

BallisticsStatus ballistics_equation_evaluate(const BallisticsEquation *equation,
                                              const void *input,
                                              size_t input_size,
                                              void *output,
                                              size_t output_size)
{
    if (equation == NULL || equation->vtable == NULL || equation->vtable->evaluate == NULL ||
        input == NULL || input_size == 0U || output == NULL || output_size == 0U)
    {
        return BALLISTICS_STATUS_INVALID_ARGUMENT;
    }
    return equation->vtable->evaluate(equation, input, input_size, output, output_size);
}

void ballistics_equation_destroy(BallisticsEquation *equation)
{
    if (equation != NULL && equation->vtable != NULL && equation->vtable->destroy != NULL)
    {
        equation->vtable->destroy(equation);
    }
}
