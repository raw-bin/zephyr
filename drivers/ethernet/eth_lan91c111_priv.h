#ifndef ETH_LAN91C111_PRIV_H_
#define ETH_LAN91C111_PRIV_H_

#define REG_BASE(dev) \
	((const struct eth_lan91c111_config *const)(dev)->config)->mac_base
/*
 *  Register mapping
 */

struct eth_lan91c111_runtime {
	struct net_if *iface;
	uint8_t mac_addr[6];
	struct k_sem tx_sem;
	bool tx_err;
#if defined(CONFIG_NET_STATISTICS_ETHERNET)
	struct net_stats_eth stats;
#endif
};

typedef void (*eth_lan91c111_config_irq_t)(const struct device *dev);

struct eth_lan91c111_config {
	uint32_t mac_base;
	uint32_t irq_num;
	eth_lan91c111_config_irq_t config_func;
};

#endif /* ETH_LAN91C111_PRIV_H_ */
