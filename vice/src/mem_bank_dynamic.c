/*
 * mem_bank_dynamic.c -- Per-machine helper that merges base bank tables
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

#include "vice.h"

#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "lib.h"
#include "mem.h"
#include "mem_bank_dynamic.h"

struct mem_bank_dynamic_s {
    const mem_bank_dynamic_config_t *cfg;

    /* First bank number assigned to cart banks: max(base_banknums) + 1.
       Computed once at create time. */
    int cart_banks_start;

    /* Dynamic arrays rebuilt when the cartridge bank registry changes.
       NULL when no cart banks are currently registered, in which case
       the accessors return the base tables directly. */
    char **dyn_banknames;
    int  *dyn_banknums;
    int  *dyn_bankindex;
    int  *dyn_bankflags;

    /* Generation counter cached from the last rebuild. */
    unsigned int cached_generation;
};

static void free_arrays(mem_bank_dynamic_t *self)
{
    int i;
    int num_cart_banks;

    if (self->dyn_banknames == NULL) {
        return;
    }

    num_cart_banks = 0;
    for (i = self->cfg->num_base_banks; self->dyn_banknums[i] != -1; i++) {
        num_cart_banks++;
    }

    for (i = 0; i < num_cart_banks; i++) {
        lib_free(self->dyn_banknames[self->cfg->num_base_banks + i]);
    }
    lib_free(self->dyn_banknames);
    lib_free(self->dyn_banknums);
    lib_free(self->dyn_bankindex);
    lib_free(self->dyn_bankflags);

    self->dyn_banknames = NULL;
    self->dyn_banknums  = NULL;
    self->dyn_bankindex = NULL;
    self->dyn_bankflags = NULL;
}

static void rebuild(mem_bank_dynamic_t *self)
{
    cart_bank_info_t *cart;
    int total_cart_banks = 0;
    int total, next_bank_num, i, j;
    int num_base = self->cfg->num_base_banks;

    free_arrays(self);

    next_bank_num = self->cart_banks_start;
    for (cart = cartridge_bank_list(); cart != NULL; cart = cart->next) {
        cart->first_bank_num = next_bank_num;
        next_bank_num += cart->num_banks;
        total_cart_banks += cart->num_banks;
    }

    if (total_cart_banks == 0) {
        self->cached_generation = cartridge_bank_generation();
        return;
    }

    total = num_base + total_cart_banks;

    self->dyn_banknames = lib_malloc((size_t)(total + 1) * sizeof(char *));
    self->dyn_banknums  = lib_malloc((size_t)(total + 1) * sizeof(int));
    self->dyn_bankindex = lib_malloc((size_t)(total + 1) * sizeof(int));
    self->dyn_bankflags = lib_malloc((size_t)(total + 1) * sizeof(int));

    for (i = 0; i < num_base; i++) {
        self->dyn_banknames[i] = (char *)self->cfg->base_banknames[i];
        self->dyn_banknums [i] = self->cfg->base_banknums [i];
        self->dyn_bankindex[i] = self->cfg->base_bankindex[i];
        self->dyn_bankflags[i] = self->cfg->base_bankflags[i];
    }

    j = num_base;
    for (cart = cartridge_bank_list(); cart != NULL; cart = cart->next) {
        int k;

        for (k = 0; k < cart->num_banks; k++) {
            char name[16];
            int flags = MEM_BANK_ISARRAY;

            snprintf(name, sizeof(name), "%s%02x", cart->prefix, (unsigned int)k);
            self->dyn_banknames[j] = lib_strdup(name);
            self->dyn_banknums [j] = cart->first_bank_num + k;
            self->dyn_bankindex[j] = k;

            if (k == 0) {
                flags |= MEM_BANK_ISARRAYFIRST;
            }
            if (k == cart->num_banks - 1) {
                flags |= MEM_BANK_ISARRAYLAST;
            }
            self->dyn_bankflags[j] = flags;
            j++;
        }
    }

    self->dyn_banknames[total] = NULL;
    self->dyn_banknums [total] = -1;
    self->dyn_bankindex[total] = -1;
    self->dyn_bankflags[total] = -1;

    self->cached_generation = cartridge_bank_generation();
}

static void check_rebuild(mem_bank_dynamic_t *self)
{
    if (cartridge_bank_generation() != self->cached_generation) {
        rebuild(self);
    }
}

