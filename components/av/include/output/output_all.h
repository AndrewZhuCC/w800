/*
 * Copyright (C) 2018-2020 Alibaba Group Holding Limited
 */

#ifndef __OUTPUT_ALL_H__
#define __OUTPUT_ALL_H__

#include <aos/aos.h>
#include "avutil/av_config.h"

__BEGIN_DECLS__

#define REGISTER_OUTPUTER(X, x)                                          \
    {                                                                    \
        extern int ao_register_##x();                                    \
        if (CONFIG_OUTPUTER_##X)                                         \
            ao_register_##x();                                           \
    }

/**
 * @brief  regist audio output for alsa
 * @return 0/-1
 */
int ao_register_alsa();

/**
 * @brief  regist all output
 * @return 0/-1
 */
static inline int ao_register_all()
{
#if defined(CONFIG_OUTPUTER_ALSA)
    REGISTER_OUTPUTER(ALSA, alsa);
#endif

    return 0;
}

__END_DECLS__

#endif /* __OUTPUT_ALL_H__ */

