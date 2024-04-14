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

#include "main.h"

/* Move mouse coordinate 'position' by 'offset', but don't fall off the screen */
int32_t move_and_keep_on_screen(int position, int offset) {
    /* Lowest we can go is 0 */
    if (position + offset < MIN_SCREEN_COORD)
        return MIN_SCREEN_COORD;

    /* Highest we can go is MAX_SCREEN_COORD */
    else if (position + offset > MAX_SCREEN_COORD)
        return MAX_SCREEN_COORD;

    /* We're still on screen, all good */
    return position + offset;
}

/* Implement basic mouse acceleration and define your own curve.
   This one is probably sub-optimal, so let me know if you have a better one. */
int32_t accelerate(device_t *state, int32_t offset) {
    const struct curve {
        int value;
        float factor;
    } acceleration[7] = {
                   // 4 |                                        *
        {2, 1},    //   |                                  *
        {5, 1.1},  // 3 |
        {15, 1.4}, //   |                       *
        {30, 1.9}, // 2 |                *
        {45, 2.6}, //   |        *
        {60, 3.4}, // 1 |  *
        {70, 4.0}, //    -------------------------------------------
    };             //        10    20    30    40    50    60    70

    if (!state->config.enable_acceleration)
        return offset;

    for (int i = 0; i < 7; i++) {
        if (abs(offset) < acceleration[i].value) {
            return offset * acceleration[i].factor;
        }
    }

    return offset * acceleration[6].factor;
}

void update_mouse_position(device_t *state, mouse_values_t *values) {
    output_t *current    = &state->config.output[state->active_output];
    uint8_t reduce_speed = 0;

    /* Check if we are configured to move slowly */
    if (state->mouse_zoom)
        reduce_speed = MOUSE_ZOOM_SCALING_FACTOR;

    /* Calculate movement */
    int offset_x = accelerate(state, values->move_x) * (current->speed_x >> reduce_speed);
    int offset_y = accelerate(state, values->move_y) * (current->speed_y >> reduce_speed);

    /* Update movement */
    state->mouse_x = move_and_keep_on_screen(state->mouse_x, offset_x);
    state->mouse_y = move_and_keep_on_screen(state->mouse_y, offset_y);

    /* Update buttons state */
    state->mouse_buttons = values->buttons;
}

/* If we are active output, queue packet to mouse queue, else send them through UART */
void output_mouse_report(mouse_report_t *report, device_t *state) {
    if (CURRENT_BOARD_IS_ACTIVE_OUTPUT) {
        queue_mouse_report(report, state);
        state->last_activity[BOARD_ROLE] = time_us_64();
    } else {
        send_packet((uint8_t *)report, MOUSE_REPORT_MSG, MOUSE_REPORT_LENGTH);
    }
}

/* Calculate and return Y coordinate when moving from screen out_from to screen out_to */
int16_t scale_y_coordinate(int screen_from, int screen_to, device_t *state) {
    output_t *from = &state->config.output[screen_from];
    output_t *to   = &state->config.output[screen_to];

    int size_to   = to->border.bottom - to->border.top;
    int size_from = from->border.bottom - from->border.top;

    /* If sizes match, there is nothing to do */
    if (size_from == size_to)
        return state->mouse_y;

    /* Moving from smaller ==> bigger screen
       y_a = top + (((bottom - top) * y_b) / HEIGHT) */

    if (size_from > size_to) {
        return to->border.top + ((size_to * state->mouse_y) / MAX_SCREEN_COORD);
    }

    /* Moving from bigger ==> smaller screen
       y_b = ((y_a - top) * HEIGHT) / (bottom - top) */

    if (state->mouse_y < from->border.top)
        return MIN_SCREEN_COORD;

    if (state->mouse_y > from->border.bottom)
        return MAX_SCREEN_COORD;

    return ((state->mouse_y - from->border.top) * MAX_SCREEN_COORD) / size_from;
}

void switch_screen(
    device_t *state, output_t *output, int new_x, int output_from, int output_to, int direction) {
    mouse_report_t hidden_pointer = {.y = MIN_SCREEN_COORD, .x = MAX_SCREEN_COORD};

    output_mouse_report(&hidden_pointer, state);
    switch_output(state, output_to);
    state->mouse_x = (direction == LEFT) ? MAX_SCREEN_COORD : MIN_SCREEN_COORD;
    state->mouse_y = scale_y_coordinate(output->number, 1 - output->number, state);
}

void switch_desktop(device_t *state, output_t *output, int new_index, int direction) {
    /* Fix for MACOS: Send relative mouse movement here, one or two pixels in the 
       direction of movement, BEFORE absolute report sets X to 0 */
    mouse_report_t move_relative_one
        = {.x = (direction == LEFT) ? SCREEN_MIDPOINT - 2 : SCREEN_MIDPOINT + 2, .mode = RELATIVE};

    switch (output->os) {
        case MACOS:
            /* Once doesn't seem reliable enough, do it twice */
            output_mouse_report(&move_relative_one, state);
            output_mouse_report(&move_relative_one, state);
            break;

        case WINDOWS:
            /* TODO: Switch to relative-only if index > 1, but keep tabs to switch back */
            state->relative_mouse = (new_index > 1);
            break;

        case LINUX:
        case OTHER:
            /* Linux should treat all desktops as a single virtual screen, so you should leave
            screen_count at 1 and it should just work */
            break;
    }

    state->mouse_x       = (direction == RIGHT) ? MIN_SCREEN_COORD : MAX_SCREEN_COORD;
    output->screen_index = new_index;
}

