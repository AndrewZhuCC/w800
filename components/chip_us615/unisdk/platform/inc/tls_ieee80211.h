

#ifndef TLS_IEEE80211_H
#define TLS_IEEE80211_H

#include "tls_common.h"
#if (GCC_COMPILE==1)
#include "uni_ieee80211_gcc.h"
#else
#include "uni_ieee80211.h"
#endif
#endif /* end of TLS_IEEE80211_H */
