// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * ospf6_bfd_enhanced.c: Enhanced OSPFv3 BFD integration utilities
 *
 * This file provides additional utilities and enhancements for OSPFv3-BFD integration
 * including improved debugging, statistics, and configuration validation.
 *
 * @copyright Copyright (C) 2024 FRR Enhancement Project
 */

#include <zebra.h>
#include "command.h"
#include "vty.h"
#include "json.h"
#include "lib/bfd.h"
#include "ospf6d.h"
#include "ospf6_interface.h"
#include "ospf6_neighbor.h"
#include "ospf6_bfd.h"
#include "ospf6_bfd_enhanced.h"

/* Debug variable for OSPFv3 BFD */
unsigned char conf_debug_ospf6_bfd = 0;

/**
 * Enhanced BFD statistics structure for OSPFv3 interfaces
 */
struct ospf6_bfd_stats {
	uint32_t sessions_created;
	uint32_t sessions_destroyed;
	uint32_t session_up_events;
	uint32_t session_down_events;
	uint32_t config_changes;
	time_t last_session_change;
};

/**
 * Show OSPFv3 BFD configuration summary
 */
DEFUN(show_ipv6_ospf6_bfd_summary, show_ipv6_ospf6_bfd_summary_cmd,
      "show ipv6 ospf6 bfd summary [json]",
      SHOW_STR
      IP6_STR
      OSPF6_STR
      "BFD information\n"
      "BFD configuration summary\n"
      JSON_STR)
{
	struct ospf6 *ospf6;
	struct ospf6_area *area;
	struct ospf6_interface *oi;
	struct listnode *anode, *inode;
	int bfd_enabled_interfaces = 0;
	int total_interfaces = 0;
	int total_bfd_sessions = 0;
	bool json = use_json(argc, argv);
	json_object *json_obj = NULL, *json_areas = NULL, *json_area = NULL;
	json_object *json_interfaces = NULL, *json_interface = NULL;

	ospf6 = ospf6_lookup_by_vrf_name(VRF_DEFAULT_NAME);
	if (ospf6 == NULL) {
		vty_out(vty, "OSPFv3 is not running\n");
		return CMD_SUCCESS;
	}

	if (json) {
		json_obj = json_object_new_object();
		json_areas = json_object_new_array();
		json_object_object_add(json_obj, "areas", json_areas);
	} else {
		vty_out(vty, "OSPFv3 BFD Configuration Summary\n");
		vty_out(vty, "================================\n\n");
	}

	for (ALL_LIST_ELEMENTS_RO(ospf6->area_list, anode, area)) {
		if (json) {
			json_area = json_object_new_object();
			json_interfaces = json_object_new_array();
			json_object_string_add(json_area, "areaId", area->name);
			json_object_object_add(json_area, "interfaces", json_interfaces);
		} else {
			vty_out(vty, "Area %s:\n", area->name);
		}

		for (ALL_LIST_ELEMENTS_RO(area->if_list, inode, oi)) {
			total_interfaces++;
			
			if (json) {
				json_interface = json_object_new_object();
				json_object_string_add(json_interface, "name", 
						      oi->interface->name);
				json_object_boolean_add(json_interface, "bfdEnabled",
						       oi->bfd_config.enabled);
			}

			if (oi->bfd_config.enabled) {
				bfd_enabled_interfaces++;
				
				if (json) {
					json_object_int_add(json_interface, "detectionMultiplier",
							   oi->bfd_config.detection_multiplier);
					json_object_int_add(json_interface, "minRx",
							   oi->bfd_config.min_rx);
					json_object_int_add(json_interface, "minTx",
							   oi->bfd_config.min_tx);
					if (oi->bfd_config.profile)
						json_object_string_add(json_interface, "profile",
								       oi->bfd_config.profile);
				} else {
					vty_out(vty, "  %-15s: BFD enabled", oi->interface->name);
					if (oi->bfd_config.profile)
						vty_out(vty, " (profile: %s)", oi->bfd_config.profile);
					vty_out(vty, "\n");
					vty_out(vty, "    Detection multiplier: %d, Min RX: %dms, Min TX: %dms\n",
						oi->bfd_config.detection_multiplier,
						oi->bfd_config.min_rx,
						oi->bfd_config.min_tx);
				}

				// Count active BFD sessions
				struct ospf6_neighbor *on;
				struct listnode *nnode;
				for (ALL_LIST_ELEMENTS_RO(oi->neighbor_list, nnode, on)) {
					if (on->bfd_session)
						total_bfd_sessions++;
				}
			} else {
				if (!json)
					vty_out(vty, "  %-15s: BFD disabled\n", oi->interface->name);
			}

			if (json)
				json_object_array_add(json_interfaces, json_interface);
		}

		if (json)
			json_object_array_add(json_areas, json_area);
		else
			vty_out(vty, "\n");
	}

	if (json) {
		json_object_int_add(json_obj, "totalInterfaces", total_interfaces);
		json_object_int_add(json_obj, "bfdEnabledInterfaces", bfd_enabled_interfaces);
		json_object_int_add(json_obj, "activeBfdSessions", total_bfd_sessions);
		vty_json(vty, json_obj);
	} else {
		vty_out(vty, "Summary:\n");
		vty_out(vty, "  Total interfaces: %d\n", total_interfaces);
		vty_out(vty, "  BFD enabled interfaces: %d\n", bfd_enabled_interfaces);
		vty_out(vty, "  Active BFD sessions: %d\n", total_bfd_sessions);
	}

	return CMD_SUCCESS;
}

