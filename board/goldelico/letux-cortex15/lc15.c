/*
 * basically the same as the OMAP5432EVM
 */

/* move away definition by included file */
#define board_mmc_init board_mmc_init_inherited
#define misc_init_r misc_init_r_inherited
#if !defined(sysinfo)
#define sysinfo sysinfo_disabled

#include "../../ti/omap5_uevm/evm.c"

#undef sysinfo

const struct omap_sysinfo sysinfo = {
	"Board: Letux Cortex 15\n"
};

#else

#include "../../ti/omap5_uevm/evm.c"

#endif
#undef board_mmc_init
#undef misc_init_r

#include <asm/emif.h>

/* careful with static variables:
 * zero vars go to DRAM, which may be not initialized yet in MLO! */
static int vers = -1;
static int ram = -1;

#ifdef CONFIG_DUAL_RANK_DDR3

/* also known as "4GB RAM CPU board"
 *
 * We should try to auto-detect the RAM size and
 * choose 2GB or 4GB automatically and getrid of CONFIG_DUAL_RANK_DDR3
 *
 * NOTE: please run a kernel with LPAE enabled to make use of the 4GB RAM!
 */

const struct emif_regs emif_regs_ddr3_532_mhz_2cs_es2 = {
	.sdram_config_init              = 0x61951BBA,
	.sdram_config                   = 0x61951BBA,
	.sdram_config2                  = 0x0,
	.ref_ctrl                       = 0x000040F1,
	.ref_ctrl_final                 = 0x00001035,
	.sdram_tim1                     = 0xCCCF36B3,
	.sdram_tim2                     = 0x308F7FDA,
	.sdram_tim3                     = 0x027F8FF8,
	.read_idle_ctrl                 = 0x00050000,
	.zq_config                      = 0xD007190B,
	.temp_alert_config              = 0x00000000,
	.emif_ddr_phy_ctlr_1_init       = 0x0020400B,
	.emif_ddr_phy_ctlr_1            = 0x0024400B,
	.emif_ddr_ext_phy_ctrl_1        = 0x04040100,
	.emif_ddr_ext_phy_ctrl_2        = 0x00000000,
	.emif_ddr_ext_phy_ctrl_3        = 0x00000000,
	.emif_ddr_ext_phy_ctrl_4        = 0x00000000,
	.emif_ddr_ext_phy_ctrl_5        = 0x4350D435,
	.emif_rd_wr_lvl_rmp_win         = 0x00000000,
	.emif_rd_wr_lvl_rmp_ctl         = 0x80000000,
	.emif_rd_wr_lvl_ctl             = 0x00000000,
	.emif_rd_wr_exec_thresh         = 0x40000305
};

static int get_board_version(void);

void emif_get_reg_dump(u32 emif_nr, const struct emif_regs **regs)
{
	get_board_version();
	if (ram != 4)
		printf("4GB RAM board encoding expected (%d)\n", ram);
	// if ram == 2 use different setup
	*regs = &emif_regs_ddr3_532_mhz_2cs_es2;
}

void dram_init_banksize(void)
{ /* make U-Boot pass the same as bootarg "mem=2032M@0x80000000 mem=2048M@0x200000000" would do */
	gd->bd->bi_dram[0].start = CONFIG_SYS_SDRAM_BASE;	// == 0x80000000
	gd->bd->bi_dram[0].size = 2032*(1024UL*1024);		// first 2GB
	gd->bd->bi_dram[1].start = 0x200000000UL;
	gd->bd->bi_dram[1].size = 2048*(1024UL*1024);		// second 2GB
#if 0	// get back "stolen" memory
	gd->bd->bi_dram[1].start = 0x2FF000000UL;
	gd->bd->bi_dram[1].size = 2064*(1024UL*1024);
#endif
	/* report 4 GB during boot */
	gd->ram_size = gd->bd->bi_dram[0].size + gd->bd->bi_dram[1].size;
}
#endif

/*
 * Board Revision Detection
 *
 * gpio2_32, gpio2_33 and gpio_34 can optionally be pulled up or down
 * by 10k resistors. These are stronger than the internal
 * pull-up or pull-down resistors of the omap5 pads.
 * by trying to pull them up/down and check who wins, we
 * can find out which resistors are installed.
 * If no resistor is installed, the gpio value follows the
 * omap5 pull-up or -down.
 * Which resistors are installed changes from board revision
 * to board revision (see schematics).
 * gpio2_33 is used for RAM size detection and
 * gpio2_32 & gpio_34 for board revision
 */

