/* Copyright 2022 Vulcalien
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3 only.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#include "shell.h"

#include <string.h>

#include "display.h"

#define COMMAND_HISTORY_SIZE (1024)
static char **command_history;
static u32 used_command_history = 0;

#define MAX_LINE_LEN (127)

static struct {
    char *text;
    u32 len;
    u32 cursor_pos;
} active_line;

static void shell_execute(void);

static void allocate_active_line(void);

int shell_init(void) {
    command_history = malloc(COMMAND_HISTORY_SIZE * sizeof(const char *));

    allocate_active_line();

    return 0;
}

void shell_tick(void) {
}

void shell_render(void) {
    display_clear(0x000000);

    display_write(active_line.text, 0xffffff, 1, 1);
}

// TODO
void shell_write(const char *text, u32 color) {
}

void shell_receive_input(const char *c) {
    // do this instead of strlen because
    // most of the times c[1] is '\0'
    for(u32 i = 0; c[i] != '\0'; i++) {
        if(active_line.len == MAX_LINE_LEN)
            return;

        if(active_line.cursor_pos == active_line.len) {
            active_line.text[active_line.cursor_pos] = c[i];
        } else {
            memmove(
                active_line.text + active_line.cursor_pos,
                active_line.text + active_line.cursor_pos + 1,
                active_line.len - active_line.cursor_pos
            );
        }
        active_line.len++;
        active_line.cursor_pos++;
    }

    luag_ask_refresh();
}

static void shell_execute(void) {
    if(used_command_history == COMMAND_HISTORY_SIZE) {
        // delete the oldest half of commands

        for(u32 i = 0; i < COMMAND_HISTORY_SIZE; i++) {
            free(command_history[i]);
        }

        // overwrite the first half
        memcpy(
            command_history,
            command_history + COMMAND_HISTORY_SIZE / 2,
            COMMAND_HISTORY_SIZE / 2
        );
        used_command_history = COMMAND_HISTORY_SIZE / 2;
    }

    command_history[used_command_history] = active_line.text;
    allocate_active_line();
}

static void allocate_active_line(void) {
    active_line.text = calloc((MAX_LINE_LEN + 1), sizeof(char));
    active_line.len = 0;
    active_line.cursor_pos = 0;
}