/**
 * Show detailed BFD information for specific OSPFv3 neighbor
 */
DEFUN(show_ipv6_ospf6_neighbor_bfd, show_ipv6_ospf6_neighbor_bfd_cmd,
      "show ipv6 ospf6 neighbor A.B.C.D bfd [json]",
      SHOW_STR
      IP6_STR
      OSPF6_STR
      "Neighbor information\n"
      "Neighbor Router ID\n"
      "BFD information\n"
      JSON_STR)
{
	struct ospf6 *ospf6;
	struct ospf6_neighbor *on;
	struct in_addr router_id;
	bool json = use_json(argc, argv);
	json_object *json_obj = NULL;

	ospf6 = ospf6_lookup_by_vrf_name(VRF_DEFAULT_NAME);
	if (ospf6 == NULL) {
		vty_out(vty, "OSPFv3 is not running\n");
		return CMD_SUCCESS;
	}

	inet_aton(argv[4]->arg, &router_id);
	on = ospf6_area_neighbor_lookup(NULL, router_id.s_addr);

	if (on == NULL) {
		vty_out(vty, "No such neighbor\n");
		return CMD_SUCCESS;
	}

	if (json)
		json_obj = json_object_new_object();

	if (on->bfd_session) {
		if (json) {
			json_object_string_add(json_obj, "routerId", argv[4]->arg);
			json_object_string_add(json_obj, "interface", on->ospf6_if->interface->name);
			json_object_boolean_add(json_obj, "bfdEnabled", true);
			bfd_sess_show(vty, json_obj, on->bfd_session);
		} else {
			vty_out(vty, "Neighbor %s BFD Information:\n", argv[4]->arg);
			vty_out(vty, "  Interface: %s\n", on->ospf6_if->interface->name);
			vty_out(vty, "  BFD Status: Enabled\n");
			bfd_sess_show(vty, NULL, on->bfd_session);
		}
	} else {
		if (json) {
			json_object_string_add(json_obj, "routerId", argv[4]->arg);
			json_object_boolean_add(json_obj, "bfdEnabled", false);
		} else {
			vty_out(vty, "Neighbor %s:\n", argv[4]->arg);
			vty_out(vty, "  BFD Status: Disabled\n");
		}
	}

	if (json)
		vty_json(vty, json_obj);

	return CMD_SUCCESS;
}

/**
 * Debug command for OSPFv3 BFD events
 */
DEFUN(debug_ospf6_bfd, debug_ospf6_bfd_cmd,
      "debug ipv6 ospf6 bfd",
      DEBUG_STR
      IP6_STR
      OSPF6_STR
      "BFD information\n")
{
	OSPF6_DEBUG_BFD_ON();
	return CMD_SUCCESS;
}

DEFUN(no_debug_ospf6_bfd, no_debug_ospf6_bfd_cmd,
      "no debug ipv6 ospf6 bfd",
      NO_STR
      DEBUG_STR
      IP6_STR
      OSPF6_STR
      "BFD information\n")
{
	OSPF6_DEBUG_BFD_OFF();
	return CMD_SUCCESS;
}

/**
 * Enhanced BFD configuration validation
 */
bool ospf6_bfd_config_validate(struct ospf6_interface *oi)
{
	if (!oi->bfd_config.enabled)
		return true;

	/* Validate detection multiplier range */
	if (oi->bfd_config.detection_multiplier < 2 || 
	    oi->bfd_config.detection_multiplier > 255) {
		zlog_warn("OSPFv3 BFD: Invalid detection multiplier %d on interface %s",
			  oi->bfd_config.detection_multiplier, oi->interface->name);
		return false;
	}

	/* Validate timing parameters */
	if (oi->bfd_config.min_rx < 50 || oi->bfd_config.min_rx > 60000) {
		zlog_warn("OSPFv3 BFD: Invalid min RX interval %dms on interface %s",
			  oi->bfd_config.min_rx, oi->interface->name);
		return false;
	}

	if (oi->bfd_config.min_tx < 50 || oi->bfd_config.min_tx > 60000) {
		zlog_warn("OSPFv3 BFD: Invalid min TX interval %dms on interface %s",
			  oi->bfd_config.min_tx, oi->interface->name);
		return false;
	}

	return true;
}

/**
 * Enhanced logging function for BFD events
 */
void ospf6_bfd_log_event(struct ospf6_neighbor *on, const char *event)
{
	if (IS_OSPF6_DEBUG_BFD) {
		zlog_debug("OSPFv3 BFD [%s->%s]: %s (State: %s)",
			   on->ospf6_if->interface->name,
			   inet_ntoa(*(struct in_addr *)&on->router_id),
			   event,
			   ospf6_neighbor_state_str[on->state]);
	}
}

/**
 * Initialize enhanced BFD functions
 */
void ospf6_bfd_enhanced_init(void)
{
	/* Install enhanced show commands */
	install_element(VIEW_NODE, &show_ipv6_ospf6_bfd_summary_cmd);
	install_element(VIEW_NODE, &show_ipv6_ospf6_neighbor_bfd_cmd);

	/* Install debug commands */
	install_element(CONFIG_NODE, &debug_ospf6_bfd_cmd);
	install_element(CONFIG_NODE, &no_debug_ospf6_bfd_cmd);
	install_element(ENABLE_NODE, &debug_ospf6_bfd_cmd);
	install_element(ENABLE_NODE, &no_debug_ospf6_bfd_cmd);
}