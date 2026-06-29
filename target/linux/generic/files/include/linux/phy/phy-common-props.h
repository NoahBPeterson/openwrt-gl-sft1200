/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef __PHY_COMMON_PROPS_H
#define __PHY_COMMON_PROPS_H
#include <dt-bindings/phy/phy.h>
struct fwnode_handle;
static inline int phy_get_rx_polarity(struct fwnode_handle *f, const char *m, unsigned int s, unsigned int d, unsigned int *v){ *v=d; return 0; }
static inline int phy_get_tx_polarity(struct fwnode_handle *f, const char *m, unsigned int s, unsigned int d, unsigned int *v){ *v=d; return 0; }
static inline int phy_get_manual_rx_polarity(struct fwnode_handle *f, const char *m, unsigned int *v){ *v=PHY_POL_NORMAL; return 0; }
static inline int phy_get_manual_tx_polarity(struct fwnode_handle *f, const char *m, unsigned int *v){ *v=PHY_POL_NORMAL; return 0; }
#endif
