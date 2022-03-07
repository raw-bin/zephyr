#ifndef ETH_LAN91C111_PRIV_H_
#define ETH_LAN91C111_PRIV_H_

/*
 *  Register mapping
 */
#define REG_BANK_SEL        (DEVICE_MMIO_GET(dev) + 0x000e)
#define REG_RCR             (DEVICE_MMIO_GET(dev) + 0x0004)
#define REG_TCR             (DEVICE_MMIO_GET(dev) + 0x0000)
#define REG_MMU_COMMAND     (DEVICE_MMIO_GET(dev) + 0x0000)

#define RCR_SOFT_RESET      0x8000
#define RCR_CLEAR           0x0000
#define TCR_CLEAR           0x0000

#define MMU_COMMAND_BUSY    (1 << 0)
#define MMU_COMMAND_RESET   (2 << 5)

struct eth_lan91c111_runtime {
	DEVICE_MMIO_RAM;
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
	DEVICE_MMIO_ROM;
	eth_lan91c111_config_irq_t config_func;
};

#endif /* ETH_LAN91C111_PRIV_H_ */
