#pragma once

#include <stdint.h>

void clear();
void loop();
void sleep();
void long_sleep();
void remove_clockdivide();

uint8_t magic_number();
uint16_t rand_();

void echo_on();
void echo_off();

void *_memcpy(void *s1, const void *s2, uint16_t n);