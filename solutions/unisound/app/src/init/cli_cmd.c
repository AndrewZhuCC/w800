/*
 */


#include <aos/debug.h>
#include <aos/cli.h>
#include <aos/cli_cmd.h>

void board_cli_init()
{
    cli_service_init();

    extern void cli_reg_cmd_help(void);
    cli_reg_cmd_help();

    cli_reg_cmd_ping();
    //cli_reg_cmd_ifconfig();

    extern void cli_reg_cmd_ps(void);
    cli_reg_cmd_ps();

    extern void cli_reg_cmd_free(void);
    cli_reg_cmd_free();

    cli_reg_cmd_kvtool();
}
