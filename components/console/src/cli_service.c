/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */

#include <errno.h>
#include <string.h>
#include <aos/debug.h>
#include <aos/cli.h>
#include <yoc/init.h>
#include <devices/uart.h>

#define STR_CNF "Command not found.\n"
#define STR_WTS "Welcome to CLI...\n"
#define STR_INIT_ERR "CLI init failed...\n"
#define STR_PROMPT "> "

typedef enum {
    CLI_CMD_SUCCESS = 0u,
    CLI_CMD_FAILED,
    CLI_CMD_BUF_OUT,
    CLI_CMD_NOT_FOUND,
    CLI_CMD_NO_OUTPUT,

    CLI_CMD_HISTORY_NORMAL = 100u,
    CLI_CMD_HISTORY_LOOKING,
    CLI_CMD_HISTORY_FOUND,
} cli_stat_t;

struct cli_service {
    char           *cmd;
    uint16_t       buf_size;
    uint16_t       len;
    char           escape;
    aos_dev_t      *console_handle;
    volatile int   have_uart_event;
    aos_mutex_t    lock;
    uint8_t        task_run;

    slist_t        cmd_lists;
};

static struct cli_service cli_svr;
#define cli_svr_lock()   (aos_mutex_lock(&cli_svr.lock, AOS_WAIT_FOREVER))
#define cli_svr_unlock() (aos_mutex_unlock(&cli_svr.lock))

static int ch_parse(struct cli_service *svr, char ch)
{
    if ((ch == '\n') && svr->len == 0) {
        return -1;
    }

    if ((ch == '\r') || (ch == '\n')) {
        putchar('\r');
        putchar('\n');
        svr->cmd[svr->len] = 0;
        return 0;
    }

    if (svr->escape) {
        /* Yes, is it an <esc>[, 3 byte sequence */
        if (ch != 0x5b || svr->escape == 2) {
            /* We are finished with the escape sequence */
            svr->escape = 0;
            return -1;
        } else {
            /* The next character is the end of a 3-byte sequence.
            * NOTE:  Some of the <esc>[ sequences are longer than
            * 3-bytes, but I have not encountered any ch normal use
            * yet and, so, have not provided the decoding logic.
            */
            svr->escape = 2;
            return -1;
        }
    } else if (ch == 0x08 || ch == 0x7f || ch == 0x1b) { /* Ascii 1b:ESC 7f:DEL 08:Backspace */
        if (svr->len > 0) {
            svr->len--;
            svr->cmd[svr->len] = 0;
            putchar(0x08);
            /* Clear line from cursor right */
            putchar(0x1b); /* 0x1b:Escape */
            putchar('[');
            putchar('K');
        }

        if (ch == 0x1b) {
            /* Check for the beginning of a escape sequence */
            /* The next character is escaped, like arrow key */
            svr->escape = 1;
            return -1;
        }

        return -1;
    }

    putchar((int)ch);
    svr->cmd[svr->len++] = ch;

    /* buffer full auto return */
    if (svr->len >= svr->buf_size) {
        printf("\r\n Command length out of range, (length < %d)\r\n>", svr->buf_size);
        svr->len    = 0;
        svr->cmd[0] = 0;
    }

    return -1;
}

static uint32_t cli_parse(struct cli_service *svr)
{
    char *items[CLI_CMD_MAX_ARG_NUM + 1];

    if ((0 == strlen(svr->cmd)) || (0 == strcmp(svr->cmd, "\r\n"))) {
        return CLI_CMD_FAILED;
    }

    /* 引号中间的空格替换成回车,避免被分割 */
    char *ch = svr->cmd;
    int quo_s = 0;
    while(*ch) {
        if (*ch == '"') {
            quo_s++;
            quo_s %= 2;
            *ch = ' ';
        }
        ch++;

        if (quo_s) {
            if (*ch == ' ') {
                *ch = '\n';
            }
        }
    }

    memset(&items, 0, sizeof(items));
    int count = strsplit(items, CLI_CMD_MAX_ARG_NUM, svr->cmd, " ");

    /* 还原被替换的空格 */
    for(int i = 0; i < CLI_CMD_MAX_ARG_NUM; i++) {
        ch = items[i];
        if (ch == NULL) {
            break;
        }

        while(*ch != '\0') {
            if (*ch == '\n') {
                *ch = ' ';
            }
            ch++;
        }
    }

    cmd_list_t *node;
    cli_svr_lock();
    slist_for_each_entry(&svr->cmd_lists, node, cmd_list_t, next) {
        if (items[0] != NULL && strcmp(items[0], node->cmd->name) == 0) {
            node->cmd->function(NULL, 0, count, items);
            cli_svr_unlock();
            return 0;
        }
    }
    cli_svr_unlock();

    return CLI_CMD_NOT_FOUND;
}


