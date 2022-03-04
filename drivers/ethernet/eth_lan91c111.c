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
	LOG_INF("%s\n", __FUNCTION__);
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

static int eth_lan91c111_dev_init(const struct device *dev)
{
	LOG_INF("%s\n", __FUNCTION__);
	return 0;
}

static void eth_lan91c111_irq_config(const struct device *dev)
{
	LOG_INF("%s\n", __FUNCTION__);
}

struct eth_lan91c111_config eth_cfg = {
	.mac_base = DT_INST_REG_ADDR(0),
	.config_func = eth_lan91c111_irq_config,
};

struct eth_lan91c111_runtime eth_data = {
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
