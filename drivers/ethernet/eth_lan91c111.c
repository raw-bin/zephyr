#define DT_DRV_COMPAT smsc_lan91c111

#define LOG_MODULE_NAME eth_lan91c111
#define LOG_LEVEL CONFIG_ETHERNET_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <net/ethernet.h>
#include <net/net_pkt.h>
#include <net/net_if.h>
#include <device.h>
#include <soc.h>
#include <ethernet/eth_stats.h>
#include "eth_lan91c111_priv.h"

static void eth_lan91c111_assign_mac(const struct device *dev)
{
	uint8_t mac_addr[6] = DT_INST_PROP(0, local_mac_address);

	LOG_INF("%s: %02x:%02x:%02x:%02x:%02x:%02x\n", __FUNCTION__, mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
}

static void eth_lan91c111_flush(const struct device *dev)
{
	LOG_INF("%s\n", __FUNCTION__);
}

static void eth_lan91c111_send_byte(const struct device *dev, uint8_t byte)
{
	LOG_INF("%s\n", __FUNCTION__);
}

static int eth_lan91c111_send(const struct device *dev, struct net_pkt *pkt)
{
	LOG_INF("%s\n", __FUNCTION__);
	return 0;
}

static void eth_lan91c111_rx_error(struct net_if *iface)
{
	LOG_INF("%s\n", __FUNCTION__);
}

static struct net_pkt *eth_lan91c111_rx_pkt(const struct device *dev,
					    struct net_if *iface)
{
	LOG_INF("%s\n", __FUNCTION__);
	return NULL;
}

static void eth_lan91c111_rx(const struct device *dev)
{
	LOG_INF("%s\n", __FUNCTION__);
}

static void eth_lan91c111_isr(const struct device *dev)
{
	LOG_INF("%s\n", __FUNCTION__);
}

static void eth_lan91c111_init(struct net_if *iface)
{
	LOG_INF("%s\n", __FUNCTION__);
}

#if defined(CONFIG_NET_STATISTICS_ETHERNET)
static struct net_stats_eth *eth_lan91c111_stats(const struct device *dev)
{
	struct eth_lan91c111_runtime *dev_data = dev->data;

	LOG_INF("%s\n", __FUNCTION__);

	return &data->stats;
}
#endif

static inline void select_bank(const struct device *dev, uint8_t num)
{
	sys_write8(num, REG_BANK_SEL);
	sys_write8(num >> 8, REG_BANK_SEL + 1);
}

static inline void set_rcr(const struct device *dev, uint16_t val)
{
	select_bank(dev, 0);

	sys_write8(val, REG_RCR);
	sys_write8(val >> 8, REG_RCR + 1);
}

static inline void set_tcr(const struct device *dev, uint16_t val)
{
	select_bank(dev, 0);

	sys_write8(val, REG_TCR);
	sys_write8(val >> 8, REG_TCR + 1);
}

static inline void set_mmu_cmd(const struct device *dev, uint16_t val)
{
	select_bank(dev, 2);

	sys_write8(val, REG_MMU_COMMAND);
	sys_write8(val >> 8, REG_MMU_COMMAND + 1);
}

static int eth_lan91c111_dev_init(const struct device *dev)
{
	const struct eth_lan91c111_config *dev_conf = dev->config;
	LOG_INF("%s\n", __FUNCTION__);

	DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);

	eth_lan91c111_assign_mac(dev);

	set_rcr(dev, RCR_SOFT_RESET);
	set_rcr(dev, RCR_CLEAR);
	set_tcr(dev, TCR_CLEAR);

	set_mmu_cmd(dev, MMU_COMMAND_RESET);

	return 0;
}

static void eth_lan91c111_irq_config(const struct device *dev)
{
	LOG_INF("%s\n", __FUNCTION__);
}

struct eth_lan91c111_config eth_cfg = {
	DEVICE_MMIO_ROM_INIT(DT_DRV_INST(0)),
	.config_func = eth_lan91c111_irq_config,
};

struct eth_lan91c111_runtime eth_data = {
	.mac_addr = DT_INST_PROP(0, local_mac_address),
	.tx_err = false,
};

static const struct ethernet_api eth_lan91c111_apis = {
	.iface_api.init	= eth_lan91c111_init,
	.send =  eth_lan91c111_send,
#if defined(CONFIG_NET_STATISTICS_ETHERNET)
	.get_stats = eth_lan91c111_stats,
#endif
};

NET_DEVICE_DT_INST_DEFINE(0,
		eth_lan91c111_dev_init, NULL,
		&eth_data, &eth_cfg, CONFIG_ETH_INIT_PRIORITY,
		&eth_lan91c111_apis, ETHERNET_L2,
		NET_L2_GET_CTX_TYPE(ETHERNET_L2), NET_ETH_MTU);
