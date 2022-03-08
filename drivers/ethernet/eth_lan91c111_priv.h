#ifndef ETH_LAN91C111_PRIV_H_
#define ETH_LAN91C111_PRIV_H_

#define ETH_TX_BUF_SIZE       1536

/*
 *  Register mapping
 */
#define BANK_SEL						(DEVICE_MMIO_GET(dev) + 0x000e)
#define RCR             		(DEVICE_MMIO_GET(dev) + 0x0004)
#define TCR             		(DEVICE_MMIO_GET(dev) + 0x0000)
#define MMU_COMMAND     		(DEVICE_MMIO_GET(dev) + 0x0000)
#define MAC_ADDR0						(DEVICE_MMIO_GET(dev) + 0x0004)
#define MAC_ADDR1						(DEVICE_MMIO_GET(dev) + 0x0008)
#define MAC_ADDR2						(DEVICE_MMIO_GET(dev) + 0x000c)
#define INT_MASK						(DEVICE_MMIO_GET(dev) + 0x000d)
#define INT_REG				  		(DEVICE_MMIO_GET(dev) + 0x000c)
#define AR_REG				  		(DEVICE_MMIO_GET(dev) + 0x0003)
#define PN_REG				  		(DEVICE_MMIO_GET(dev) + 0x0002)
#define PTR_REG				  		(DEVICE_MMIO_GET(dev) + 0x0006)
#define DATA_REG				  	(DEVICE_MMIO_GET(dev) + 0x0008)

#define RCR_CLEAR           0x0000
#define RCR_ENABLE          0x0100
#define RCR_SOFT_RESET      0x8000

#define TCR_CLEAR           0x0000
#define TCR_ENABLE          0x0001

#define IMASK_RX_INTR				0x0001
#define IMASK_TX_INTR				0x0002
#define IMASK_TX_EMPTY_INTR	0x0004
#define IMASK_ALLOC_INTR		0x0008

#define MMU_COMMAND_BUSY    (1 << 0)
#define MMU_COMMAND_ALLOC   (1 << 5)
#define MMU_COMMAND_RESET   (2 << 5)
#define MMU_COMMAND_ENQUEUE (6 << 5)

#define AR_FAILED						0x80

#define PTR_AUTO_INCREMENT	0x4000

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

static inline void select_bank(const struct device *dev, uint8_t num)
{
	sys_write8(num, BANK_SEL);
	sys_write8(num >> 8, BANK_SEL + 1);
}

static inline void set_rcr(const struct device *dev, uint16_t val)
{
	select_bank(dev, 0);

	sys_write8(val, RCR);
	sys_write8(val >> 8, RCR + 1);
}

static inline void set_tcr(const struct device *dev, uint16_t val)
{
	select_bank(dev, 0);

	sys_write8(val, TCR);
	sys_write8(val >> 8, TCR + 1);
}

static inline void set_mmu_cmd(const struct device *dev, uint16_t val)
{
	select_bank(dev, 2);

	sys_write8(val, MMU_COMMAND);
	sys_write8(val >> 8, MMU_COMMAND + 1);
}

static inline uint16_t get_mmu_cmd(const struct device *dev)
{
	uint16_t retval = 0;

	select_bank(dev, 2);

	retval = sys_read8(MMU_COMMAND);
	retval |= sys_read8(MMU_COMMAND + 1) << 8;

	return retval;
}

static inline void mmu_busy_wait(const struct device *dev)
{
	uint64_t timeout = k_uptime_ticks() + 5;

	while(get_mmu_cmd(dev) & MMU_COMMAND_BUSY) {
		if (k_uptime_ticks() > timeout) {
			LOG_ERR("%s: timeout\n", __FUNCTION__);
			break;
		}
		k_cpu_idle();
	}
}

static inline void set_imask(const struct device *dev, uint8_t val)
{
	select_bank(dev, 2);

	sys_write8(val, INT_MASK);
}

static inline uint8_t get_imask(const struct device *dev)
{
	select_bank(dev, 2);

	return sys_read8(INT_MASK);
}

static inline void set_ireg(const struct device *dev, uint8_t val)
{
	select_bank(dev, 2);

	sys_write8(val, INT_REG);
}

static inline uint8_t get_ireg(const struct device *dev)
{
	select_bank(dev, 2);

	return sys_read8(INT_REG);
}

static inline uint8_t get_ar(const struct device *dev)
{
	select_bank(dev, 2);

	return sys_read8(AR_REG);
}

static inline uint8_t get_pn(const struct device *dev)
{
	select_bank(dev, 2);

	return sys_read8(PN_REG);
}

static inline void set_pn(const struct device *dev, uint8_t val)
{
	select_bank(dev, 2);

	sys_write8(val, PN_REG);
}

static inline void set_ptr(const struct device *dev, uint16_t val)
{
	select_bank(dev, 2);

	sys_write8(val, PTR_REG);
	sys_write8(val >> 8, PTR_REG + 1);
}

static inline void set_pkt_header(const struct device *dev, uint16_t status, uint16_t length)
{
	select_bank(dev, 2);

	sys_write8(status, DATA_REG);
	sys_write8(status >> 8, DATA_REG + 1);

	sys_write8(length, DATA_REG);
	sys_write8(length >> 8, DATA_REG + 1);
}

static inline void set_tx_data(const struct device *dev, uint8_t *buffer, uint16_t length)
{
	uint16_t i, data;

	select_bank(dev, 2);

	for (i = 0; i < length; i++) {
		data = buffer[i];
		sys_write8(data, DATA_REG);
		sys_write8(data >> 8, DATA_REG + 1);
	}
}

static inline void enable_interrupt(const struct device *dev, uint8_t interrupt)
{
	uint8_t mask = get_imask(dev);
	mask |= interrupt;
	set_imask(dev, mask);
}

static inline void disable_interrupt(const struct device *dev, uint8_t interrupt)
{
	uint8_t mask = get_imask(dev);
	mask &= ~interrupt;
	set_imask(dev, mask);
}

#endif /* ETH_LAN91C111_PRIV_H_ */
