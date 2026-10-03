#ifndef TABLE_DISPATCH_H
#define TABLE_DISPATCH_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

/* HCI event codes that select a sub-table (same values as EVT_LE_META_EVENT and EVT_VENDOR). */
#define TABLE_DISPATCH_EVT_LE_META 0x3EU
#define TABLE_DISPATCH_EVT_VENDOR 0xFFU

/* Returned by TableDispatch_Find when no entry has the code. */
#define TABLE_DISPATCH_NOT_FOUND SIZE_MAX

typedef enum
{
    TABLE_DISPATCH_PLAIN = 0,
    TABLE_DISPATCH_LE_META = 1,
    TABLE_DISPATCH_VENDOR = 2
} TableDispatch_Route_t;

/* clang-format off */
/*
 * Looking up the handler of an HCI event, one routine for the three event tables.
 *
 * TableDispatch_Route: which table an HCI event code is looked up in: LE_META for
 * TABLE_DISPATCH_EVT_LE_META, VENDOR for TABLE_DISPATCH_EVT_VENDOR, PLAIN for every other code.
 *
 * TableDispatch_Find: the index of the first entry of a table whose 16-bit code equals `code`,
 * or TABLE_DISPATCH_NOT_FOUND. The table is an array of `entry_count` entries of `entry_size`
 * bytes each; the code is read from `code_offset` bytes into an entry as two bytes, least
 * significant first (the uint16_t member of the entry struct on a little-endian machine such as
 * the STM32 and the PC the unit tests run on). NOT_FOUND is also returned for a NULL
 * table, an empty table, an entry too small to hold the code at that offset, and a table size
 * that would overflow size_t.
 */
TableDispatch_Route_t TableDispatch_Route(uint8_t event_code);
size_t TableDispatch_Find(const uint8_t* table, size_t entry_size, size_t entry_count,
                          size_t code_offset, uint16_t code);
/* clang-format on */

#ifdef __cplusplus
}
#endif

#endif /* TABLE_DISPATCH_H */
