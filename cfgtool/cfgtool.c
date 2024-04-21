/*
 * This file is part of DeskHop (https://github.com/hrvach/deskhop).
 * Copyright (c) 2024 Kevin P. Fleming
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

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "checksum.h"
#include "minIni.h"

enum gnv_e {
    SUCCESS,
    FAILURE,
    NOT_PRESENT
};

static enum gnv_e get_numeric_value(const char *ini_file, const char *section_name, const char *key_name, long *result, char *error_buf, size_t error_buf_size) {
    char val_buf[80];

    if (ini_haskey(section_name, key_name, ini_file)) {
	long val = ini_getl(section_name, key_name, -1, ini_file);
	if (val == -1) {
	    if (error_buf != NULL) {
		ini_gets(section_name, key_name, NULL, val_buf, sizeof(val_buf), ini_file);
		snprintf(error_buf, error_buf_size, "invalid value for %s - %s", key_name, val_buf);
	    }
	    return FAILURE;
	};

	*result = val;
	return SUCCESS;
    }

    return NOT_PRESENT;
}

static bool import_output_border(const char *ini_file, const char *section_name, border_size_t *border, char *error_buf, size_t error_buf_size) {
    long val = 0;

    switch (get_numeric_value(ini_file, section_name, "top", &val, error_buf, error_buf_size)) {
    case FAILURE:
	return false;
    case SUCCESS:
	border->top = (uint16_t) val;
	break;
    default:
	break;
    }

    switch (get_numeric_value(ini_file, section_name, "bottom", &val, error_buf, error_buf_size)) {
    case FAILURE:
	return false;
    case SUCCESS:
	border->bottom = (uint16_t) val;
	break;
    default:
	break;
    }

    return true;
}

static bool import_output_screensaver(const char *ini_file, const char *section_name, screensaver_t *screensaver, char *error_buf, size_t error_buf_size) {
    long val = 0;

    screensaver->enabled = ini_getbool(section_name, "enabled", screensaver->enabled, ini_file);
    screensaver->only_if_inactive = ini_getbool(section_name, "only_if_inactive", screensaver->only_if_inactive, ini_file);

    switch (get_numeric_value(ini_file, section_name, "idle_time_sec", &val, error_buf, error_buf_size)) {
    case FAILURE:
	return false;
    case SUCCESS:
	screensaver->idle_time_us = (uint64_t) val * 1000000;
	break;
    default:
	break;
    }

    switch (get_numeric_value(ini_file, section_name, "max_time_sec", &val, error_buf, error_buf_size)) {
    case FAILURE:
	return false;
    case SUCCESS:
	screensaver->max_time_us = (uint64_t) val * 1000000;
	break;
    default:
	break;
    }

    return true;
}

static bool import_output(const char *ini_file, const char *section_name, output_t *output, char *error_buf, size_t error_buf_size) {
    char name_buffer[32];
    char val_buf[80];
    long val = 0;

    switch (get_numeric_value(ini_file, section_name, "screen_count", &val, error_buf, error_buf_size)) {
    case FAILURE:
	return false;
    case SUCCESS:
	output->screen_count = (uint8_t) val;
	break;
    default:
	break;
    }

    switch (get_numeric_value(ini_file, section_name, "speed_x", &val, error_buf, error_buf_size)) {
    case FAILURE:
	return false;
    case SUCCESS:
	output->speed_x = (uint16_t) val;
	break;
    default:
	break;
    }

    switch (get_numeric_value(ini_file, section_name, "speed_y", &val, error_buf, error_buf_size)) {
    case FAILURE:
	return false;
    case SUCCESS:
	output->speed_y = (uint16_t) val;
	break;
    default:
	break;
    }

    if (ini_gets(section_name, "os", "", val_buf, sizeof(val_buf), ini_file) != 0) {
	if (!strcmp("linux", val_buf)) {
	    output->os = LINUX;
	} else if (!strcmp("macos", val_buf)) {
	    output->os = MACOS;
	} else if (!strcmp("windows", val_buf)) {
	    output->os = WINDOWS;
	} else if (!strcmp("other", val_buf)) {
	    output->os = OTHER;
	} else {
	    if (error_buf != NULL) {
		snprintf(error_buf, error_buf_size, "invalid value for os - %s", val_buf);
	    }
	    return false;
	}
    }

    if (ini_gets(section_name, "position", "", val_buf, sizeof(val_buf), ini_file) != 0) {
	if (!strcmp("left", val_buf)) {
	    output->pos = LEFT;
	} else if (!strcmp("middle", val_buf)) {
	    output->pos = MIDDLE;
	} else if (!strcmp("right", val_buf)) {
	    output->pos = RIGHT;
	} else {
	    if (error_buf != NULL) {
		snprintf(error_buf, error_buf_size, "invalid value for position - %s", val_buf);
	    }
	    return false;
	}
    }

    sprintf(name_buffer, "%s.border", section_name);
    if (!import_output_border(ini_file, name_buffer, &output->border, error_buf, error_buf_size)) {
	return false;
    }

    sprintf(name_buffer, "%s.screensaver", section_name);
    if (!import_output_screensaver(ini_file, name_buffer, &output->screensaver, error_buf, error_buf_size)) {
	return false;
    }

    return true;
}

static bool config_ini_import(const char *ini_file, config_t *cfg, char *error_buf, size_t error_buf_size) {
    long val = 0;

    cfg->kbd_led_as_indicator = ini_getbool("core", "kbd_led_as_indicator", cfg->kbd_led_as_indicator, ini_file);
    cfg->enable_acceleration = ini_getbool("core", "enable_acceleration", cfg->enable_acceleration, ini_file);
    cfg->enforce_ports = ini_getbool("core", "enforce_ports", cfg->enforce_ports, ini_file);

    switch (get_numeric_value(ini_file, "core", "hotkey_toggle", &val, error_buf, error_buf_size)) {
    case FAILURE:
	return false;
    case SUCCESS:
	cfg->hotkey_toggle = (uint8_t) val;
	break;
    default:
	break;
    }

    if (!import_output(ini_file, "output_a", &cfg->output[OUTPUT_A], error_buf, error_buf_size)) {
	return false;
    }
    if (!import_output(ini_file, "output_b", &cfg->output[OUTPUT_B], error_buf, error_buf_size)) {
	return false;
    }

    return true;
}

static int import(const char *ini_file, const char *config_file) {
    config_storage_t storage = { .config = user_config };
    char error_buf[80];

    if (!config_ini_import(ini_file, &storage.config, error_buf, sizeof(error_buf))) {
	fprintf(stderr, "Import failure: %s\n", error_buf);
	return EXIT_FAILURE;
    }

    storage.magic_header = CONFIG_MAGIC;
    storage.checksum = calc_checksum((uint8_t *) &storage.config, sizeof(storage.config));

    FILE *output;
    if (!(output = fopen(config_file, "wb"))) {
	fprintf(stderr, "Config file open failure: %s\n", strerror(errno));
	return EXIT_FAILURE;
    }

    size_t wrote;
    if ((wrote = fwrite(&storage, 1, sizeof(storage), output)) != sizeof(storage)) {
	fprintf(stderr, "Config file write failed: wrote %zu of %zu bytes\n", wrote, sizeof(storage));
	return EXIT_FAILURE;
    }

    fclose(output);

    return EXIT_SUCCESS;
}

static void export_output_border(const char *ini_file, const char *section_name, const border_size_t *border) {
    ini_putl(section_name, "top", border->top, ini_file);
    ini_putl(section_name, "bottom", border->bottom, ini_file);
}

static void export_output_screensaver(const char *ini_file, const char *section_name, const screensaver_t *screensaver) {
    ini_putbool(section_name, "enabled", screensaver->enabled, ini_file);
    ini_putbool(section_name, "only_if_inactive", screensaver->only_if_inactive, ini_file);
    ini_putl(section_name, "idle_time_sec", (long) screensaver->idle_time_us / 1000000, ini_file);
    ini_putl(section_name, "max_time_sec", (long) screensaver->max_time_us / 1000000, ini_file);
}

static void export_output(const char *ini_file, const char *section_name, const output_t *output) {
    char name_buffer[32];

    ini_putl(section_name, "screen_count", output->screen_count, ini_file);
    ini_putl(section_name, "speed_x", output->speed_x, ini_file);
    ini_putl(section_name, "speed_y", output->speed_y, ini_file);

    switch (output->os) {
    case LINUX:
	ini_puts(section_name, "os", "linux", ini_file);
	break;
    case MACOS:
	ini_puts(section_name, "os", "macos", ini_file);
	break;
    case WINDOWS:
	ini_puts(section_name, "os", "windows", ini_file);
	break;
    default:
	ini_puts(section_name, "os", "other", ini_file);
	break;
    }

    switch (output->pos) {
    case LEFT:
	ini_puts(section_name, "pos", "left", ini_file);
	break;
    case MIDDLE:
	ini_puts(section_name, "pos", "middle", ini_file);
	break;
    case RIGHT:
	ini_puts(section_name, "pos", "right", ini_file);
	break;
    }

    sprintf(name_buffer, "%s.border", section_name);
    export_output_border(ini_file, name_buffer, &output->border);

    sprintf(name_buffer, "%s.screensaver", section_name);
    export_output_screensaver(ini_file, name_buffer, &output->screensaver);
}

static bool config_ini_export(const char *ini_file, const config_t *cfg) {
    ini_putbool("core", "kbd_led_as_indicator", cfg->kbd_led_as_indicator, ini_file);
    ini_putl("core", "hotkey_toggle", cfg->hotkey_toggle, ini_file);
    ini_putl("core", "jump_threshold", cfg->jump_threshold, ini_file);
    ini_putbool("core", "enable_acceleration", cfg->enable_acceleration, ini_file);
    ini_putbool("core", "enforce_ports", cfg->enforce_ports, ini_file);

    export_output(ini_file, "output_a", &cfg->output[OUTPUT_A]);
    export_output(ini_file, "output_b", &cfg->output[OUTPUT_B]);

    return true;
}

static int export(const char *ini_file, const char *config_file) {
    config_storage_t storage;

    FILE *input;
    if (!(input = fopen(config_file, "rb"))) {
	fprintf(stderr, "Config file open failure: %s\n", strerror(errno));
	return EXIT_FAILURE;
    }

    size_t read;
    if ((read = fread(&storage, 1, sizeof(storage), input)) != sizeof(storage)) {
	fprintf(stderr, "Config file read failed: read %zu of %zu bytes\n", read, sizeof(storage));
	return EXIT_FAILURE;
    }

    fclose(input);

    if (storage.magic_header != CONFIG_MAGIC) {
	fprintf(stderr, "Config file does not have proper magic header\n");
	return EXIT_FAILURE;
    }

    if (storage.config.version != CURRENT_CONFIG_VERSION) {
	fprintf(stderr, "Config file is version %u but only version %u is supported\n", storage.config.version, CURRENT_CONFIG_VERSION);
	return EXIT_FAILURE;
    }

    uint32_t checksum = calc_checksum((uint8_t *) &storage.config, sizeof(storage.config));

    if (storage.checksum != checksum) {
	fprintf(stderr, "Config file checksum is incorrect %u - %u\n", storage.checksum, checksum);
	return EXIT_FAILURE;
    }

    if (!config_ini_export(ini_file, &storage.config)) {
	fprintf(stderr, "Export failure\n");
	return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

static int usage(const char *prog) {
    fprintf(stderr, "Usage (import mode): %s import <ini file path> <config file path>\n", prog);
    fprintf(stderr, "Usage (export mode): %s export <config file path> <ini file path>\n", prog);
    return EXIT_FAILURE;
}

int main(int argc, char *argv[]) {
    if (argc < 4) {
	return usage(argv[0]);
    }

    if (!strcmp(argv[1], "import")) {
	return import(argv[2], argv[3]);
    } else if (!strcmp(argv[1], "export")) {
	return export(argv[3], argv[2]);
    } else {
	return usage(argv[0]);
    }
}
