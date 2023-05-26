#include "app_main.h"
#include "app_sys.h"
#include "user_gpio.h"

#define TAG "at_command"
#define RECEIVE_BUF_LEN 2048

void send_at_command(char* command, uint32_t timeout) {
    int ret = user_uart_send(1, command, strlen(command));
    LOGI(TAG, "ret: %d send: %s", ret, command);

    char recv_buf[RECEIVE_BUF_LEN] = {0};
    ret = user_uart_recv(1, recv_buf, RECEIVE_BUF_LEN, timeout);
    if (ret == -1) {
        LOGE(TAG, "uart recv error");
        return;
    }

    LOGI(TAG, "ret: %d receive: %s", ret, recv_buf);
}