#include "table_dispatch.h"

#include <stdbool.h>

TableDispatch_Route_t TableDispatch_Route(uint8_t event_code)
{
    TableDispatch_Route_t route = TABLE_DISPATCH_PLAIN;

    if (event_code == TABLE_DISPATCH_EVT_LE_META)
    {
        route = TABLE_DISPATCH_LE_META;
    }
    else if (event_code == TABLE_DISPATCH_EVT_VENDOR)
    {
        route = TABLE_DISPATCH_VENDOR;
    }
    else
    {
        /* every other event is looked up in the plain table */
    }

    return route;
}

size_t TableDispatch_Find(const uint8_t* table, size_t entry_size, size_t entry_count,
                          size_t code_offset, uint16_t code)
{
    size_t found = TABLE_DISPATCH_NOT_FOUND;
    const bool usable = (table != NULL) && (entry_count != 0U) &&
                        (entry_size >= sizeof(uint16_t)) &&
                        (code_offset <= (entry_size - sizeof(uint16_t))) &&
                        (entry_count <= (SIZE_MAX / entry_size));

    for (size_t i = 0U; usable && (found == TABLE_DISPATCH_NOT_FOUND) && (i < entry_count); i++)
    {
        const size_t at = (i * entry_size) + code_offset;
        const uint16_t entry_code =
            (uint16_t)((uint16_t)table[at] | (uint16_t)((uint16_t)table[at + 1U] << 8U));

        if (entry_code == code)
        {
            found = i;
        }
    }

    return found;
}
