/*
 * Copyright 2017, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(DATA61_BSD)
 */

/* A simple vector math library
 *
 * This file provides the macro VECTOR_2D_DEFINITION(name, T)
 * which expands into a vector math library with vectors represented
 * by tuples of type "T", and generated symbols (type and function
 * names) prefixed with "name".
 *
 * Also provided in this file are 2 default implementations:
 * vector_2d_long_int (long int)
 * vector_2d_double (double)
 *
 * e.g.
 * VECTOR_2D_DEFINITION(vector_2d_int, long int)
 * would expand to
 *
 * typedef struct {
 *     long int x;
 *     long int y;
 * } vector_2d_int_t;
 * static inline void vector_2d_int_add(vector_2d_int_t *a, vector_2d_int_t *b, vector_2d_int_t *result) {
 *     result->x = a->x + b->x;
 *     result->y = a->y + b->y;
 * }
 * static inline void vector_2d_int_subtract(vector_2d_int_t *a, vector_2d_int_t *b, vector_2d_int_t *result) {
 * ...etc
 */

#pragma once

#include <math.h>

#define VECTOR_2D_DEFINITION(name, T) \
typedef struct {        \
    T x;                \
    T y;                \
} name##_t;             \
static inline void name##_add(name##_t *a, name##_t *b, name##_t *result) {\
    result->x = a->x + b->x;\
    result->y = a->y + b->y;\
}\
static inline void name##_subtract(name##_t *a, name##_t *b, name##_t *result) {\
    result->x = a->x - b->x;\
    result->y = a->y - b->y;\
}\
static inline void name##_scalar_multiply(name##_t *a, T scalar, name##_t *result) {\
    result->x = a->x * scalar;\
    result->y = a->y * scalar;\
}\
static inline double name##_length(name##_t *a) {\
    return sqrt(a->x*a->x + a->y*a->y);\
}\
static inline double name##_angle(name##_t *a) {\
    return atan2(a->y, a->x);\
}\
static inline void name##_from_cartesian(T x, T y, name##_t *result) {\
    result->x = x;\
    result->y = y;\
}\
static inline void name##_from_polar(double length, double radians, name##_t  *result) {\
    result->x = length * cos(radians);\
    result->y = length * sin(radians);\
}

/* Default implementations for long int and double */
VECTOR_2D_DEFINITION(vector_2d_long_int, long int)
VECTOR_2D_DEFINITION(vector_2d_double, double)
