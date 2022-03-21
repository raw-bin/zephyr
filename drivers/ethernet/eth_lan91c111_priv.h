#ifndef ETH_LAN91C111_PRIV_H_
#define ETH_LAN91C111_PRIV_H_

#define ETH_TX_BUF_SIZE       1536

/*
 *  Register mapping
 */
#define BANK_SEL										(DEVICE_MMIO_GET(dev) + 0x000e)
#define RCR             						(DEVICE_MMIO_GET(dev) + 0x0004)
#define TCR             						(DEVICE_MMIO_GET(dev) + 0x0000)
#define MMU_COMMAND     						(DEVICE_MMIO_GET(dev) + 0x0000)
#define MAC_ADDR0										(DEVICE_MMIO_GET(dev) + 0x0004)
#define MAC_ADDR1										(DEVICE_MMIO_GET(dev) + 0x0006)
#define MAC_ADDR2										(DEVICE_MMIO_GET(dev) + 0x0008)
#define INT_MASK										(DEVICE_MMIO_GET(dev) + 0x000d)
#define INT_REG				  						(DEVICE_MMIO_GET(dev) + 0x000c)
#define AR_REG				  						(DEVICE_MMIO_GET(dev) + 0x0003)
#define PN_REG				  						(DEVICE_MMIO_GET(dev) + 0x0002)
#define TX_FIFO_REG				  				(DEVICE_MMIO_GET(dev) + 0x0004)
#define RX_FIFO_REG				  				(DEVICE_MMIO_GET(dev) + 0x0005)
#define PTR_REG				  						(DEVICE_MMIO_GET(dev) + 0x0006)
#define DATA_REG				  					(DEVICE_MMIO_GET(dev) + 0x0008)

#define RCR_CLEAR           				0x0000
#define RCR_ENABLE          				0x0100
#define RCR_SOFT_RESET      				0x8000

#define TCR_CLEAR           				0x0000
#define TCR_ENABLE          				0x0001

#define IMASK_RX_INTR								0x0001
#define IMASK_TX_INTR								0x0002
#define IMASK_TX_EMPTY_INTR					0x0004
#define IMASK_ALLOC_INTR						0x0008

#define MMU_COMMAND_BUSY    				(1 << 0)
#define MMU_COMMAND_NOP   					(0 << 5)
#define MMU_COMMAND_ALLOC   				(1 << 5)
#define MMU_COMMAND_RESET   				(2 << 5)
#define MMU_COMMAND_REMOVE  				(3 << 5)
#define MMU_COMMAND_RELEASE 				(4 << 5)
#define MMU_COMMAND_FREE    				(5 << 5)
#define MMU_COMMAND_ENQUEUE 				(6 << 5)
#define MMU_COMMAND_TX_FIFO_RESET 	(7 << 5)

#define AR_FAILED										0x0080

#define PTR_READ										0x2000
#define PTR_AUTO_INCREMENT					0x4000
#define PTR_RECEIVE									0x8000

#define TX_FIFO_EMPTY 							0x0080
#define RX_FIFO_EMPTY 							0x0080

