/*
 * cartridge_banks.h -- Generic monitor bank registration for cartridge memory.
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

/*! \brief Describes a cartridge memory region exposed as monitor banks.
 *
 * The registry is for cartridge (or internal-expansion) memory that is
 * not always fully visible in the CPU address space -- for example RAM
 * behind an I/O window, or paged ROM where only the currently selected
 * bank is mapped.  The whole backing buffer is exposed linearly,
 * regardless of the cartridge's own banking state.
 *
 * Cartridges with all memory permanently mapped (e.g. simple 8 KiB ROMs,
 * Expert SRAM) do not need to register; they are already covered by the
 * existing @code{cart} bank.
 *
 * A single cartridge may register more than one of these structs to
 * expose distinct regions.  For example MMC Replay registers two: one
 * for its 512 KiB Flash ROM, one for its 512 KiB SRAM, with separate
 * prefixes (e.g. "mmcrf" and "mmcrr").
 *
 * Banks are always 64 KiB pages.  The read/write functions take a
 * linear byte address @code{(bank_index << 16) | offset} and access the
 * backing buffer directly -- bypassing any flash command sequences,
 * bank-select latches, etc. that the cartridge would normally apply for
 * CPU accesses.
 *
 * The write pointer may be NULL for true read-only regions (mask ROM).
 * Flash ROM should provide a write that hits the buffer directly so the
 * monitor's @code{>} (poke) command can modify bytes for debugging.
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
     * NULL for true read-only regions (mask ROM); the monitor's poke is
     * silently dropped.  Flash carts should provide a write that
     * modifies the backing buffer directly, ignoring any flash command
     * sequence logic the cart applies to normal CPU writes.
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

/*! \brief Register a cartridge memory region with the monitor bank system.
 *
 * Idempotent: if the info pointer is already registered, the generation
 * counter is bumped (useful after a resize where num_banks changed).
 * Call on cartridge enable, on resize (after updating num_banks), and
 * once for each region of a multi-region cart.
 *
 * \param info  Pointer to a caller-owned cart_bank_info_t. */
void cartridge_bank_register(cart_bank_info_t *info);

/*! \brief Unregister a cartridge memory region from the monitor bank system.
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
 * Consumers (machine mem modules via mem_bank_dynamic) cache this value
 * and rebuild their bank arrays when it differs from the cached copy. */
unsigned int cartridge_bank_generation(void);

#endif
