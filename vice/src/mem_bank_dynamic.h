/*
 * mem_bank_dynamic.h -- Per-machine helper that merges base bank tables
 *                       with the cartridge bank registry for the monitor.
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

/*! \brief Static per-machine configuration for the dynamic bank helper.
 *
 * Each machine mem module supplies the four parallel base-bank tables it
 * already used (NULL/-1 sentinels as appropriate) and the fixed-bank count.
 * The first bank number reserved for cartridge banks is derived from the
 * tables at create time as (max base bank number + 1).
 *
 * The config is read-only and typically a static const in the machine. */
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

/*! \brief Allocate and initialise a dynamic-bank helper for one machine.
 *
 * The config pointer is stored, not copied, and must outlive the helper. */
mem_bank_dynamic_t *mem_bank_dynamic_create(const mem_bank_dynamic_config_t *cfg);

/*! \brief Free a helper allocated with mem_bank_dynamic_create(). */
void mem_bank_dynamic_destroy(mem_bank_dynamic_t *self);

/*! \brief Bank-list accessors -- forward these from per-machine
 *  mem_bank_list / mem_bank_list_nos / mem_bank_from_name etc. */
const char **mem_bank_dynamic_list          (mem_bank_dynamic_t *self);
const int   *mem_bank_dynamic_list_nos      (mem_bank_dynamic_t *self);
int          mem_bank_dynamic_from_name     (mem_bank_dynamic_t *self, const char *name);
int          mem_bank_dynamic_index_from_bank(mem_bank_dynamic_t *self, int bank);
int          mem_bank_dynamic_flags_from_bank(mem_bank_dynamic_t *self, int bank);

/*! \brief Try to read a byte from a registered cartridge bank.
 *
 * If the bank refers to a registered cart, sets *out and returns true.
 * Otherwise returns false; the caller must fall back to its base-bank
 * dispatch.  Used by both mem_bank_read and mem_bank_peek (cartridge
 * memory access has no side effects). */
bool mem_bank_dynamic_try_read (mem_bank_dynamic_t *self, int bank, uint16_t addr, uint8_t *out);

/*! \brief Try to write a byte to a registered cartridge bank.
 *
 * Returns true if the bank refers to a registered cart and the write was
 * dispatched (or silently dropped because the cart's write pointer is
 * NULL, indicating a read-only region).  Returns false if the bank is
 * not a registered cart bank, in which case the caller must fall back
 * to its base-bank dispatch. */
bool mem_bank_dynamic_try_write(mem_bank_dynamic_t *self, int bank, uint16_t addr, uint8_t value);

#endif