struct eth_lan91c111_runtime {
	DEVICE_MMIO_RAM;
	struct net_if *iface;
	uint8_t mac_addr[6];
	struct k_sem tx_sem;
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

static inline uint16_t get_rcr(const struct device *dev)
{
	uint16_t retval = 0;

	select_bank(dev, 0);

	retval = sys_read8(RCR);
	retval |= sys_read8(RCR + 1) << 8;

	return retval;
}

static inline void set_tcr(const struct device *dev, uint16_t val)
{
	select_bank(dev, 0);

	sys_write8(val, TCR);
	sys_write8(val >> 8, TCR + 1);
}

static inline uint16_t get_tcr(const struct device *dev)
{
	uint16_t retval = 0;

	select_bank(dev, 0);

	retval = sys_read8(TCR);
	retval |= sys_read8(TCR + 1) << 8;

	return retval;
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

static inline void set_int_mask_raw(const struct device *dev, uint8_t val)
{
	select_bank(dev, 2);

	sys_write8(val, INT_MASK);
}

static inline uint8_t get_int_mask_raw(const struct device *dev)
{
	select_bank(dev, 2);

	return sys_read8(INT_MASK);
}

static inline void set_int_mask(const struct device *dev, uint8_t mask)
{
	uint8_t imask = get_int_mask_raw(dev);
	imask |= mask;
	set_int_mask_raw(dev, imask);
}

static inline uint8_t get_int_mask(const struct device *dev)
{
	return get_int_mask_raw(dev);
}

static inline void clear_int_mask(const struct device *dev, uint8_t mask)
{
	uint8_t imask = get_int_mask_raw(dev);
	imask &= ~mask;
	set_int_mask_raw(dev, imask);
}

static inline void set_int_reg_raw(const struct device *dev, uint8_t val)
{
	select_bank(dev, 2);

	sys_write8(val, INT_REG);
}

static inline uint8_t get_int_reg_raw(const struct device *dev)
{
	select_bank(dev, 2);

	return sys_read8(INT_REG);
}

static inline void set_int_reg(const struct device *dev, uint8_t mask)
{
	uint8_t reg = get_int_reg_raw(dev);
	reg |= mask;
	set_int_reg_raw(dev, reg);
}

static inline uint8_t get_int_reg(const struct device *dev)
{
	return get_int_reg_raw(dev);
}

static inline void clear_int_reg(const struct device *dev, uint8_t mask)
{
	uint8_t reg = get_int_reg(dev);
	reg &= ~mask;
	set_int_reg_raw(dev, reg);
}

static inline uint8_t get_ar(const struct device *dev)
{
	select_bank(dev, 2);

	return sys_read8(AR_REG);
}

static inline uint8_t get_tx_fifo_status(const struct device *dev)
{
	select_bank(dev, 2);

	return sys_read8(TX_FIFO_REG);
}

static inline uint8_t get_rx_fifo_status(const struct device *dev)
{
	select_bank(dev, 2);

	return sys_read8(RX_FIFO_REG);
}

static inline uint8_t get_page_number(const struct device *dev)
{
	select_bank(dev, 2);

	return sys_read8(PN_REG);
}

static inline void set_page_number(const struct device *dev, uint8_t val)
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

static inline uint16_t get_ptr(const struct device *dev)
{
	uint16_t retval = 0;

	select_bank(dev, 2);

	retval = sys_read8(PTR_REG);
	retval |= (sys_read8(PTR_REG + 1) << 8);

	return retval;	
}

static inline void set_pkt_header(const struct device *dev, uint16_t status, uint16_t length)
{
	select_bank(dev, 2);

	sys_write8(status, DATA_REG);
	sys_write8(status >> 8, DATA_REG + 1);

	sys_write8(length, DATA_REG);
	sys_write8(length >> 8, DATA_REG + 1);
}

static inline void get_pkt_header(const struct device *dev, uint16_t *status, uint16_t *length)
{
	uint16_t val = 0;

	select_bank(dev, 2);

	val = sys_read8(DATA_REG);
	val |= (sys_read8(DATA_REG + 1) << 8);
	*status = val;

	val = sys_read8(DATA_REG);
	val |= (sys_read8(DATA_REG + 1) << 8);
	*length = val;
}

static inline uint16_t get_pkt_header_status(const struct device *dev)
{
	uint16_t status;

	status = sys_read8(DATA_REG);
	status |= (sys_read8(DATA_REG + 1) << 8);

	return status;
}

static inline uint16_t get_pkt_header_length(const struct device *dev)
{
	uint16_t length;

	length = sys_read8(DATA_REG);
	length |= (sys_read8(DATA_REG + 1) << 8);

	return length;
}

#endif /* ETH_LAN91C111_PRIV_H_ */
