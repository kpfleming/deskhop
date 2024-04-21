#include "config.h"
#include "user_config.h"

/* User configuration */
const config_t user_config = {
    .version = CURRENT_CONFIG_VERSION,
    .kbd_led_as_indicator = KBD_LED_AS_INDICATOR,
    .hotkey_toggle = HOTKEY_TOGGLE,
    .jump_threshold = JUMP_THRESHOLD,
    .enable_acceleration = ENABLE_ACCELERATION,
    .enforce_ports = ENFORCE_PORTS,
    .output[OUTPUT_A] =
    {
	.number = OUTPUT_A,
	.speed_x = MOUSE_SPEED_A_FACTOR_X,
	.speed_y = MOUSE_SPEED_A_FACTOR_Y,
	.border = {
	    .top = 0,
	    .bottom = MAX_SCREEN_COORD,
	},
	.screen_count = 1,
	.screen_index = 1,
	.os = OUTPUT_A_OS,
	.pos = RIGHT,
	.screensaver = {
	    .enabled = SCREENSAVER_A_ENABLED,
	    .only_if_inactive = SCREENSAVER_A_ONLY_IF_INACTIVE,
	    .idle_time_us = SCREENSAVER_A_IDLE_TIME_SEC * 1000000,
	    .max_time_us = SCREENSAVER_A_MAX_TIME_SEC * 1000000,
	}
    },
    .output[OUTPUT_B] =
    {
	.number = OUTPUT_B,
	.speed_x = MOUSE_SPEED_B_FACTOR_X,
	.speed_y = MOUSE_SPEED_B_FACTOR_Y,
	.border = {
	    .top = 0,
	    .bottom = MAX_SCREEN_COORD,
	},
	.screen_count = 1,
	.screen_index = 1,
	.os = OUTPUT_B_OS,
	.pos = LEFT,
	.screensaver = {
	    .enabled = SCREENSAVER_B_ENABLED,
	    .only_if_inactive = SCREENSAVER_B_ONLY_IF_INACTIVE,
	    .idle_time_us = SCREENSAVER_B_IDLE_TIME_SEC * 1000000,
	    .max_time_us = SCREENSAVER_B_MAX_TIME_SEC * 1000000,
	}
    },
};
