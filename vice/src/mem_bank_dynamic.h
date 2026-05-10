/*
 * mem_bank_dynamic.h -- Per-machine helper that merges base bank tables
 *                       with the cartridge bank registry.
 *
 * Written by
 *  Jesper Balman Gravgaard <jesper@balmangravgaard.dk>
 *
 * This file is part of VICE, the Versatile Commodore Emulator.
 * See README for copyright notice.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 *  02111-1307  USA.
 *
 */

#ifndef VICE_MEM_BANK_DYNAMIC_H
#define VICE_MEM_BANK_DYNAMIC_H

#include "types.h"
#include "cartridge_banks.h"

/*! \brief Static configuration passed to mem_bank_dynamic_create().
 *
 * Holds pointers to the four parallel base-bank tables and the count
 * of valid entries.  The first bank number reserved for cart banks is
 * derived from the tables at create time as (max base bank number + 1).
 *
 * The config is read-only; the helper stores the pointer, not a copy. */
typedef struct mem_bank_dynamic_config_s {
    /*! \brief NULL-terminated array of base bank names. */
    const char *const *base_banknames;

    /*! \brief -1-terminated array of bank numbers, parallel to base_banknames. */
    const int *base_banknums;

    /*! \brief -1-terminated array of bank indices, parallel to base_banknames. */
    const int *base_bankindex;

    /*! \brief -1-terminated array of bank flags, parallel to base_banknames. */
    const int *base_bankflags;

    /*! \brief Number of valid entries in the base arrays, excluding the sentinel. */
    int num_base_banks;
} mem_bank_dynamic_config_t;

/*! \brief Opaque per-machine state. */
typedef struct mem_bank_dynamic_s mem_bank_dynamic_t;

/*! \brief Allocate and initialize a helper instance.
 *
 * The config pointer is stored, not copied, and must outlive the
 * returned helper. */
mem_bank_dynamic_t *mem_bank_dynamic_create(const mem_bank_dynamic_config_t *cfg);

/*! \brief Free a helper allocated with mem_bank_dynamic_create(). */
void mem_bank_dynamic_destroy(mem_bank_dynamic_t *self);

/*! \brief Return the merged (base + cart) bank-name list.
 *
 * NULL-terminated.  Owned by the helper; lifetime ends at the next
 * call that may rebuild (any of these accessors). */
const char **mem_bank_dynamic_list          (mem_bank_dynamic_t *self);

/*! \brief Return the merged bank-number list, parallel to _list().
 *
 * -1-terminated. */
const int   *mem_bank_dynamic_list_nos      (mem_bank_dynamic_t *self);

/*! \brief Return the bank number for @a name, or -1 if not present. */
int          mem_bank_dynamic_from_name     (mem_bank_dynamic_t *self, const char *name);

/*! \brief Return the bank index for @a bank, or -1 if not present. */
int          mem_bank_dynamic_index_from_bank(mem_bank_dynamic_t *self, int bank);

/*! \brief Return the bank flags for @a bank, or -1 if not present. */
int          mem_bank_dynamic_flags_from_bank(mem_bank_dynamic_t *self, int bank);

/*! \brief If @a bank is a registered cart bank, store the byte at
 *  @a addr in @a *out and return true.  Otherwise return false. */
bool mem_bank_dynamic_try_read (mem_bank_dynamic_t *self, int bank, uint16_t addr, uint8_t *out);

/*! \brief If @a bank is a registered cart bank, dispatch the write and
 *  return true (writes to a region with a NULL write pointer are
 *  dropped but still report true).  Otherwise return false. */
bool mem_bank_dynamic_try_write(mem_bank_dynamic_t *self, int bank, uint16_t addr, uint8_t value);

#endif
