/*
 * This file is part of DeskHop (https://github.com/hrvach/deskhop).
 * Copyright (c) 2024 Hrvoje Cavrak
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#define CURRENT_CONFIG_VERSION 5
#define CONFIG_MAGIC 0xB00B1E5

/*********  Screen  **********/
#define MIN_SCREEN_COORD 0
#define MAX_SCREEN_COORD 32767
#define SCREEN_MIDPOINT 16384

enum output_e {
    OUTPUT_A = 0,
    OUTPUT_B = 1,
    NUM_OUTPUTS
};

enum os_type_e {
    LINUX   = 1,
    MACOS   = 2,
    WINDOWS = 3,
    OTHER   = 255,
};

enum screen_pos_e {
    LEFT   = 1,
    RIGHT  = 2,
    MIDDLE = 3,
};

enum itf_num_e {
    ITF_NUM_HID       = 0,
    ITF_NUM_HID_REL_M = 1,
};

typedef struct {
    uint16_t top;    // When jumping from a smaller to a bigger screen, go to THIS top height
    uint16_t bottom; // When jumping from a smaller to a bigger screen, go to THIS bottom height
} border_size_t;

/* Define screensaver parameters */
typedef struct {
    bool enabled;
    bool only_if_inactive;
    uint64_t idle_time_us;
    uint64_t max_time_us;
} screensaver_t;

/* Define output parameters */
typedef struct {
    uint8_t number;            // Number of this output (e.g. OUTPUT_A = 0 etc)
    uint8_t screen_count;      // How many monitors per output (e.g. Output A is Windows with 3 monitors)
    uint8_t screen_index;      // Current active screen
    uint16_t speed_x;          // Mouse speed per output, in direction X
    uint16_t speed_y;          // Mouse speed per output, in direction Y
    border_size_t border;      // Screen border size/offset to keep cursor at same height when switching
    enum os_type_e os;         // Operating system on this output
    enum screen_pos_e pos;     // Screen position on this output
    screensaver_t screensaver; // Screensaver parameters for this output
} output_t;

/* Define core parameters and version */
typedef struct {
    uint32_t version;
    bool kbd_led_as_indicator;
    uint8_t hotkey_toggle;
    uint16_t jump_threshold;
    bool enable_acceleration;
    bool enforce_ports;
    output_t output[NUM_OUTPUTS];
} config_t;

/* Data structure defining how configuration is stored */
typedef struct {
    uint32_t magic_header;
    uint32_t checksum;
    config_t config;
} config_storage_t;

extern const config_t user_config;
