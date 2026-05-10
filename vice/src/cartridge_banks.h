/*
 * cartridge_banks.h -- Registry of linear cartridge memory regions.
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

/*! \brief Registry entry describing one linear cartridge memory region.
 *
 * One entry advertises @code{num_banks} pages of 64 KiB each, named
 * @code{<prefix>00}..@code{<prefix>NN}.  A cartridge populates a static
 * cart_bank_info_t and calls cartridge_bank_register() to add it; a
 * cartridge with multiple regions registers multiple entries, each
 * with a unique prefix.
 *
 * Reads call @code{info->read(linear_addr)} where @code{linear_addr}
 * is @code{(bank_index << 16) | page_offset} and @code{bank_index}
 * ranges over @code{0..num_banks-1}.  Writes call @code{info->write}
 * with the same address layout; a NULL write pointer marks the region
 * read-only and writes through the registry are dropped.
 */
typedef struct cart_bank_info_s {
    /*! \brief Short lowercase prefix used to form bank names.
     *
     * Bank names are formed as @code{prefix%02x}, so a prefix of length
     * N produces names of length N+2.  Must be unique across all
     * registered structs (a cart with two regions uses two prefixes). */
    const char *prefix;

    /*! \brief Number of 64 KiB banks exposed. */
    int num_banks;

    /*! \brief Read one byte from a linear address within this region.
     *
     * \param addr  Linear byte address: (bank_index << 16) | page_offset.
     * \return      Byte value, or 0 if out of range. */
    uint8_t (*read)(unsigned int addr);

    /*! \brief Write one byte to a linear address within this region.
     *
     * NULL marks the region read-only; writes through the registry are
     * dropped.  Implementations should index the backing buffer
     * directly (no bank-select or flash command-sequence logic).
     *
     * \param addr   Linear byte address: (bank_index << 16) | page_offset.
     * \param value  Byte value to write. */
    void (*write)(unsigned int addr, uint8_t value);

    /*! \brief Bank number assigned to the first bank of this region.
     *
     * Set by cartridge_bank_register(); do not set manually. */
    int first_bank_num;

    /*! \brief Internal linked-list pointer. Do not use directly. */
    struct cart_bank_info_s *next;
} cart_bank_info_t;

/*! \brief Add a cart_bank_info_t to the registry.
 *
 * If @a info is already in the registry the entry is not duplicated;
 * the generation counter is bumped either way.
 *
 * \param info  Pointer to a caller-owned cart_bank_info_t. */
void cartridge_bank_register(cart_bank_info_t *info);

/*! \brief Remove a cart_bank_info_t from the registry.
 *
 * No-op if @a info is not currently in the registry.
 *
 * \param info  Pointer to the cart_bank_info_t to remove. */
void cartridge_bank_unregister(cart_bank_info_t *info);

/*! \brief Return the head of the registered cart_bank_info_t list.
 *
 * Iterate via @code{info->next}; the list terminates with NULL. */
cart_bank_info_t *cartridge_bank_list(void);

/*! \brief Return a counter that increments on every register/unregister. */
unsigned int cartridge_bank_generation(void);

#endif