static const struct pad_conf_entry padconf_version_pd_lc15[] = {
	{LLIB_WAKEREQOUT, (IEN | PTD | M6)}, /* gpio 2_32 */
	{C2C_CLKOUT0, (IEN | PTD | M6)}, /* gpio 2_33 */
	{C2C_CLKOUT1, (IEN | PTD | M6)}, /* gpio 2_34 */
};

static const struct pad_conf_entry padconf_version_pu_lc15[] = {
	{LLIB_WAKEREQOUT, (IEN | PTU | M6)}, /* gpio 2_32 */
	{C2C_CLKOUT0, (IEN | PTU | M6)}, /* gpio 2_33 */
	{C2C_CLKOUT1, (IEN | PTU | M6)}, /* gpio 2_34 */
};

/* operational mode */
static const struct pad_conf_entry padconf_version_operation_lc15[] = {
	{LLIB_WAKEREQOUT, (IEN | M6)}, /* gpio 2_32 */
	{C2C_CLKOUT0, (IEN | M6)}, /* gpio 2_33 */
	{C2C_CLKOUT1, (IEN | M6)}, /* gpio 2_34 */
};

/* there are two U and D bits for each gpio - 00=PD 10=FLT 11=PU */
#define PULLEDUP 3
#define PULLEDDOWN 0
#define FLOATING 2

/* bit 1/0 = gpio2_32, bit 3/2 = gpio2_34 */

static const int versions[]={
#if 0	// deprecated version codes
	[(FLOATING << 0) + (FLOATING << 2)] = 49,
	[(PULLEDUP << 0) + (FLOATING << 2)] = 50,
#endif
	[(PULLEDDOWN << 0) + (FLOATING << 2)] = 51,	// v5.1.2
	[(PULLEDUP << 0) + (FLOATING << 2)] = 52,	// v5.1.3
	[(FLOATING << 0) + (FLOATING << 2)] = 53,	// V5.3
	[(FLOATING << 0) + (PULLEDDOWN << 2)] = 54,
	[(FLOATING << 0) + (PULLEDUP << 2)] = 55,
	/* others tbd. */
};

/* bit 1/0 = gpio2_33 */

static const int rams[]={
	[FLOATING] = 2,
	[PULLEDDOWN] = 4,
	[PULLEDUP] = 8,
};

static int get_board_version(void)
{ /* read get board version from resistors */
	if (vers == -1) {
		gpio_request(32, "R2701/2");	/* version resistors */
		gpio_request(33, "R2703/4");
		gpio_request(34, "R27tbd");
		gpio_direction_input(32);
		gpio_direction_input(33);
		gpio_direction_input(34);

		/* pull all down */
		do_set_mux((*ctrl)->control_padconf_core_base,
			   padconf_version_pd_lc15,
			   sizeof(padconf_version_pd_lc15) /
			   sizeof(struct pad_conf_entry));
		ram = gpio_get_value(33) << 0;
		vers = (gpio_get_value(32) << 0) | (gpio_get_value(34) << 2);

		/* pull all up */
		do_set_mux((*ctrl)->control_padconf_core_base,
			   padconf_version_pu_lc15,
			   sizeof(padconf_version_pu_lc15) /
			   sizeof(struct pad_conf_entry));
		ram |= (gpio_get_value(33) << 1);
		vers |= (gpio_get_value(32) << 1) | (gpio_get_value(34) << 3);

		gpio_free(32);
		gpio_free(33);
		gpio_free(34);

		/* safe mode */
		do_set_mux((*ctrl)->control_padconf_core_base,
			   padconf_version_operation_lc15,
			   sizeof(padconf_version_operation_lc15) /
			   sizeof(struct pad_conf_entry));
#if 1
		printf("ram code 0x%01x\n", ram);
		printf("version code 0x%01x\n", vers);
#endif
		vers = versions[vers&0xf];
		ram = rams[ram&3];
		printf("Found LC15 V%d.%d with %dGB RAM\n", vers/10, vers%10, ram);
	}
	return vers;
}

/* U-Boot only code */
#if !defined(CONFIG_SPL_BUILD)

