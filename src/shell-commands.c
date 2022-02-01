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
#include "shell-commands.h"

#include "terminal.h"
#include "lua-engine.h"
#include "cartridge.h"
#include "archive-util.h"

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>

#include <sys/types.h>
#include <dirent.h>

static char *editor_folder = NULL;

static int check_is_developer(void) {
    if(!dev_mode) {
        terminal_write(
            "Error:\n"
            "only developers can\n"
            "use this command",
            true
        );
        return -1;
    }
    return 0;
}

#define CMD(name) void name(u32 argc, char **argv)
#define CALL(command) command(argc, argv)
#define TEST(command) !strcmp(cmd, command)

static CMD(cmd_run) {
    if(argc == 0) {
        if(dev_mode) {
            game_folder = USERDATA_FOLDER;
            engine_load(false);
        } else {
            terminal_write(
                "Error:\n"
                "insert cartridge name",
                true
            );
        }
    } else {
        char *filename = malloc(PATH_MAX * sizeof(char));
        snprintf(filename, PATH_MAX, "%s.luag", argv[0]);

        game_folder = cartridge_extract(filename, NULL);
        if(game_folder) {
            engine_load(false);
        } else {
            char *error_msg = malloc(128 * sizeof(char));
            snprintf(
                error_msg, 128,
                "Error:\n"
                "'%s'\n"
                "cartridge not found",
                filename
            );

            terminal_write(error_msg, true);
            free(error_msg);
        }

        free(filename);
    }
}

static CMD(cmd_edit) {
    if(check_is_developer())
        return;

    if(!editor_folder) {
        editor_folder = malloc(PATH_MAX * sizeof(char));
        snprintf(editor_folder, PATH_MAX, "%s/editor", res_folder);
    }

    game_folder = editor_folder;
    engine_load(true);
}

static CMD(cmd_pack) {
}

static CMD(cmd_setup) {
    if(check_is_developer())
        return;

    DIR *dir = opendir(USERDATA_FOLDER);
    if(dir) {
        closedir(dir);

        terminal_write(
            "Error:\n"
            "'" USERDATA_FOLDER "'\n"
            "already exists",
            true
        );
        return;
    }

    char *template_file = malloc(PATH_MAX * sizeof(char));
    snprintf(
        template_file, PATH_MAX,
        "%s/template.luag", res_folder
    );

    int err = archiveutil_extract(template_file, USERDATA_FOLDER);
    free(template_file);

    if(err) {
        terminal_write(
            "Error:\n"
            "could not extract\n"
            "cartridge template",
            true
        );
    }
}

static CMD(cmd_cls) {
    terminal_clear();
}

static CMD(cmd_ver) {
    terminal_write(LUAG_VERSION, false);
    terminal_write(COPYRIGHT_NOTICE, false);
    terminal_write("This is Free software", false);
}

static CMD(cmd_help) {
    if(argc > 0) {

    } else {
        terminal_write("run: runs game", false);
        terminal_write("edit: opens editor", false);
        terminal_write("pack: creates cartridge", false);
        terminal_write("setup: creates game files", false);
        terminal_write("cls: clears shell", false);
        terminal_write("ver: prints version", false);
        terminal_write("help: prints this list", false);
        terminal_write("mode: changes console mode", false);
        terminal_write("files: opens game folder", false);
        terminal_write("log: opens log file", false);
    }
}

static CMD(cmd_mode) {
    if(argc == 0) {
        terminal_write("current mode:", false);
        if(dev_mode)
            terminal_write("developer", false);
        else
            terminal_write("user", false);
    } else {
        const char *mode = argv[0];
        if(!strcmp(mode, "d") || !strcmp(mode, "developer")) {
            dev_mode = true;
            terminal_write(
                "switching to\n"
                "developer mode",
                false
            );
        } else if(!strcmp(mode, "u") || !strcmp(mode, "user")) {
            dev_mode = false;
            terminal_write(
                "switching to\n"
                "user mode",
                false
            );
        } else {
            terminal_write(
                "Error\n"
                "unrecognized mode\n"
                "try 'd' or 'u'",
                true
            );
        }
    }
}

static CMD(cmd_files) {
    if(check_is_developer())
        return;

    #ifdef __unix__
        system("xdg-open " USERDATA_FOLDER);
    #elif _WIN32
        system("explorer " USERDATA_FOLDER);
    #endif
}

static CMD(cmd_log) {
}

static CMD(cmd_exit) {
    should_quit = true;
}

int commands_init(void) {
    return 0;
}

void commands_destroy(void) {
    if(editor_folder)
        free(editor_folder);
}

bool execute_command(char *cmd, u32 argc, char **argv) {
    // make cmd lowercase
    for(u32 i = 0; cmd[i] != '\0'; i++) {
        cmd[i] = tolower(cmd[i]);
    }

    if(TEST("run"))
        CALL(cmd_run);
    else if(TEST("edit") || TEST("editor"))
        CALL(cmd_edit);
    else if(TEST("pack"))
        CALL(cmd_pack);
    else if(TEST("setup"))
        CALL(cmd_setup);
    else if(TEST("cls") || TEST("clear"))
        CALL(cmd_cls);
    else if(TEST("ver") || TEST("version"))
        CALL(cmd_ver);
    else if(TEST("help"))
        CALL(cmd_help);
    else if(TEST("mode"))
        CALL(cmd_mode);
    else if(TEST("files"))
        CALL(cmd_files);
    else if(TEST("log"))
        CALL(cmd_log);
    else if(TEST("exit"))
        CALL(cmd_exit);
    else
        terminal_write("unknown command", false);

    terminal_write("", false);

    return false;
}
