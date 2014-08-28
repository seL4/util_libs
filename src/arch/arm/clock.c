/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#include "clock.h"
#include <string.h>
#include <assert.h>
#include "../../services.h"


void
clk_print_tree(clk_t* clk, char* prefix)
{
    int depth = strlen(prefix);
    char new_prefix[depth + 2];
    strcpy(new_prefix, prefix);
    strcpy(new_prefix + depth, "|");
    while (clk != NULL) {
        const char* units[] = {"hz", "Khz", "Mhz", "Ghz"};
        const char** u = units;
        float freq = clk_get_freq(clk);
        /* Find frequency with appropriate units */
        while(freq > 10000){
            freq/= 1000;
            u++;
        }
        /* Generate tree graphics */
        if (clk->sibling == NULL) {
            strcpy(new_prefix + depth, " ");
        }
        printf("%s\\%s (%0.1f %s)\n", new_prefix, clk->name, freq, *u);
        clk_print_tree(clk->child, new_prefix);
        clk = clk->sibling;
    }
}

void
clk_register_child(clk_t* parent, clk_t* child)
{
    /* Lets make sure that we were initialised correctly
     * to avoid tree loops */
    if (child->parent != NULL) {
        /* If we are registered with a parent */
        clk_t* sibling = parent->child;
        /* Make sure that we are a sibling of the parent's child */
        while (sibling != child) {
            assert(sibling);
            sibling = sibling->sibling;
        }
    }
    if (child->parent == NULL) {
        child->parent = parent;
        child->sibling = parent->child;
        parent->child = child;
    } else if (child->parent != parent) {
        printf("%s->%s\n | %s already has parent %s",
               child->name, parent->name, child->name,
               child->parent->name);
        assert(!"Changing parents not supported\n");
    }
}

void
clk_recal(clk_t* clk)
{
    assert(clk);
    assert(clk->recal);
    clk->recal(clk);
}

