/*
 * cartridge_banks.h -- Generic monitor bank registration for cartridge RAM.
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

#ifndef VICE_CARTRIDGE_BANKS_H
#define VICE_CARTRIDGE_BANKS_H

#include "types.h"

/*! \brief Describes a cartridge RAM region exposed as monitor banks.
 *
 * Each cartridge or expansion that wants to expose its RAM to the monitor
 * allocates one of these structs (typically as a static variable), fills
 * in the fields, and calls cartridge_bank_register().  The monitor bank
 * system then includes the cartridge's banks in its bank list without
 * needing to know anything about the specific cartridge.
 *
 * Banks are always presented as 64 KiB pages.  The read/write functions
 * receive a linear byte address: (bank_index << 16) | offset.
 *
 * The write pointer may be NULL for read-only regions (e.g. flash ROM).
 */
typedef struct cart_bank_info_s {
    /*! \brief Short prefix used to form bank names, e.g. "reu" -> "reu00".
     *
     * Must be unique across all registered cartridges.  Use lowercase. */
    const char *prefix;

    /*! \brief Number of 64 KiB banks exposed. */
    int num_banks;

    /*! \brief Total RAM size in bytes (num_banks * 65536). */
    unsigned int total_size;

    /*! \brief Read one byte from a linear address within this cartridge RAM.
     *
     * \param addr  Linear byte address: (bank_index << 16) | page_offset.
     * \return      Byte value, or 0 if out of range. */
    uint8_t (*read)(unsigned int addr);

    /*! \brief Write one byte to a linear address within this cartridge RAM.
     *
     * May be NULL for read-only regions.
     *
     * \param addr   Linear byte address: (bank_index << 16) | page_offset.
     * \param value  Byte value to write. */
    void (*write)(unsigned int addr, uint8_t value);

    /*! \brief Bank number assigned to the first bank of this cartridge.
     *
     * Set by cartridge_bank_register(); do not set manually. */
    int first_bank_num;

    /*! \brief Internal linked-list pointer. Do not use directly. */
    struct cart_bank_info_s *next;
} cart_bank_info_t;

/*! \brief Register a cartridge RAM region with the monitor bank system.
 *
 * Idempotent: if the info pointer is already registered, the existing
 * entry is updated in place and the generation counter is bumped.
 * Call this on cartridge enable and on resize (after updating num_banks
 * and total_size).
 *
 * \param info  Pointer to a caller-owned cart_bank_info_t. */
void cartridge_bank_register(cart_bank_info_t *info);

/*! \brief Unregister a cartridge RAM region from the monitor bank system.
 *
 * Safe to call even if the info is not currently registered.
 *
 * \param info  Pointer to the cart_bank_info_t to remove. */
void cartridge_bank_unregister(cart_bank_info_t *info);

/*! \brief Return the head of the registered cartridge bank list.
 *
 * Iterate with info->next; the list ends when next is NULL. */
cart_bank_info_t *cartridge_bank_list(void);

/*! \brief Return a generation counter that increases on every change.
 *
 * Machine mem modules cache this value and call rebuild_bank_arrays()
 * when it differs from their cached copy. */
unsigned int cartridge_bank_generation(void);

#endif
