/*
 * Copyright 2019, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(DATA61_BSD)
 */

#include <stdbool.h>
#include <stddef.h>
#include <utils/util.h>
#include <platsupport/gpio_utils.h>

/* This is forward declared in the header file. */
struct gpio_chain {
    list_t pin_list;
};

struct gpio_callback_token {
    bool is_read;
    char *buffer;
    size_t bits_processed;
    size_t nbits_to_process;
}

static int gpio_chain_comparator(void *a, void *b)
{
    gpio_t *gpio_a = a;
    gpio_t *gpio_b = b;
    
    if (gpio_a->id == gpio_b->id) {
        return 0;
    }

    return -1;
}

int gpio_chain_init(ps_malloc_ops_t *malloc_ops, gpio_chain_t **ret_chain)
{
    if (!malloc_ops || !ret_chain) {
        ZF_LOGE("Arguments are NULL!");
        return -EINVAL;
    }

    int error = ps_calloc(malloc_ops, 1, sizeof(**ret_chain), (void **) ret_chain);
    if (error) {
        return -ENOMEM;
    }

    error = list_init(&(*ret_chain)->pin_list);
    if (error) {
        ps_free(malloc_ops, sizeof(**ret_chain), *ret_chain);
        return error;
    }

    return 0;
}

int gpio_chain_destroy(ps_malloc_ops_t *malloc_ops, gpio_chain_t *chain)
{
    if (!malloc_ops || !chain) {
        ZF_LOGE("Arguments are NULL!");
        return -EINVAL;
    }

    /* We're expected to have removed all elements from
     * the list before destroying it */
    int error = list_remove_all(&chain->pin_list);
    if (error) {
        return error;
    }

    error = list_destroy(&chain->pin_list);
    if (error) {
        return error;
    }

    ps_free(malloc_ops, sizeof(*chain), chain);

    return 0;
}

int gpio_chain_add(gpio_chain_t *chain, gpio_t *gpio)
{
    if (!chain || !gpio) {
        ZF_LOGE("Arguments are NULL!");
        return -EINVAL;
    }

    int error = list_append(&chain->pin_list, (void *) gpio);
    if (error) {
        return error;
    }

    return 0;
}

int gpio_chain_remove(gpio_chain_t *chain, gpio_t *gpio)
{
    if (!chain || !gpio) {
        ZF_LOGE("Arguments are NULL!");
        return -EINVAL;
    }

    int not_found = list_remove(&chain->pin_list, gpio, gpio_chain_comparator);
    if (not_found) {
        return -ENOENT;
    }
    return 0;
}

int gpio_chain_io_callback(void *data, void *token)
{
    gpio_t *gpio = data;
    struct gpio_callback_token *cb_token = token;

    /* Do nothing if we've already processed enough bits */
    if (cb_token->nbits_to_process == cb_token->bits_processed) {
        return 0;
    }

    size_t curr_byte = cb_token->bits_processed / sizeof(*(cb_token->buffer));
    size_t curr_bit = cb_token->bits_processed % sizeof(*(cb_token->buffer)); 

    if (cb_token->is_read) {
        int val = gpio_get(gpio);
        
        val <<= curr_bit;
        cb_token->buffer[curr_byte] &= ~BIT(curr_bit);
        cb_token->buffer[curr_byte] |= val;
    } else {
        if (cb_token->buffer[curr_byte] & BIT(curr_bit)) {
            gpio_set(gpio);
        } else {
            gpio_clr(gpio);
        } 
    }
    
    cb_token->bits_processed++;

    return 0;
}

int gpio_chain_read(gpio_chain_t *chain, char *data, int len)
{
    if (!chain || !data) {
        ZF_LOGE("Arguments are NULL!");
        return -EINVAL;
    }

    struct gpio_callback_token cb_token = { .is_read = true, .buffer = data, .bits_processed = 0,
                                            .nbits_to_process = len };

    int error = list_foreach(&chain->pin_list, gpio_chain_io_callback, &cb_token);
    if (error) {
        return error;
    }

    return cb_token->bits_processed;
}

int gpio_chain_write(gpio_chain_t *chain, const char *data, int len)
{
    if (!chain || !data) {
        ZF_LOGE("Arguments are NULL!");
        return -EINVAL;
    }

    struct gpio_callback_token cb_token = { .is_read = false, .buffer = data, .bits_processed = 0,
                                            .nbits_to_process = len };

    int error = list_foreach(&chain->pin_list, gpio_chain_io_callback, &cb_token);
    if (error) {
        return error;
    }

    return cb_token->bits_processed;
}
