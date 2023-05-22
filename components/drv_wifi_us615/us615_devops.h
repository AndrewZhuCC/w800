#ifndef __US615_DEVOPS_H__
#define __US615_DEVOPS_H__
#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int         smartcfg_pin;
} us615_wifi_param_t;

void wifi_us615_register(us615_wifi_param_t *config);
int wifi_is_connected_to_ap(void);

#ifdef __cplusplus
}
#endif

#endif /* __US615_DEVOPS_H__ */
