#include <resea/ipc.h>
#include <resea/printf.h>
#include <resea/malloc.h>
#include <resea/syscall.h>
#include <string.h>
#include "commands.h"

#define BOLD "\e[1;34m" // Bold.
#define PRINTF(fmt, ...) printf(BOLD fmt SGR_RESET, ##__VA_ARGS__)

static void skip_whitespaces(char **cmdline) {
    while (**cmdline == ' ') {
        (*cmdline)++;
    }
}

static int parse(char *cmdline, char **argv, int argv_max) {
    skip_whitespaces(&cmdline);
    if (*cmdline == '\0') {
        return 0;
    }

    int argc = 1;
    argv[0] = cmdline;
    while (*cmdline != '\0') {
        if (*cmdline == ' ') {
            *cmdline++ = '\0';
            skip_whitespaces(&cmdline);
            argv[argc] = cmdline;
            argc++;
            if (argc == argv_max - 1) {
                break;
            }
        } else {
            cmdline++;
        }
    }

    argv[argc] = NULL;
    return argc;
}

static void run(char *cmdline) {
    int argv_max = 8;
    char **argv = malloc(sizeof(char *) * argv_max);
    int argc = parse(cmdline, argv, argv_max);
    if (!argc) {
        return;
    }

    for (int i = 0; commands[i].name != NULL; i++) {
        if (!strcmp(commands[i].name, argv[0])) {
            commands[i].run(argc, argv);
            return;
        }
    }

    // The command not found, show the help message...
    WARN("Unknown shell command '%s'. Try 'help' to show usages.", argv[0]);
}

static void prompt(const char *input) {
    PRINTF("\e[1K\rshell> %s", input);
    printf_flush();
}

static void read_input(void) {
    char buf[256];
    static char cmdline[512];
    static int cmdline_index = 0;

    OOPS_OK(kdebug_with_buf("_input", buf, sizeof(buf)));
    for (size_t i = 0; i < sizeof(buf) && buf[i] != '\0'; i++) {
        PRINTF("%c", buf[i]);
        switch (buf[i]) {
            case '\n':
                cmdline[cmdline_index] = '\0';
                printf("\e[0m");
                printf_flush();
                run(cmdline);
                cmdline_index = 0;
                prompt("");
                break;
            // Backspace.
            case 0x7f:
                if (cmdline_index > 0) {
                    printf("\b");
                    cmdline_index--;
                    cmdline[cmdline_index] = '\0';
                    prompt(cmdline);
                }
                break;
            default:
                if (cmdline_index == sizeof(cmdline) - 1) {
                    WARN("too long command");
                    cmdline_index = 0;
                    prompt("");
                } else {
                    cmdline[cmdline_index] = buf[i];
                    cmdline_index++;
                }
        }
    }

    printf_flush();
}

void main(void) {
    TRACE("starting...");

    OOPS_OK(kdebug("_listeninput"));

    // The mainloop: receive and handle messages.
    prompt("");
    while (true) {
        struct message m;
        error_t err = ipc_recv(IPC_ANY, &m);
        ASSERT_OK(err);

        switch (m.type) {
            case NOTIFICATIONS_MSG:
                if (m.notifications.data & NOTIFY_IRQ) {
                    read_input();
                }
                break;
            default:
                WARN("unknown message type (type=%d)", m.type);
        }
    }
}