static inline int is_normal_outputs(void)
{
    extern int *g_current_inputs;
    if(g_current_inputs) {
        return 0;
    }

    return 1;
}

static void _cli_task(void *arg)
{
    int ret;
    struct cli_service *srv = (struct cli_service *)arg;

    while (srv->task_run) {
        int ch = fgetc(NULL);
        if (ch > 250) continue;
        if (ch_parse(srv, (char)ch) == 0) {
            ret         = cli_parse(srv);
            if (ret == CLI_CMD_NOT_FOUND) {
                printf("%s:"STR_CNF, srv->cmd);
            }
            srv->len    = 0;
            srv->cmd[0] = 0;
            printf(STR_PROMPT);
        }
    }
}

void cli_service_init()
{
    const char *devname = console_get_devname();

    cli_svr.buf_size       = console_get_buffer_size();
    cli_svr.console_handle = uart_open(devname);
    cli_svr.len            = 0;
    cli_svr.escape         = 0;
    cli_svr.cmd            = malloc(cli_svr.buf_size + 1);
    aos_mutex_new(&cli_svr.lock);
    cli_svr.task_run = 1;

	aos_task_t cli_task;
    int ret = aos_task_new_ext(&cli_task, "uart_task", _cli_task, (void *)&cli_svr, CONFIG_CLI_TASK_STACK_SIZE, CONFIG_CLI_TASK_PRIO);
    // int ret = 0;
    if (ret < 0) {
        printf(STR_INIT_ERR);
    } else {
        printf(STR_WTS);
        printf(STR_PROMPT);        
    }
}

int cli_service_stop(void)
{
    return 0;
}

int cli_service_reg_cmd(const struct cli_command *info)
{
    if (NULL == info || info->name == NULL || info->function == NULL) {
        return -EINVAL;
    }
    cli_svr_lock();
    cmd_list_t *temp_cmd_node;
    slist_for_each_entry(&cli_svr.cmd_lists, temp_cmd_node, cmd_list_t, next) {
        if (strcmp(temp_cmd_node->cmd->name, info->name) == 0) {
            goto err;
        }
    }

    cmd_list_t *cmd_node = (cmd_list_t *)aos_malloc(sizeof(cmd_list_t));

    if (cmd_node == NULL) {
        goto err;
    }

    cmd_node->cmd = info;
    slist_add_tail(&cmd_node->next, &cli_svr.cmd_lists);

    cli_svr_unlock();
    return 0;
err:
    cli_svr_unlock();
    return -1;
}

int cli_service_unreg_cmd(const struct cli_command *info)
{
    if (NULL == info) {
        return -EINVAL;
    }

    if (info->name && info->function) {
        cmd_list_t *cmd_node;

        cli_svr_lock();
        slist_for_each_entry(&cli_svr.cmd_lists, cmd_node, cmd_list_t, next) {
            if (cmd_node->cmd == info) {
                slist_del(&cmd_node->next, &cli_svr.cmd_lists);
                aos_free(cmd_node);
                break;
            }
        }
        cli_svr_unlock();

        return 0;
    } else {
        return -EINVAL;
    }

    return -1;
}

int cli_service_reg_cmds(const struct cli_command commands[], int num_commands)
{
    int i, ret;
    aos_check_return_einval(commands);

    for (i = 0; i < num_commands; i++) {
        ret = cli_service_reg_cmd(&commands[i]);
        if (0 != ret) {
            return ret;
        }
    }

    return 0;
}

int cli_service_unreg_cmds(const struct cli_command commands[], int num_commands)
{
    int i, ret;

    aos_check_return_einval(commands);

    for (i = 0; i < num_commands; i++) {
        ret = cli_service_unreg_cmd(&commands[i]);
        if (0 != ret) {
            return ret;
        }
    }

    return 0;
}

slist_t cli_service_get_cmd_list(void)
{
    return cli_svr.cmd_lists;
}
