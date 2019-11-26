#include "keepkey/firmware/life.h"

#include "keepkey/board/draw.h"
#include "keepkey/board/keepkey_button.h"
#include "keepkey/board/keepkey_display.h"
#include "keepkey/board/timer.h"
#include "keepkey/board/variant.h"
#include "trezor/crypto/rand.h"
#include <limits.h>
#include <stdint.h>
#include <string.h>

static Canvas *canvas;
static uint8_t new_buffer[KEEPKEY_DISPLAY_HEIGHT * KEEPKEY_DISPLAY_WIDTH];

static const unsigned int intervals[] = {16, 32, 64, 128, 256, 512, 1024};
int interval = 0;

#define SCALE_COLOR(color) (!!(color) * (uint8_t)-1)

// Returns [0, b) even for negative a
static inline int modulo(int a, int b) {
	return (a % b + b) % b;
}

static uint8_t get_pixel(unsigned char buffer[KEEPKEY_DISPLAY_HEIGHT * KEEPKEY_DISPLAY_WIDTH], int row, int column) {
	return buffer[row * KEEPKEY_DISPLAY_WIDTH + column];
}

static void set_pixel(unsigned char buffer[KEEPKEY_DISPLAY_HEIGHT * KEEPKEY_DISPLAY_WIDTH], int row, int column, uint8_t value) {
	buffer[row * KEEPKEY_DISPLAY_WIDTH + column] = value;
}

static int get_neighbor_count(int r, int c) {
	return !!get_pixel(canvas->buffer, modulo(r - 1, KEEPKEY_DISPLAY_HEIGHT), modulo(c - 1, KEEPKEY_DISPLAY_WIDTH)) +
	       !!get_pixel(canvas->buffer, modulo(r - 1, KEEPKEY_DISPLAY_HEIGHT), modulo(c, KEEPKEY_DISPLAY_WIDTH)) +
	       !!get_pixel(canvas->buffer, modulo(r - 1, KEEPKEY_DISPLAY_HEIGHT), modulo(c + 1, KEEPKEY_DISPLAY_WIDTH)) +
	       !!get_pixel(canvas->buffer, modulo(r, KEEPKEY_DISPLAY_HEIGHT), modulo(c - 1, KEEPKEY_DISPLAY_WIDTH)) +
	       !!get_pixel(canvas->buffer, modulo(r, KEEPKEY_DISPLAY_HEIGHT), modulo(c + 1, KEEPKEY_DISPLAY_WIDTH)) +
	       !!get_pixel(canvas->buffer, modulo(r + 1, KEEPKEY_DISPLAY_HEIGHT), modulo(c - 1, KEEPKEY_DISPLAY_WIDTH)) +
	       !!get_pixel(canvas->buffer, modulo(r + 1, KEEPKEY_DISPLAY_HEIGHT), modulo(c, KEEPKEY_DISPLAY_WIDTH)) +
	       !!get_pixel(canvas->buffer, modulo(r + 1, KEEPKEY_DISPLAY_HEIGHT), modulo(c + 1, KEEPKEY_DISPLAY_WIDTH));
}

static void animate_life(void) {
	for (int r = 0; r < KEEPKEY_DISPLAY_HEIGHT; ++r) {
		for (int c = 0; c < KEEPKEY_DISPLAY_WIDTH; ++c) {
			int count = get_neighbor_count(r, c);
			set_pixel(new_buffer, r, c, SCALE_COLOR(count == 3 || (get_pixel(canvas->buffer, r, c) && count == 2) || (count && !get_pixel(canvas->buffer, r, c) && !(random32() % 0xfff))));
		}
	}

	memcpy(canvas->buffer, new_buffer, sizeof(new_buffer));
	canvas->dirty = true;
	display_refresh();
}

static void change_speed(void *context) {
	(void)context;
	++interval;
	interval %= sizeof(intervals) / sizeof(*intervals);
}

void life(void) {
	canvas = display_canvas();
	for (int i = 0; i < KEEPKEY_DISPLAY_HEIGHT * KEEPKEY_DISPLAY_WIDTH; ++i) {
		canvas->buffer[i] = SCALE_COLOR(random32() % 2);
	}
	keepkey_button_set_on_release_handler(change_speed, NULL);
	while (1) {
		delay_ms(intervals[interval]);
		animate_life();
	}
}
