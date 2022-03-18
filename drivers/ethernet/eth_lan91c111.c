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

	select_bank(dev, 1);

	sys_write8(mac_addr[0], MAC_ADDR0);
	sys_write8(mac_addr[1], MAC_ADDR0 + 1);
	sys_write8(mac_addr[2], MAC_ADDR1);
	sys_write8(mac_addr[3], MAC_ADDR1 + 1);
	sys_write8(mac_addr[4], MAC_ADDR2);
	sys_write8(mac_addr[5], MAC_ADDR2 + 1);
}

static int eth_lan91c111_send(const struct device *dev, struct net_pkt *pkt)
{
	struct eth_lan91c111_runtime *dev_data = dev->data;
	uint16_t i, data_len, npages, pkt_num, cmd;
	uint8_t interrupt_status = 0, buffer;

	data_len = net_pkt_get_len(pkt);

	npages = ((data_len & ~0x1) + (6 - 1)) >> 8;
	if (npages > 7) {
		LOG_ERR("%s: TX Packet too big", __FUNCTION__);
		return -EIO;
	}

	set_mmu_cmd(dev, npages | MMU_COMMAND_ALLOC);

	for (i = 0; i < 32; i++) {
		interrupt_status = get_int_reg(dev);
		if (interrupt_status & IMASK_ALLOC_INTR) {
			set_int_reg(dev, IMASK_ALLOC_INTR);
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

	set_page_number(dev, pkt_num);
	set_ptr(dev, PTR_AUTO_INCREMENT);
	set_pkt_header(dev, 0, data_len + 6);

	select_bank(dev, 2);

	for (i = 0; i < (data_len & ~1); i++) {
		if (net_pkt_read_u8(pkt, &buffer)) {
			LOG_ERR("%s: net_pkt_read error\n", __FUNCTION__);
			return -ENOBUFS;
		} else {
			sys_write8(buffer, DATA_REG);
		}
	}
	
	if (data_len & 1) {
		if (net_pkt_read_u8(pkt, &buffer)) {
			LOG_ERR("%s: net_pkt_read error\n", __FUNCTION__);
			return -ENOBUFS;
		} else {
			cmd = buffer | 0x2000;
		} 
	} else {
		cmd = 0;
	}

	sys_write8(cmd & 0xff, DATA_REG);
	sys_write8(cmd >> 8, DATA_REG + 1);

	set_mmu_cmd(dev, MMU_COMMAND_ENQUEUE);
	set_int_mask(dev, IMASK_TX_EMPTY_INTR);

	/* Wait and check if transmit successful or not. */
	k_sem_take(&dev_data->tx_sem, K_FOREVER);

	return 0;
}

static void eth_lan91c111_rx_error(struct net_if *iface)
{
	eth_stats_update_errors_rx(iface);
}

static struct net_pkt *eth_lan91c111_rx_pkt(const struct device *dev,
					    struct net_if *iface)
{
	struct net_pkt *pkt;
	unsigned int data_len, i;
	uint16_t status = 0, frame_len = 0;
	uint8_t packet_status, data;

	packet_status = get_rx_fifo_status(dev);
	if (packet_status & RX_FIFO_EMPTY) {
		LOG_ERR("%s: No packet in RX FIFO\n", __FUNCTION__);
	}

	set_ptr(dev, PTR_READ | PTR_RECEIVE | PTR_AUTO_INCREMENT);
	get_pkt_header(dev, &status, &frame_len);

	frame_len &= 0x07ff;
	data_len = frame_len - 6;

	pkt = net_pkt_rx_alloc_with_buffer(iface, frame_len, AF_UNSPEC, 0, K_NO_WAIT);
	if (!pkt) {
		return NULL;
	}

	select_bank(dev, 2);

	for (i = 0; i < data_len; i++) {
		data = sys_read8(DATA_REG);
		if (net_pkt_write_u8(pkt, data)) {
			goto error;
		}
	}

	mmu_busy_wait(dev);
	set_mmu_cmd(dev, MMU_COMMAND_RELEASE);

	return pkt;

error:
	net_pkt_unref(pkt);
	return NULL;
}

static void eth_lan91c111_rx(const struct device *dev)
{
	struct eth_lan91c111_runtime *dev_data = dev->data;
	struct net_if *iface = dev_data->iface;
	struct net_pkt *pkt;

	pkt = eth_lan91c111_rx_pkt(dev, iface);
	if (!pkt) {
		LOG_ERR("Failed to read data");
		goto err_mem;
	}

	if (net_recv_data(iface, pkt) < 0) {
		LOG_ERR("Failed to place frame in RX Queue");
		goto pkt_unref;
	}

	return;

pkt_unref:
	net_pkt_unref(pkt);

err_mem:
	eth_lan91c111_rx_error(iface);
}

static void eth_lan91c111_tx_done(const struct device *dev)
{
	unsigned int packet_status, packet_num;

	packet_num = get_page_number(dev);

	packet_status = get_tx_fifo_status(dev);
	if (packet_status & TX_FIFO_EMPTY) {
		LOG_ERR("%s: No packet in TX FIFO\n", __FUNCTION__);
	}

	set_page_number(dev, packet_status);
	mmu_busy_wait(dev);
	set_mmu_cmd(dev, MMU_COMMAND_FREE);

	mmu_busy_wait(dev);
	set_page_number(dev, packet_num);
}

static void eth_lan91c111_isr(const struct device *dev)
{
	struct eth_lan91c111_runtime *dev_data = dev->data;
	uint32_t lock, count = 0;
	uint16_t ptr;
	uint8_t pending_interrupts = 0, interrupt_mask;

	lock = irq_lock();

	ptr = get_ptr(dev);
	interrupt_mask = get_int_mask(dev);
	
	set_int_mask(dev, 0);

	for (count = 0; count < 16; count++) {
		pending_interrupts = get_int_reg(dev) & interrupt_mask;
		if (!pending_interrupts)
			break;

		if (pending_interrupts & IMASK_TX_INTR) {
			set_int_reg(dev, IMASK_TX_INTR);
			eth_lan91c111_tx_done(dev);
		}

		if (pending_interrupts & IMASK_TX_EMPTY_INTR) {
			set_int_reg(dev, IMASK_TX_EMPTY_INTR);
			k_sem_give(&dev_data->tx_sem);
			interrupt_mask &= ~IMASK_TX_EMPTY_INTR;
		} 

		if (pending_interrupts & IMASK_RX_INTR) {
			set_int_reg(dev, IMASK_RX_INTR);
			eth_lan91c111_rx(dev);
		} 
	}

	set_ptr(dev, ptr);
	set_int_mask_raw(dev, interrupt_mask);

	irq_unlock(lock);
}

static void eth_lan91c111_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	const struct eth_lan91c111_config *dev_conf = dev->config;
	struct eth_lan91c111_runtime *dev_data = dev->data;

	dev_data->iface = iface;
	net_if_set_link_addr(iface, dev_data->mac_addr, 6, NET_LINK_ETHERNET);

	ethernet_init(iface);

	k_sem_init(&dev_data->tx_sem, 0, 1);

	dev_conf->config_func(dev);
}

#if defined(CONFIG_NET_STATISTICS_ETHERNET)
static struct net_stats_eth *eth_lan91c111_stats(const struct device *dev)
{
	struct eth_lan91c111_runtime *dev_data = dev->data;

	return &dev_data->stats;
}
#endif

static int eth_lan91c111_dev_init(const struct device *dev)
{
	DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);

	eth_lan91c111_assign_mac(dev);

	set_rcr(dev, RCR_SOFT_RESET);
	set_rcr(dev, RCR_CLEAR);
	set_tcr(dev, TCR_CLEAR);

	set_mmu_cmd(dev, MMU_COMMAND_RESET);
	mmu_busy_wait(dev);

	set_rcr(dev, RCR_ENABLE);
	set_tcr(dev, TCR_ENABLE);

	set_int_mask_raw(dev, IMASK_RX_INTR | IMASK_TX_INTR);

	return 0;
}

static void eth_lan91c111_irq_config(const struct device *dev)
{
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
