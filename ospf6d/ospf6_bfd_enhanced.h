// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * ospf6_bfd_enhanced.h: Enhanced OSPFv3 BFD integration utilities
 *
 * @copyright Copyright (C) 2024 FRR Enhancement Project
 */

#ifndef OSPF6_BFD_ENHANCED_H
#define OSPF6_BFD_ENHANCED_H

#include "lib/json.h"

/* Debug option for OSPFv3 BFD */
extern unsigned char conf_debug_ospf6_bfd;
#define OSPF6_DEBUG_BFD_ON() (conf_debug_ospf6_bfd = 1)
#define OSPF6_DEBUG_BFD_OFF() (conf_debug_ospf6_bfd = 0)
#define IS_OSPF6_DEBUG_BFD (conf_debug_ospf6_bfd)

/**
 * Enhanced BFD configuration validation
 */
extern bool ospf6_bfd_config_validate(struct ospf6_interface *oi);

/**
 * Enhanced logging function for BFD events
 */
extern void ospf6_bfd_log_event(struct ospf6_neighbor *on, const char *event);

/**
 * Initialize enhanced BFD functions
 */
extern void ospf6_bfd_enhanced_init(void);

#endif /* OSPF6_BFD_ENHANCED_H */