static cart_bank_info_t *find_cart_for_bank(int bank)
{
    cart_bank_info_t *cart;

    for (cart = cartridge_bank_list(); cart != NULL; cart = cart->next) {
        if (bank >= cart->first_bank_num
            && bank < cart->first_bank_num + cart->num_banks) {
            return cart;
        }
    }
    return NULL;
}

mem_bank_dynamic_t *mem_bank_dynamic_create(const mem_bank_dynamic_config_t *cfg)
{
    mem_bank_dynamic_t *self = lib_malloc(sizeof(*self));
    int i, max_bank_num = -1;

    /* cart_banks_start = max(base_banknums) + 1 */
    for (i = 0; i < cfg->num_base_banks; i++) {
        if (cfg->base_banknums[i] > max_bank_num) {
            max_bank_num = cfg->base_banknums[i];
        }
    }

    self->cfg = cfg;
    self->cart_banks_start = max_bank_num + 1;
    self->dyn_banknames = NULL;
    self->dyn_banknums  = NULL;
    self->dyn_bankindex = NULL;
    self->dyn_bankflags = NULL;
    self->cached_generation = (unsigned int)-1;
    return self;
}

void mem_bank_dynamic_destroy(mem_bank_dynamic_t *self)
{
    if (self == NULL) {
        return;
    }
    free_arrays(self);
    lib_free(self);
}

const char **mem_bank_dynamic_list(mem_bank_dynamic_t *self)
{
    check_rebuild(self);
    return (const char **)(self->dyn_banknames
                           ? self->dyn_banknames
                           : (char **)self->cfg->base_banknames);
}

const int *mem_bank_dynamic_list_nos(mem_bank_dynamic_t *self)
{
    check_rebuild(self);
    return self->dyn_banknums ? self->dyn_banknums : self->cfg->base_banknums;
}

int mem_bank_dynamic_from_name(mem_bank_dynamic_t *self, const char *name)
{
    const char **names;
    const int *nums;
    int i = 0;

    check_rebuild(self);
    names = (const char **)(self->dyn_banknames
                            ? self->dyn_banknames
                            : (char **)self->cfg->base_banknames);
    nums  = self->dyn_banknums ? self->dyn_banknums : self->cfg->base_banknums;

    while (names[i]) {
        if (!strcmp(name, names[i])) {
            return nums[i];
        }
        i++;
    }
    return -1;
}

int mem_bank_dynamic_index_from_bank(mem_bank_dynamic_t *self, int bank)
{
    const int *nums;
    const int *idx;
    int i = 0;

    check_rebuild(self);
    nums = self->dyn_banknums ? self->dyn_banknums : self->cfg->base_banknums;
    idx  = self->dyn_bankindex ? self->dyn_bankindex : self->cfg->base_bankindex;

    while (nums[i] > -1) {
        if (nums[i] == bank) {
            return idx[i];
        }
        i++;
    }
    return -1;
}

int mem_bank_dynamic_flags_from_bank(mem_bank_dynamic_t *self, int bank)
{
    const int *nums;
    const int *flags;
    int i = 0;

    check_rebuild(self);
    nums  = self->dyn_banknums ? self->dyn_banknums : self->cfg->base_banknums;
    flags = self->dyn_bankflags ? self->dyn_bankflags : self->cfg->base_bankflags;

    while (nums[i] > -1) {
        if (nums[i] == bank) {
            return flags[i];
        }
        i++;
    }
    return -1;
}

bool mem_bank_dynamic_try_read(mem_bank_dynamic_t *self, int bank, uint16_t addr, uint8_t *out)
{
    cart_bank_info_t *cart;

    if (bank < self->cart_banks_start) {
        return false;
    }
    cart = find_cart_for_bank(bank);
    if (cart == NULL) {
        return false;
    }
    *out = cart->read(((unsigned int)(bank - cart->first_bank_num) << 16) | addr);
    return true;
}

bool mem_bank_dynamic_try_write(mem_bank_dynamic_t *self, int bank, uint16_t addr, uint8_t value)
{
    cart_bank_info_t *cart;

    if (bank < self->cart_banks_start) {
        return false;
    }
    cart = find_cart_for_bank(bank);
    if (cart == NULL) {
        return false;
    }
    if (cart->write != NULL) {
        cart->write(((unsigned int)(bank - cart->first_bank_num) << 16) | addr, value);
    }
    return true;
}