/*                               BORDER
                                   |
       .---------.    .---------.  |  .---------.    .---------.    .---------.
      ||    B2   ||  ||    B1   || | ||    A1   ||  ||    A2   ||  ||    A3   ||   (output, index)
      ||  extra  ||  ||   main  || | ||   main  ||  ||  extra  ||  ||  extra  ||   (main or extra)
       '---------'    '---------'  |  '---------'    '---------'    '---------'
          )___(          )___(     |     )___(          )___(          )___(
*/
void check_screen_switch(const mouse_values_t *values, device_t *state) {
    int new_x        = state->mouse_x + values->move_x;
    output_t *output = &state->config.output[state->active_output];

    bool jump_left  = new_x < MIN_SCREEN_COORD - state->config.jump_threshold;
    bool jump_right = new_x > MAX_SCREEN_COORD + state->config.jump_threshold;

    int direction = jump_left ? LEFT : RIGHT;

    /* No switching allowed if explicitly disabled or mouse button is held */
    if (state->switch_lock || state->mouse_buttons)
        return;

    /* No jump condition met == nothing to do, return */
    if (!jump_left && !jump_right)
        return;

    /* We want to jump in the direction of the other computer */
    if (output->pos != direction) {
        if (output->screen_index == 1) /* We are at the border -> switch outputs */
            switch_screen(state, output, new_x, state->active_output, 1 - state->active_output, direction);

        /* If here, this output has multiple desktops and we are not on the main one */
        else
            switch_desktop(state, output, output->screen_index - 1, direction);
    }

    /* We want to jump away from the other computer, only possible if there is another screen to jump to */
    else if (output->screen_index < output->screen_count)
        switch_desktop(state, output, output->screen_index + 1, direction);
}

void extract_report_values(uint8_t *raw_report, device_t *state, mouse_values_t *values) {
    /* Interpret values depending on the current protocol used. */
    if (state->mouse_dev.protocol == HID_PROTOCOL_BOOT) {
        hid_mouse_report_t *mouse_report = (hid_mouse_report_t *)raw_report;

        values->move_x  = mouse_report->x;
        values->move_y  = mouse_report->y;
        values->wheel   = mouse_report->wheel;
        values->buttons = mouse_report->buttons;
        return;
    }

    /* If HID Report ID is used, the report is prefixed by the report ID so we have to move by 1 byte */
    if (state->mouse_dev.uses_report_id)
        raw_report++;

    values->move_x  = get_report_value(raw_report, &state->mouse_dev.move_x);
    values->move_y  = get_report_value(raw_report, &state->mouse_dev.move_y);
    values->wheel   = get_report_value(raw_report, &state->mouse_dev.wheel);
    values->buttons = get_report_value(raw_report, &state->mouse_dev.buttons);
}

mouse_report_t create_mouse_report(device_t *state, mouse_values_t *values) {
    mouse_report_t mouse_report = {
        .buttons = values->buttons,
        .x       = state->mouse_x,
        .y       = state->mouse_y,
        .wheel   = values->wheel,
        .mode    = ABSOLUTE,
    };

    /* Workaround for Windows multiple desktops */
    if (state->relative_mouse) {
        mouse_report.x = SCREEN_MIDPOINT + values->move_x;
        mouse_report.y = SCREEN_MIDPOINT + values->move_y;
        mouse_report.mode = RELATIVE;
        mouse_report.buttons = values->buttons;
        mouse_report.wheel = values->wheel;
    }
    return mouse_report;
}

void process_mouse_report(uint8_t *raw_report, int len, device_t *state) {
    mouse_values_t values = {0};

    /* Interpret the mouse HID report, extract and save values we need. */
    extract_report_values(raw_report, state, &values);

    /* Calculate and update mouse pointer movement. */
    update_mouse_position(state, &values);

    /* Create the report for the output PC based on the updated values */
    mouse_report_t report = create_mouse_report(state, &values);

    /* Move the mouse, depending where the output is supposed to go */
    output_mouse_report(&report, state);

    /* We use the mouse to switch outputs, the logic is in check_screen_switch() */
    check_screen_switch(&values, state);
}

/* ==================================================== *
 * Mouse Queue Section
 * ==================================================== */

void process_mouse_queue_task(device_t *state) {
    mouse_report_t report = {0};

    /* We need to be connected to the host to send messages */
    if (!state->tud_connected)
        return;

    /* Peek first, if there is anything there... */
    if (!queue_try_peek(&state->mouse_queue, &report))
        return;

    /* If we are suspended, let's wake the host up */
    if (tud_suspended())
        tud_remote_wakeup();

    /* ... try sending it to the host, if it's successful */
    bool succeeded = tud_mouse_report(report.mode, report.buttons, report.x, report.y, report.wheel);

    /* ... then we can remove it from the queue */
    if (succeeded)
        queue_try_remove(&state->mouse_queue, &report);
}

void queue_mouse_report(mouse_report_t *report, device_t *state) {
    /* It wouldn't be fun to queue up a bunch of messages and then dump them all on host */
    if (!state->tud_connected)
        return;

    queue_try_add(&state->mouse_queue, report);
}
