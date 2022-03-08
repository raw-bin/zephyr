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

static uint8_t tx_buffer[ETH_TX_BUF_SIZE];

static void eth_lan91c111_assign_mac(const struct device *dev)
{
	uint8_t mac_addr[6] = DT_INST_PROP(0, local_mac_address);
	uint8_t test_mac_addr[6] = { 0 };

	LOG_INF("%s: %02x:%02x:%02x:%02x:%02x:%02x\n", __FUNCTION__, mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);

	select_bank(dev, 1);

	sys_write8(mac_addr[0], MAC_ADDR0);
	sys_write8(mac_addr[1], MAC_ADDR0 + 1);
	sys_write8(mac_addr[2], MAC_ADDR1);
	sys_write8(mac_addr[3], MAC_ADDR1 + 1);
	sys_write8(mac_addr[4], MAC_ADDR2);
	sys_write8(mac_addr[5], MAC_ADDR2 + 1);

	select_bank(dev, 1);

	test_mac_addr[0] = sys_read8(MAC_ADDR0);
	test_mac_addr[1] = sys_read8(MAC_ADDR0 + 1);
	test_mac_addr[2] = sys_read8(MAC_ADDR1);
	test_mac_addr[3] = sys_read8(MAC_ADDR1 + 1);
	test_mac_addr[4] = sys_read8(MAC_ADDR2);
	test_mac_addr[5] = sys_read8(MAC_ADDR2 + 1);

	LOG_INF("%s: %02x:%02x:%02x:%02x:%02x:%02x\n", __FUNCTION__, test_mac_addr[0], test_mac_addr[1], test_mac_addr[2], test_mac_addr[3], test_mac_addr[4], test_mac_addr[5]);
}

static int eth_lan91c111_send(const struct device *dev, struct net_pkt *pkt)
{
	uint16_t i, data_len, npages, pkt_num, cmd;
	uint8_t interrupt_status = 0;

	LOG_INF("%s\n", __FUNCTION__);

	data_len = net_pkt_get_len(pkt);
	npages = ((data_len & ~0x1) + (6 - 1)) >> 8;

	if (npages > 7) {
		LOG_ERR("%s: TX Packet too big", __FUNCTION__);
		return -EIO;
	}

	set_mmu_cmd(dev, npages | MMU_COMMAND_ALLOC);

	for (i = 0; i < 32; i++) {
		interrupt_status = get_ireg(dev);
		if (interrupt_status & IMASK_ALLOC_INTR) {
			set_ireg(dev, IMASK_ALLOC_INTR);
			break;
		}
	}

	if (i == 32) {
		LOG_ERR("%s: MMU allocation timeout\n", __FUNCTION__);
		return -EIO;
	}

	pkt_num = get_ar(dev);

	if (pkt_num & AR_FAILED) {
		LOG_ERR("%s: AR failed\n", __FUNCTION__);
		return -EIO;
	}

	set_pn(dev, pkt_num);
	set_ptr(dev, PTR_AUTO_INCREMENT);

	set_pkt_header(dev, 0, data_len + 6);

	if (net_pkt_read(pkt, tx_buffer, data_len)) {
		LOG_ERR("%s: net_pkt_read error\n", __FUNCTION__);
		return -ENOBUFS;
	}
	
	set_tx_data(dev, tx_buffer, data_len & ~1);
	
	if (data_len & 1) {
		cmd = tx_buffer[data_len - 1] | 0x2000;
	} else {
		cmd = 0;
	}

	sys_write8(cmd, DATA_REG);
	sys_write8(cmd >> 8, DATA_REG + 1);

	set_mmu_cmd(dev, MMU_COMMAND_ENQUEUE);
	enable_interrupt(dev, IMASK_TX_INTR | IMASK_TX_EMPTY_INTR);

	LOG_INF("pkt sent %p len %d", pkt, data_len);
	return 0;
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
	const struct device *dev = net_if_get_device(iface);
	const struct eth_lan91c111_config *dev_conf = dev->config;
	struct eth_lan91c111_runtime *dev_data = dev->data;

	LOG_INF("%s\n", __FUNCTION__);

	dev_data->iface = iface;

	/* Assign link local address. */
	net_if_set_link_addr(iface,
			     dev_data->mac_addr, 6, NET_LINK_ETHERNET);

	ethernet_init(iface);

	/* Initialize semaphore. */
	k_sem_init(&dev_data->tx_sem, 0, 1);

	/* Initialize Interrupts. */
	dev_conf->config_func(dev);
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
	const struct eth_lan91c111_config *dev_conf = dev->config;
	LOG_INF("%s\n", __FUNCTION__);

	DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);

	eth_lan91c111_assign_mac(dev);

	set_rcr(dev, RCR_SOFT_RESET);
	set_rcr(dev, RCR_CLEAR);
	set_tcr(dev, TCR_CLEAR);

	set_mmu_cmd(dev, MMU_COMMAND_RESET);
	mmu_busy_wait(dev);

	set_rcr(dev, RCR_ENABLE);
	set_rcr(dev, TCR_ENABLE);

	set_imask(dev, IMASK_RX_INTR);
	set_imask(dev, IMASK_TX_INTR);

	return 0;
}

static void eth_lan91c111_irq_config(const struct device *dev)
{
	LOG_INF("%s: irq_num: 0x%08x\n", __FUNCTION__, DT_INST_IRQN(0));

	/* Enable Interrupt. */
	IRQ_CONNECT(DT_INST_IRQN(0),
		    DT_INST_IRQ(0, priority),
		    eth_lan91c111_isr, DEVICE_DT_INST_GET(0), 0);
	irq_enable(DT_INST_IRQN(0));
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