static void set_fdtfile(void)
{
	char devtree[256];
	int vers = get_board_version();

	if (!vers)
		return;	/* can't handle */

	sprintf(devtree, "omap5-letux-cortex15-v%d.%d.dtb", vers/10, vers%10);
#if defined(CONFIG_MORE_FDT)
	{
	void CONFIG_MORE_FDT(char *devtree);
	CONFIG_MORE_FDT(devtree);	/* for main board detection */
	}
#endif
	setenv("fdtfile", devtree);

	printf("Device Tree: %s\n", devtree);
}

int misc_init_r(void)
{
	int r = misc_init_r_inherited();	/* from evm.c */
	if (!r)
		{
#if defined(CONFIG_MORE_MISC_INIT_R)
		void CONFIG_MORE_MISC_INIT_R(void);
		CONFIG_MORE_MISC_INIT_R();	/* for main board detection */
#endif
		set_fdtfile();
		if (gd->ram_size > 2*1024UL*1024UL*1024UL)
			setenv("kernelsuffix", ".lpae");
		}
	return r;
}

#endif

/*
 * the Letux Cortex 15
 * supports 3 MMC interfaces
 * and we have an mmc switch for 4-bit µSD and 8 bit eMMC
 */

#include <mmc.h>

/*
 * add eMMC/uSD switch logic here
 * so that we can boot from the internal uSD
 * and release/press the boot button without
 * throwing the eMMC/uSD switch.
 */

#if defined(CONFIG_SPL_BUILD)

const struct pad_conf_entry wkupconf_mmcmux_lc15[] = {
	{DRM_EMU1, (IEN | PTU | M6)}, /* gpio 1_wk7 */
};

const struct pad_conf_entry padconf_mmcmux_lc15[] = {
	{HSI2_ACFLAG, (IEN | M6)}, /* gpio 3_82 */
	{HSI2_CAREADY, (IEN | M6)}, /* gpio 3_76 */
};

/* FIXME: board revision 5.0 uses different gpios */

const struct pad_conf_entry padconf_mmcmux_lc15_50[] = {
//	{HSI2_ACFLAG, (IEN | M6)}, /* gpio 3_32 */
//	{HSI2_CAREADY, (IEN | M6)}, /* gpio 3_33 */
};

#endif

static void simple_ram_test(void)
{
	static int v[2];
	v[0] = 0x12345678;
	dsb();
	v[1] = 0;
	dsb();
	if (v[0] != 0x12345678) {
		printf("FATAL: DDR RAM is not working! (%lx: %x)\n", (unsigned long) v, v[0]);
		twl603x_poweroff(false);
	}
}

#define INT1_MASK	0x11
#define INT1_VBAT_MON	(1u << 7)
#define INT2_MASK	0x16
#define INT2_VAC_ACOK	(1u << 7)

static void configure_palmas(void)
{
	/*
	 * mask some interrupts so the board doesn't boot when USB is connected,
	 * so that the device can be charged while off
	 * VAC_ACOK has to be masked, and for some reason VBAT_MON too
	 */
	u8 val = 0;
	int ret;

	ret  = palmas_i2c_read_u8(TWL603X_CHIP_P2, INT2_MASK, &val);
	val |= INT2_VAC_ACOK;
	if (!ret)
		ret |= palmas_i2c_write_u8(TWL603X_CHIP_P2, INT2_MASK, val);
	ret |= palmas_i2c_read_u8(TWL603X_CHIP_P2, INT1_MASK, &val);
	val |= INT1_VBAT_MON;
	if (!ret)
		ret |= palmas_i2c_write_u8(TWL603X_CHIP_P2, INT1_MASK, val);
	if (ret)
		printf("WARNING: palmas irq masking failed\n");
}

