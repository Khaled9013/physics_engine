#ifndef BALLISTICS_REGISTRY_VALIDATION_H
#define BALLISTICS_REGISTRY_VALIDATION_H

#include <stdbool.h>
#include <stddef.h>

static inline bool ballistics_registry_identifier_is_valid(const char *identifier)
{
    size_t index;

    if (identifier == NULL || identifier[0] == '\0')
    {
        return false;
    }
    for (index = 0U; identifier[index] != '\0'; ++index)
    {
        const char character = identifier[index];
        const bool is_lowercase = character >= 'a' && character <= 'z';
        const bool is_digit = character >= '0' && character <= '9';
        if (!is_lowercase && !is_digit && character != '-' && character != '.')
        {
            return false;
        }
    }
    return true;
}

#endif
