/* Copyright 2022 Vulcalien
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#include "luag-console.h"

#include "gameloop.h"
#include "lua-engine.h"
#include "terminal.h"
#include "shell-commands.h"
#include "input.h"
#include "display.h"
#include "sound.h"
#include "cartridge.h"
#include "map.h"

#include <stdio.h>
#include <string.h>
#include <limits.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

static int init(void);
static void destroy(void);
static int find_res_folder(char **result);

bool should_quit = false;

bool dev_mode = false;

char *res_folder = NULL;
char *game_folder = NULL;

int main(int argc, const char *argv[]) {
    int err = 0;

    err = init();
    if(err)
        goto exit;

    gameloop();

    exit:
    destroy();
    return err;
}

void tick(void) {
    input_tick();

    if(engine_running)
        engine_tick();
    else
        terminal_tick();

    if(should_quit)
        gameloop_stop();
}

void render(void) {
    if(engine_running)
        engine_render();
    else
        terminal_render();

    display_refresh();
}

static int init(void) {
    if(find_res_folder(&res_folder))
        return -1;

    input_init();

    if(display_init())
        return -2;
    if(sound_init())
        return -3;
    if(terminal_init())
        return -4;
    if(commands_init())
        return -5;
    if(cartridge_init())
        return -6;
    if(map_init())
        return -7;

    return 0;
}

static void destroy(void) {
    if(engine_running)
        engine_stop();

    sound_destroy();
    display_destroy();
    terminal_destroy();
    commands_destroy();
    cartridge_destroy();
    map_destroy();

    if(res_folder)
        free(res_folder);
}

static char *clone_str(char *src) {
    char *result = malloc((strlen(src) + 1) * sizeof(char));
    strcpy(result, src);
    return result;
}

// the pointer has to be freed
static int find_res_folder(char **result) {
    *result = NULL;

    #ifdef __unix__
        char *list[][2] = {
            { "/usr/share/luag-console" },
            { "/usr/local/share/luag-console" },
            { "%s/.local/share/luag-console" },
            { NULL }
        };

        list[2][1] = clone_str(getenv("HOME"));
    #elif _WIN32
        char *list[][2] = {
            { "%s/LuaG Console/res" },
            { "%s/LuaG Console/res" },
            { "%s/LuaG Console/res" },
            { NULL }
        };

        list[0][1] = clone_str(getenv("PROGRAMFILES"));
        list[1][1] = clone_str(getenv("PROGRAMFILES(x86)"));
        list[2][1] = clone_str(getenv("LOCALAPPDATA"));
    #endif

    char *path = malloc(PATH_MAX * sizeof(char));
    for(u32 i = 0; list[i][0] != NULL; i++) {
        snprintf(
            path, PATH_MAX,
            list[i][0], list[i][1]
        );

        struct stat st;
        if(!stat(path, &st) && S_ISDIR(st.st_mode)) {
            printf("Found resource folder: '%s'\n", path);
            *result = path;
            break;
        }
    }

    for(u32 i = 0; list[i][0] != NULL; i++) {
        if(list[i][1])
            free(list[i][1]);
    }

    if(!*result) {
        free(path);

        fputs("LuaG: resource folder not found\n", stderr);
        return -1;
    }
    return 0;
}