int board_mmc_init(bd_t *bis)
{
	int hard_select = 7;	/* gpio to select uSD (0) or eMMC (1) by external hw signal */
	int soft_select = 82;	/* gpio to select uSD (1) or eMMC (0) */
	int control = 76;	/* control between SW (1) and HW (0) select */

	int val;	/* is the value of soft_select gpio after processing */

	simple_ram_test();
	int vers = get_board_version();
#if 1
	printf("board_mmc_init called (vers = %d)\n", vers);
#endif
	if (vers <= 49) {
		 /* has no working mmc switch! */
		soft_select = -1;
		hard_select = -1;
		control = -1;
	}

	if (vers == 50) {
		/* board revision 5.0 shares the revision gpios with mmc_switch control */
		soft_select = 32;
		control = 33;
	}

	gpio_request(hard_select, "bootsel");		/* BOOTSEL button */
	gpio_request(soft_select, "soft-select");	/* chooses uSD and not eMMC */
	gpio_request(control, "mmc-control");		/* MMC switch control */

#if defined(CONFIG_SPL_BUILD)

	/* in MLO we ask the external hard_select, copy to soft_select and change the control */
#if 1
	printf("  MLO\n");
#endif

	/* make gpio1_wk7 an input so that we can read the state of the BOOTSEL button */
	do_set_mux((*ctrl)->control_padconf_wkup_base,
		   wkupconf_mmcmux_lc15,
		   sizeof(wkupconf_mmcmux_lc15) /
		   sizeof(struct pad_conf_entry));

	/* is BOOTSEL active? Then we did boot SPL from µSD */
	gpio_direction_input(hard_select);
	val = !gpio_get_value(hard_select);	/* is inverted */

#if 1
	printk("  hard: gpio%d = %s\n", hard_select, val?"uSD":"eMMC");
#endif

	/* pass hard-select to soft-select so that the user can now
	 * release/press the button without disturbing the setting
	 */
	gpio_direction_output(soft_select, val);
	/* switch to soft control */
	gpio_direction_output(control, 1);

	/* make the control gpios active outputs */
	/* note: never make gpio3_82 an output on V5.0 boards! */
	if (vers == 50) {
		do_set_mux((*ctrl)->control_padconf_wkup_base,
			   padconf_mmcmux_lc15_50,
			   sizeof(padconf_mmcmux_lc15_50) /
			   sizeof(struct pad_conf_entry));
	} else if (vers > 50) {
		do_set_mux((*ctrl)->control_padconf_core_base,
			   padconf_mmcmux_lc15,
			   sizeof(padconf_mmcmux_lc15) /
			   sizeof(struct pad_conf_entry));
	}
#if 1
	/* read back */
	printk("  soft: gpio%d = %d (mmc1=%s)\n", soft_select, gpio_get_value(soft_select), gpio_get_value(soft_select)?"uSD":"eMMC");
	printk("  ctrl: gpio%d = %d (ctrl=%s)\n", control, gpio_get_value(control), gpio_get_value(control)?"soft":"hard");
#endif

#else	/* defined(CONFIG_SPL_BUILD) */

	/* in U-Boot we just fetch the state of the soft_select */
#if 1
	printf("  U-Boot\n");
#endif

	val = gpio_get_value(soft_select);

#if 1
	printk("  soft: gpio%d = %d (mmc1=%s)\n", soft_select, val, val?"uSD":"eMMC");
#endif

#endif	/* defined(CONFIG_SPL_BUILD) */

	gpio_free(hard_select);
	gpio_free(soft_select);
	gpio_free(control);

	writel(0x02, 0x4A009120);	/* enable MMC3 module */
	writel(0x02, 0x4A009128);	/* enable MMC4 module */

	/* MMC1 = left SD */
	omap_mmc_init(0, 0, 0, -1, -1);
	/* MMC2 = eMMC (8 bit) / uSD (4 bit) */
	/* NOTE: the mask defines which bits are removed! */
	omap_mmc_init(1, val?MMC_MODE_8BIT:0, 0, -1, -1);
#if 0
	/* MMC3 = WLAN */
	omap_mmc_init(2, MMC_MODE_8BIT|MMC_MODE_4BIT, 0, -1, -1);
#endif
	/* SDIO4 = right SD */
	omap_mmc_init(3, 0, 0, -1, -1);

#if 0
	printf("%08x: %08x\n", 0x4A009120, readl(0x4A009120));
	printf("%08x: %08x\n", 0x4A009128, readl(0x4A009128));
#endif

#if 0	// include if plugging in/out USB cables shall not wakeup Palmas
	configure_palmas();
#endif
	return 0;
}

int do_poweroff(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	twl603x_poweroff(false);
	printf ("failed to power down\n");
	return (0);
}

/* SPL only code */
#if defined(CONFIG_SPL_BUILD) && defined(CONFIG_SPL_OS_BOOT)

int spl_start_uboot(void)
{
	/* make it depend on pressing some shoulder button */
	return 1;	/* set to 0 to try Falcon boot to Linux */
}

#endif

