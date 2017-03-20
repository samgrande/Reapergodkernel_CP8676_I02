
/* PIN CONFIGURE  add by lucky.wang@yulong.com 2015-03-31*/
#define FPC1020_IRQ_GPIO            11
#define FPC1020_INT_IRQNO           FPC1020_IRQ_GPIO

#define GPIO_FPC_SPI_CS             65
#define GPIO_FPC_SPI_CLK            66
#define GPIO_FPC_SPI_MISO           67
#define GPIO_FPC_SPI_M0SI           68
#define GPIO_FPC1020_IRQ            FPC1020_IRQ_GPIO
#define GPIO_FPC1020_RESET          59
#define GPIO_FPC1020_ID             12

/* SPI Board Info config add by lucky.wang@yulong.com 2015-03-31*/
static struct spi_board_info fpc1020_board_devs[] __initdata = {
	[0] = {
        .modalias="fpc1020",
		.bus_num = 0,
		.chip_select=0,
		.mode = SPI_MODE_0,
	},
};

struct mt_chip_conf spi_conf_mt65xx = {
	.setuptime = 15,
	.holdtime = 15,
	.high_time = 12, 
	.low_time = 12,
	.cs_idletime = 20,
	.ulthgh_thrsh = 0,

	.cpol = 0,
	.cpha = 0,

	.rx_mlsb = 1,
	.tx_mlsb = 1,

	.tx_endian = 0,
	.rx_endian = 0,

	.com_mod = FIFO_TRANSFER,
	.pause = 0,
	.finish_intr = 1,
	.deassert = 0,
	.ulthigh = 0,
	.tckdly = 0,
};


