/*
 * ngene-cards.c: nGene PCIe bridge driver - card specific info
 *
 * Copyright (C) 2005-2007 Micronas
 *
 * Copyright (C) 2008-2009 Ralph Metzler <rjkm@metzlerbros.de>
 *                         Modifications for new nGene firmware,
 *                         support for EEPROM-copying,
 *                         support for new dual DVB-S2 card prototype
 *
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 only, as published by the Free Software Foundation.
 *
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA
 * Or, point your browser to http://www.gnu.org/copyleft/gpl.html
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/pci.h>
#include <linux/pci_ids.h>
#include "compat.h"

#include "ngene.h"

/* demods/tuners */
#include "stv6110x.h"
#include "stv090x.h"
#include "lnbh24.h"
#include "lgdt330x.h"
#include "mt2131.h"
#include "tda18271c2dd.h"
#include "drxk.h"
#include "tda18212dd.h"
#include "stv0367dd.h"
#include "cxd2843.h"


enum DEMOD_TYPE {
	DMD_NONE = 0,
	DMD_STV0900,
	DMD_DRXK,
	DMD_STV0367,
	DMD_CXD28xx,
};


/****************************************************************************/
/* Demod/tuner attachment ***************************************************/
/****************************************************************************/

static int tuner_attach_stv6110(struct ngene_channel *chan)
{
	struct i2c_adapter *i2c;
	struct stv090x_config *feconf = (struct stv090x_config *)
		chan->dev->card_info->fe_config[chan->number];
	struct stv6110x_config *tunerconf = (struct stv6110x_config *)
		chan->dev->card_info->tuner_config[chan->number];
	struct stv6110x_devctl *ctl;

	/* tuner 1+2: i2c adapter #0, tuner 3+4: i2c adapter #1 */
	if (chan->number < 2)
		i2c = &chan->dev->channel[0].i2c_adapter;
	else
		i2c = &chan->dev->channel[1].i2c_adapter;

	ctl = dvb_attach(stv6110x_attach, chan->fe, tunerconf, i2c);
	if (ctl == NULL) {
		printk(KERN_ERR	DEVICE_NAME ": No STV6110X found!\n");
		return -ENODEV;
	}

	feconf->tuner_init          = ctl->tuner_init;
	feconf->tuner_sleep         = ctl->tuner_sleep;
	feconf->tuner_set_mode      = ctl->tuner_set_mode;
	feconf->tuner_set_frequency = ctl->tuner_set_frequency;
	feconf->tuner_get_frequency = ctl->tuner_get_frequency;
	feconf->tuner_set_bandwidth = ctl->tuner_set_bandwidth;
	feconf->tuner_get_bandwidth = ctl->tuner_get_bandwidth;
	feconf->tuner_set_bbgain    = ctl->tuner_set_bbgain;
	feconf->tuner_get_bbgain    = ctl->tuner_get_bbgain;
	feconf->tuner_set_refclk    = ctl->tuner_set_refclk;
	feconf->tuner_get_status    = ctl->tuner_get_status;

	return 0;
}

#if 0
static int tuner_attach_mt2060(struct ngene_channel *chan)
{
	struct ngene *dev = chan->dev;
	void *tconf = dev->card_info->tuner_config[chan->number];
	u8 drxa = dev->card_info->demoda[chan->number];
	struct dvb_frontend *fe = chan->fe, *fe2;

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 19)
	fe->misc_priv = chan;
#else
	fe->sec_priv = chan;
#endif
	fe->ops.i2c_gate_ctrl = dev->card_info->gate_ctrl;

	dev->card_info->gate_ctrl(fe, 1);
	fe2 = mt2060_attach(fe, &chan->i2c_adapter, tconf, 1220);
	dev->card_info->gate_ctrl(fe, 0);

	i2c_write_register(&chan->i2c_adapter, drxa, 3, 4);
	write_demod(&chan->i2c_adapter, drxa, 0x1012, 15);
	write_demod(&chan->i2c_adapter, drxa, 0x1007, 0xc27);
	write_demod(&chan->i2c_adapter, drxa, 0x0020, 0x003);

	return fe2 ? 0 : -ENODEV;
}

static int tuner_attach_xc3028(struct ngene_channel *chan)
{
	struct ngene *dev = chan->dev;
	void *tconf = dev->card_info->tuner_config[chan->number];
	struct dvb_frontend *fe = chan->fe, *fe2;

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 19)
	fe->misc_priv = chan;
#else
	fe->sec_priv = chan;
#endif
	fe->ops.i2c_gate_ctrl = dev->card_info->gate_ctrl;

	dev->card_info->gate_ctrl(fe, 1);
	fe2 = xc3028_attach(fe, &chan->i2c_adapter, tconf);
	dev->card_info->gate_ctrl(fe, 0);

	/*chan->fe->ops.tuner_ops.set_frequency(chan->fe,231250000);*/

	return fe2 ? 0 : -ENODEV;
}

static int demod_attach_drxd(struct ngene_channel *chan)
{
	void *feconf = chan->dev->card_info->fe_config[chan->number];

	chan->fe = drxd_attach(feconf,
			       chan, &chan->i2c_adapter,
			       &chan->dev->pci_dev->dev);
	return (chan->fe) ? 0 : -ENODEV;
}

static int demod_attach_drxh(struct ngene_channel *chan)
{
	void *feconf = chan->dev->card_info->fe_config[chan->number];

	chan->fe = drxh_attach(feconf, chan,
			       &chan->i2c_adapter, &chan->dev->pci_dev->dev);
	return (chan->fe) ? 0 : -ENODEV;
}

static int demod_attach_stb0899(struct ngene_channel *chan)
{
	void *feconf = chan->dev->card_info->fe_config[chan->number];

	chan->fe = stb0899_attach(feconf,
				  chan, &chan->i2c_adapter,
				  &chan->dev->pci_dev->dev);
	if (chan->fe) {
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 18)
		chan->set_tone = chan->fe->ops->set_tone;
		chan->fe->ops->set_tone = lnbh21_set_tone;
		chan->fe->ops->set_voltage = lnbh21_set_voltage;
#else
		chan->set_tone = chan->fe->ops.set_tone;
		chan->fe->ops.set_tone = lnbh21_set_tone;
		chan->fe->ops.set_voltage = lnbh21_set_voltage;
#endif
	}

	return (chan->fe) ? 0 : -ENODEV;
}
#endif

static int locked_gate_ctrl(struct dvb_frontend *fe, int enable)
{
	struct ngene_channel *chan = fe->sec_priv;
	int status;

	if (enable) {
		down(&chan->dev->pll_mutex);
		status = chan->gate_ctrl(fe, 1);
	} else {
		status = chan->gate_ctrl(fe, 0);
		up(&chan->dev->pll_mutex);
	}
	return status;
}

static int tuner_attach_tda18271(struct ngene_channel *chan)
{
	struct i2c_adapter *i2c;
	struct dvb_frontend *fe;

	i2c = &chan->dev->channel[0].i2c_adapter;
	if (chan->fe->ops.i2c_gate_ctrl)
		chan->fe->ops.i2c_gate_ctrl(chan->fe, 1);
	fe = dvb_attach(tda18271c2dd_attach, chan->fe, i2c, 0x60);
	if (chan->fe->ops.i2c_gate_ctrl)
		chan->fe->ops.i2c_gate_ctrl(chan->fe, 0);
	if (!fe) {
		printk(KERN_ERR "No TDA18271 found!\n");
		return -ENODEV;
	}

	return 0;
}

static int tuner_attach_tda18212dd(struct ngene_channel *chan)
{
	struct i2c_adapter *i2c;
	struct dvb_frontend *fe;
	u8 addr = (chan->number & 1) ? 0x63 : 0x60;

	if (chan->demod_type == DMD_CXD28xx && chan->number < 2)
		addr ^= 0x04;
	i2c = &chan->dev->channel[0].i2c_adapter;
	fe = dvb_attach(tda18212dd_attach, chan->fe, i2c, addr);
	if (!fe) {
		printk(KERN_ERR "No TDA18212 found!\n");
		return -ENODEV;
	}
	return 0;
}

static int tuner_attach_probe(struct ngene_channel *chan)
{
	switch(chan->demod_type)
	{
	case DMD_STV0900:
		return tuner_attach_stv6110(chan);

	case DMD_DRXK:
		return tuner_attach_tda18271(chan);

	case DMD_STV0367:
	case DMD_CXD28xx:
		return tuner_attach_tda18212dd(chan);

	default:
		pr_err("Unknown demod %x\n", chan->demod_type);
		break;
	}
	return -EINVAL;
}

static int demod_attach_stv0900(struct ngene_channel *chan)
{
	struct i2c_adapter *i2c;
	struct stv090x_config *feconf = (struct stv090x_config *)
		chan->dev->card_info->fe_config[chan->number];

	/* tuner 1+2: i2c adapter #0, tuner 3+4: i2c adapter #1 */
	/* Note: Both adapters share the same i2c bus, but the demod     */
	/*       driver requires that each demod has its own i2c adapter */
	if (chan->number < 2)
		i2c = &chan->dev->channel[0].i2c_adapter;
	else
		i2c = &chan->dev->channel[1].i2c_adapter;

	chan->fe = dvb_attach(stv090x_attach, feconf, i2c,
			(chan->number & 1) == 0 ? STV090x_DEMODULATOR_0
						: STV090x_DEMODULATOR_1);
	if (chan->fe == NULL) {
		printk(KERN_ERR	DEVICE_NAME ": No STV0900 found!\n");
		return -ENODEV;
	}

	/* store channel info */
	if (feconf->tuner_i2c_lock)
		chan->fe->analog_demod_priv = chan;

	if (!dvb_attach(lnbh24_attach, chan->fe, i2c, 0,
			0, chan->dev->card_info->lnb[chan->number])) {
		printk(KERN_ERR DEVICE_NAME ": No LNBH24 found!\n");
		dvb_frontend_detach(chan->fe);
		chan->fe = NULL;
		return -ENODEV;
	}

	return 0;
}

static void cineS2_tuner_i2c_lock(struct dvb_frontend *fe, int lock)
{
	struct ngene_channel *chan = fe->analog_demod_priv;

	if (lock)
		down(&chan->dev->pll_mutex);
	else
		up(&chan->dev->pll_mutex);
}

static int i2c_read(struct i2c_adapter *adapter, u8 adr, u8 *val)
{
	struct i2c_msg msgs[1] = {{.addr = adr,  .flags = I2C_M_RD,
				   .buf  = val,  .len   = 1 } };
	return (i2c_transfer(adapter, msgs, 1) == 1) ? 0 : -1;
}

static int i2c_read_regs(struct i2c_adapter *adapter,
			 u8 adr, u8 reg, u8 *val, u8 len)
{
	struct i2c_msg msgs[2] = {{.addr = adr,  .flags = 0,
				   .buf  = &reg, .len   = 1 },
				  {.addr = adr,  .flags = I2C_M_RD,
				   .buf  = val,  .len   = len } };
	return (i2c_transfer(adapter, msgs, 2) == 2) ? 0 : -1;
}

static int i2c_read_reg(struct i2c_adapter *adapter, u8 adr,
			u8 reg, u8 *val)
{
	struct i2c_msg msgs[2] = {{.addr = adr, .flags = 0,
				   .buf  = &reg, .len  = 1},
				  {.addr = adr, .flags = I2C_M_RD,
				   .buf  = val, .len   = 1} };
	return (i2c_transfer(adapter, msgs, 2) == 2) ? 0 : -1;
}

static int i2c_read_reg16(struct i2c_adapter *adapter, u8 adr,
			  u16 reg, u8 *val)
{
	u8 msg[2] = {reg>>8, reg&0xff};
	struct i2c_msg msgs[2] = {{.addr = adr, .flags = 0,
				   .buf  = msg, .len   = 2},
				  {.addr = adr, .flags = I2C_M_RD,
				   .buf  = val, .len   = 1} };
	return (i2c_transfer(adapter, msgs, 2) == 2) ? 0 : -1;
}

static int i2c_write_reg(struct i2c_adapter *adapter, u8 adr,
			 u8 reg, u8 val)
{
	u8 msg[2] = {reg, val};
	struct i2c_msg msgs[1] = {{.addr = adr,  .flags = 0,
                                   .buf  = msg,  .len   = 2 } };
        return (i2c_transfer(adapter, msgs, 1) == 1) ? 0 : -1;
}

static int port_has_stv0900(struct i2c_adapter *i2c, int port)
{
	u8 val;
	if (i2c_read_reg16(i2c, 0x68+port/2, 0xf100, &val) < 0)
		return 0;
	return 1;
}

static int port_has_drxk(struct i2c_adapter *i2c, int port)
{
	u8 val;

	if (i2c_read(i2c, 0x29+port, &val) < 0)
		return 0;
	return 1;
}

static int demod_attach_drxk(struct ngene_channel *chan,
			     struct i2c_adapter *i2c)
{
	struct drxk_config config;

	memset(&config, 0, sizeof(config));
	config.adr = 0x29 + (chan->number ^ 2);
	config.microcode_name = "drxk_a3.mc";

#ifdef USE_API3
	chan->fe = dvb_attach(drxk_attach, &config, i2c, &chan->fe2);
#else
	chan->fe = dvb_attach(drxk_attach, &config, i2c);
#endif
	if (!chan->fe) {
		printk(KERN_ERR "No DRXK found!\n");
		return -ENODEV;
	}
	chan->fe->sec_priv = chan;
	chan->gate_ctrl = chan->fe->ops.i2c_gate_ctrl;
	chan->fe->ops.i2c_gate_ctrl = locked_gate_ctrl;
	return 0;
}

static int port_has_stv0367(struct i2c_adapter *i2c, int port)
{
	u8 val;

	if (i2c_read_reg16(i2c, 0x1c + (port ^ 1), 0xf000, &val) < 0)
		return 0;
	if (val != 0x60)
		return 0;
	return 1;
}

static int demod_attach_stv0367dd(struct ngene_channel *chan,
				  struct i2c_adapter *i2c)
{
	struct stv0367_cfg cfg;

	memset(&cfg, 0, sizeof cfg);
	cfg.adr = 0x1c + (chan->number ^ 1);

	chan->fe = dvb_attach(stv0367_attach, i2c, &cfg, &chan->fe2);
	if (!chan->fe) {
		printk(KERN_ERR "No stv0367 found!\n");
		return -ENODEV;
	}
	chan->fe->sec_priv = chan;
	chan->gate_ctrl = chan->fe->ops.i2c_gate_ctrl;
	chan->fe->ops.i2c_gate_ctrl = locked_gate_ctrl;
	return 0;
}

static int port_has_xo2(struct i2c_adapter *i2c, int port, u8 *id)
{
	u8 addr = (port < 2) ? 0x14 : 0x10;
	u8 val;
	u8 probe[1] = { 0x00 }, data[4];
	struct i2c_msg msgs[2] = {{ .addr = addr,  .flags = 0,
				    .buf  = probe, .len   = 1 },
				  { .addr = addr,  .flags = I2C_M_RD,
				    .buf  = data,  .len   = 4 } };
#if 0
	u8 cfg[] = { 0x79, 0x00, 0x00 };
	struct i2c_msg cmsg = { .addr = 0x44, .flags = 0,
				.buf  = cfg,  .len   = 3 };
	if (port == 0)
		i2c_transfer(i2c, &cmsg, 1);
	pr_info("chan %d addr %x\n", port, addr);
	msleep(1000);
#endif
	val = i2c_transfer(i2c, msgs, 2);
	if (val != 2)
		return 0;

	if (data[0] != 'D' || data[1] != 'F')
		return 0;

	*id = data[2];
	return 1;
}

static int init_xo2(struct i2c_adapter *i2c, int port)
{
	u8 addr = (port < 2) ? 0x14 : 0x10;
	u8 val, data[2];
	int res;

	if (port & 1)
		return 0;

	res = i2c_read_regs(i2c, addr, 0x04, data, 2);
	if (res < 0)
		return res;

	if (data[0] != 0x01)  {
		pr_info("Invalid XO2\n");
		return -1;
	}

	i2c_read_reg(i2c, addr, 0x08, &val);
	if (val != 0) {
		i2c_write_reg(i2c, addr, 0x08, 0x00);
		msleep(100);
	}
	/* Enable tuner power, disable pll, reset demods */
	i2c_write_reg(i2c, addr, 0x08, 0x04);
	usleep_range(2000, 3000);
	/* Release demod resets */
	i2c_write_reg(i2c, addr, 0x08, 0x07);
	usleep_range(2000, 3000);
	/* Start XO2 PLL */
	i2c_write_reg(i2c, addr, 0x08, 0x87);

	return 0;
}

#define DDB_TUNER_XO2		16
#define DDB_TUNER_DVBS_STV0910	16
#define DDB_TUNER_DVBCT2_SONY	17
#define DDB_TUNER_ISDBT_SONY	18
#define DDB_TUNER_DVBC2T2_SONY	19
#define DDB_TUNER_ATSC_ST	20
#define DDB_TUNER_DVBC2T2_ST	21

static char *xo2names[] = {
	"DUAL DVB-S2",
	"DUAL DVB-C/T/T2",
	"DUAL DVB-ISDBT",
	"DUAL DVB-C/C2/T/T2",
	"DUAL ATSC",
	"DUAL DVB-C/C2/T/T2",
	"", ""
};

struct cxd2843_cfg cxd2843[] = {
	{ .adr = 0x68, },
	{ .adr = 0x69, },
	{ .adr = 0x6c, },
	{ .adr = 0x6d, },
};

static int demod_attach_cxd2843(struct ngene_channel *chan,
				struct i2c_adapter *i2c)
{
	chan->fe = dvb_attach(cxd2843_attach, i2c, cxd2843+chan->number);
	if (!chan->fe) {
		pr_err("No cxd2837/38/43 found!\n");
		return -ENODEV;
	}
	chan->fe->sec_priv = chan;
	chan->gate_ctrl = chan->fe->ops.i2c_gate_ctrl;
	chan->fe->ops.i2c_gate_ctrl = locked_gate_ctrl;
	return 0;
}

static int cineS2_probe(struct ngene_channel *chan)
{
	struct i2c_adapter *i2c;
	struct stv090x_config *fe_conf;
	u8 buf[3];
	struct i2c_msg i2c_msg = { .flags = 0, .buf = buf };
	int rc;
	u8 id;

	/* tuner 1+2: i2c adapter #0, tuner 3+4: i2c adapter #1 */
	if (chan->number < 2)
		i2c = &chan->dev->channel[0].i2c_adapter;
	else
		i2c = &chan->dev->channel[1].i2c_adapter;

	if (port_has_xo2(i2c, chan->number, &id)) {
		id >>= 2;
		if (id > 5) {
			pr_info("Channel %d: Unknown XO2 DuoFlex %u\n",
				chan->number, id);
			return -ENODEV;
		}
		init_xo2(i2c, chan->number);
		switch(DDB_TUNER_XO2 + id)
		{
		case DDB_TUNER_DVBCT2_SONY:
		case DDB_TUNER_ISDBT_SONY:
		case DDB_TUNER_DVBC2T2_SONY:
			chan->demod_type = DMD_CXD28xx;
			pr_info("Channel %d: %s\n", chan->number, xo2names[id]);
			demod_attach_cxd2843(chan, i2c);		
			break;
		default:
			pr_info("Channel %d: %s not supported yet\n",
				chan->number, xo2names[id]);
			return -ENODEV;
		}

	} else if (chan->dev->channel[0].demod_type != DMD_CXD28xx &&
		   port_has_stv0900(i2c, chan->number)) {
		chan->demod_type = DMD_STV0900;
		pr_info("Channel %d: STV0900\n", chan->number);
		fe_conf = chan->dev->card_info->fe_config[chan->number];
		/* demod found, attach it */
		rc = demod_attach_stv0900(chan);
		if (rc < 0 || chan->number < 2)
			return rc;

		/* demod #2: reprogram outputs DPN1 & DPN2 */
		i2c_msg.addr = fe_conf->address;
		i2c_msg.len = 3;
		buf[0] = 0xf1;
		switch (chan->number) {
		case 2:
			buf[1] = 0x5c;
			buf[2] = 0xc2;
			break;
		case 3:
			buf[1] = 0x61;
			buf[2] = 0xcc;
			break;
		default:
			return -ENODEV;
		}
		rc = i2c_transfer(i2c, &i2c_msg, 1);
		if (rc != 1) {
			printk(KERN_ERR DEVICE_NAME ": could not setup DPNx\n");
			return -EIO;
		}

	} else if (port_has_drxk(i2c, chan->number^2)) {
		chan->demod_type = DMD_DRXK;
		pr_info("Channel %d: DRXK\n", chan->number);
		demod_attach_drxk(chan, i2c);

	} else if (port_has_stv0367(i2c, chan->number)) {
		chan->demod_type = DMD_STV0367;
		pr_info("Channel %d: STV0367\n", chan->number);
		demod_attach_stv0367dd(chan, i2c);

	} else {
		printk(KERN_ERR "No demod found on chan %d\n", chan->number);
		return -ENODEV;
	}
	return 0;
}


static struct lgdt330x_config aver_m780 = {
	.demod_address = 0xb2 >> 1,
	.demod_chip    = LGDT3303,
	.serial_mpeg   = 0x00, /* PARALLEL */
	.clock_polarity_flip = 1,
};

static struct mt2131_config m780_tunerconfig = {
	0xc0 >> 1
};

/* A single func to attach the demo and tuner, rather than
 * use two sep funcs like the current design mandates.
 */
static int demod_attach_lg330x(struct ngene_channel *chan)
{
	chan->fe = dvb_attach(lgdt330x_attach, &aver_m780, &chan->i2c_adapter);
	if (chan->fe == NULL) {
		printk(KERN_ERR	DEVICE_NAME ": No LGDT330x found!\n");
		return -ENODEV;
	}

	dvb_attach(mt2131_attach, chan->fe, &chan->i2c_adapter,
		   &m780_tunerconfig, 0);

	return (chan->fe) ? 0 : -ENODEV;
}

/****************************************************************************/
/* Switch control (I2C gates, etc.) *****************************************/
/****************************************************************************/

#if 0
static int avf_output(struct ngene_channel *chan, int state)
{
	if (chan->dev->card_info->avf[chan->number])
		i2c_write_register(&chan->i2c_adapter,
				   chan->dev->card_info->avf[chan->number],
				   0xf2, state ? 0x89 : 0x80);
	return 0;
}

/* Viper expander: sw11,sw12,sw21,sw22,i2csw1,i2csw2,tsen1,tsen2 */

static int exp_set(struct ngene *dev)
{
	return i2c_write(&dev->channel[0].i2c_adapter,
			 dev->card_info->exp, dev->exp_val);
}

static int exp_init(struct ngene *dev)
{
	if (!dev->card_info->exp)
		return 0;
	dev->exp_val = dev->card_info->exp_init;
	return exp_set(dev);
}

static int exp_set_bit(struct ngene *dev, int bit, int val)
{
	if (val)
		set_bit(bit, &dev->exp_val);
	else
		clear_bit(bit, &dev->exp_val);
	return exp_set(dev);
}

static int viper_switch_ctrl(struct ngene_channel *chan, int type, int val)
{
	switch (type) {
	case 0: /* I2C tuner gate on/off */
		return exp_set_bit(chan->dev, 4 + chan->number, val);
	case 1: /* Stream: 0=TS 1=ITU */
		avf_output(chan, val);
		return exp_set_bit(chan->dev, 6 + chan->number, val);
	case 2: /* Input: 0=digital 1=analog antenna input */
		exp_set_bit(chan->dev, 0 + chan->number * 2, val ? 0 : 1);
		exp_set_bit(chan->dev, 1 + chan->number * 2, val ? 1 : 0);
		break;
	}
	return 0;
}

static int viper_switch_ctrl2(struct ngene_channel *chan, int type, int val)
{
	switch (type) {
	case 0: /* I2C tuner gate on/off */
		return exp_set_bit(chan->dev, 4 + chan->number, val);
	case 1: /* Stream: 0=TS 1=ITU */
		avf_output(chan, val);
		return exp_set_bit(chan->dev, 6 + chan->number, val);
	case 2: /* Input: 0=digital 1=analog antenna input */
		exp_set_bit(chan->dev, 0 + chan->number * 2, val ? 0 : 1);
		exp_set_bit(chan->dev, 1 + chan->number * 2, 0);
		break;
	}
	return 0;
}

static int viper_gate_ctrl(struct dvb_frontend *fe, int enable)
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 19)
	struct ngene_channel *chan = fe->misc_priv;
#else /* Why is there no misc_priv available anymore !?!?! */
	/* Well, just abuse sec :-) */
	struct ngene_channel *chan = fe->sec_priv;
#endif
	struct ngene *dev = chan->dev;

	return dev->card_info->switch_ctrl(chan, 0, enable);
}

static int python_switch_ctrl(struct ngene_channel *chan, int type, int val)
{
	switch (type) {
	case 0: /* I2C tuner gate on/off */
		if (chan->number > 1)
			return -EINVAL;
		return ngene_command_gpio_set(chan->dev, 3 + chan->number, val);
	case 1: /* Stream: 0=TS 1=ITU */
		avf_output(chan, val);
		return 0;
	}
	return 0;
}

static int viper_reset_xc(struct dvb_frontend *fe)
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 19)
	struct ngene_channel *chan = fe->misc_priv;
#else
	struct ngene_channel *chan = fe->sec_priv;
#endif
	struct ngene *dev = chan->dev;

	printk(KERN_INFO DEVICE_NAME ": Reset XC3028\n");

	if (chan->number > 1)
		return -EINVAL;

	ngene_command_gpio_set(dev, 3 + chan->number, 0);
	msleep(150);
	ngene_command_gpio_set(dev, 3 + chan->number, 1);
	return 0;
}

static int python_gate_ctrl(struct dvb_frontend *fe, int enable)
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 19)
	struct ngene_channel *chan = fe->misc_priv;
#else /* Why is there no misc_priv available anymore !?!?! */
	struct ngene_channel *chan = fe->sec_priv;
#endif
	struct ngene *dev = chan->dev;

	if (chan->number == 0)
		return ngene_command_gpio_set(dev, 3, enable);
	if (chan->number == 1)
		return ngene_command_gpio_set(dev, 4, enable);
	return -EINVAL;
}
#endif

static struct stv090x_config fe_cineS2 = {
	.device         = STV0900,
	.demod_mode     = STV090x_DUAL,
	.clk_mode       = STV090x_CLK_EXT,

	.xtal           = 27000000,
	.address        = 0x68,

	.ts1_mode       = STV090x_TSMODE_SERIAL_PUNCTURED,
	.ts2_mode       = STV090x_TSMODE_SERIAL_PUNCTURED,

	.repeater_level = STV090x_RPTLEVEL_16,

	.adc1_range	= STV090x_ADC_1Vpp,
	.adc2_range	= STV090x_ADC_1Vpp,

	.diseqc_envelope_mode = true,

	.tuner_i2c_lock = cineS2_tuner_i2c_lock,
};

static struct stv090x_config fe_cineS2_2 = {
	.device         = STV0900,
	.demod_mode     = STV090x_DUAL,
	.clk_mode       = STV090x_CLK_EXT,

	.xtal           = 27000000,
	.address        = 0x69,

	.ts1_mode       = STV090x_TSMODE_SERIAL_PUNCTURED,
	.ts2_mode       = STV090x_TSMODE_SERIAL_PUNCTURED,

	.repeater_level = STV090x_RPTLEVEL_16,

	.adc1_range	= STV090x_ADC_1Vpp,
	.adc2_range	= STV090x_ADC_1Vpp,

	.diseqc_envelope_mode = true,

	.tuner_i2c_lock = cineS2_tuner_i2c_lock,
};

static struct stv6110x_config tuner_cineS2_0 = {
	.addr	= 0x60,
	.refclk	= 27000000,
	.clk_div = 1,
};

static struct stv6110x_config tuner_cineS2_1 = {
	.addr	= 0x63,
	.refclk	= 27000000,
	.clk_div = 1,
};

static struct ngene_info ngene_info_cineS2 = {
	.type		= NGENE_SIDEWINDER,
	.name		= "Linux4Media cineS2 DVB-S2 Twin Tuner",
	.io_type	= {NGENE_IO_TSIN, NGENE_IO_TSIN},
	.demod_attach	= {demod_attach_stv0900, demod_attach_stv0900},
	.tuner_attach	= {tuner_attach_stv6110, tuner_attach_stv6110},
	.fe_config	= {&fe_cineS2, &fe_cineS2},
	.tuner_config	= {&tuner_cineS2_0, &tuner_cineS2_1},
	.lnb		= {0x0b, 0x08},
	.tsf		= {3, 3},
	.fw_version	= 18,
	.msi_supported	= true,
};

static struct ngene_info ngene_info_satixS2 = {
	.type		= NGENE_SIDEWINDER,
	.name		= "Mystique SaTiX-S2 Dual",
	.io_type	= {NGENE_IO_TSIN, NGENE_IO_TSIN},
	.demod_attach	= {demod_attach_stv0900, demod_attach_stv0900},
	.tuner_attach	= {tuner_attach_stv6110, tuner_attach_stv6110},
	.fe_config	= {&fe_cineS2, &fe_cineS2},
	.tuner_config	= {&tuner_cineS2_0, &tuner_cineS2_1},
	.lnb		= {0x0b, 0x08},
	.tsf		= {3, 3},
	.fw_version	= 18,
	.msi_supported	= true,
};

static struct ngene_info ngene_info_satixS2v2 = {
	.type		= NGENE_SIDEWINDER,
	.name		= "Mystique SaTiX-S2 Dual (v2)",
	.io_type	= {NGENE_IO_TSIN, NGENE_IO_TSIN, NGENE_IO_TSIN, NGENE_IO_TSIN,
			   NGENE_IO_TSOUT},
	.demod_attach	= {demod_attach_stv0900, demod_attach_stv0900, cineS2_probe, cineS2_probe},
	.tuner_attach	= {tuner_attach_stv6110, tuner_attach_stv6110, tuner_attach_probe, tuner_attach_probe},
	.fe_config	= {&fe_cineS2, &fe_cineS2, &fe_cineS2_2, &fe_cineS2_2},
	.tuner_config	= {&tuner_cineS2_0, &tuner_cineS2_1, &tuner_cineS2_0, &tuner_cineS2_1},
	.lnb		= {0x0a, 0x08, 0x0b, 0x09},
	.tsf		= {3, 3},
	.fw_version	= 18,
	.msi_supported	= true,
};

static struct ngene_info ngene_info_cineS2v5 = {
	.type		= NGENE_SIDEWINDER,
	.name		= "Linux4Media cineS2 DVB-S2 Twin Tuner (v5)",
	.io_type	= {NGENE_IO_TSIN, NGENE_IO_TSIN, NGENE_IO_TSIN, NGENE_IO_TSIN,
			   NGENE_IO_TSOUT},
	.demod_attach	= {demod_attach_stv0900, demod_attach_stv0900, cineS2_probe, cineS2_probe},
	.tuner_attach	= {tuner_attach_stv6110, tuner_attach_stv6110, tuner_attach_probe, tuner_attach_probe},
	.fe_config	= {&fe_cineS2, &fe_cineS2, &fe_cineS2_2, &fe_cineS2_2},
	.tuner_config	= {&tuner_cineS2_0, &tuner_cineS2_1, &tuner_cineS2_0, &tuner_cineS2_1},
	.lnb		= {0x0a, 0x08, 0x0b, 0x09},
	.tsf		= {3, 3},
	.fw_version	= 18,
	.msi_supported	= true,
};


static struct ngene_info ngene_info_duoFlex = {
	.type           = NGENE_SIDEWINDER,
	.name           = "Digital Devices DuoFlex PCIe or miniPCIe",
	.io_type        = {NGENE_IO_TSIN, NGENE_IO_TSIN, NGENE_IO_TSIN, NGENE_IO_TSIN,
			   NGENE_IO_TSOUT},
	.demod_attach   = {cineS2_probe, cineS2_probe, cineS2_probe, cineS2_probe},
	.tuner_attach   = {tuner_attach_probe, tuner_attach_probe, tuner_attach_probe, tuner_attach_probe},
	.fe_config      = {&fe_cineS2, &fe_cineS2, &fe_cineS2_2, &fe_cineS2_2},
	.tuner_config   = {&tuner_cineS2_0, &tuner_cineS2_1, &tuner_cineS2_0, &tuner_cineS2_1},
	.lnb            = {0x0a, 0x08, 0x0b, 0x09},
	.tsf            = {3, 3},
	.fw_version     = 18,
	.msi_supported	= true,
};

static struct ngene_info ngene_info_m780 = {
	.type           = NGENE_APP,
	.name           = "Aver M780 ATSC/QAM-B",

	/* Channel 0 is analog, which is currently unsupported */
	.io_type        = { NGENE_IO_NONE, NGENE_IO_TSIN },
	.demod_attach   = { NULL, demod_attach_lg330x },

	/* Ensure these are NULL else the frame will call them (as funcs) */
	.tuner_attach   = { 0, 0, 0, 0 },
	.fe_config      = { NULL, &aver_m780 },
	.avf            = { 0 },

	/* A custom electrical interface config for the demod to bridge */
	.tsf		= { 4, 4 },
	.fw_version	= 15,
};

/****************************************************************************/

#if 0
static struct drxd_config fe_terratec_dvbt_0 = {
	.index          = 0,
	.demod_address  = 0x70,
	.demod_revision = 0xa2,
	.demoda_address = 0x00,
	.pll_address    = 0x60,
	.pll_type       = DRXD_PLL_DTT7520X,
	.clock          = 20000,
	.pll_set        = ngene_pll_set_th_dtt7520x,
	.osc_deviation  = osc_deviation,
};

static struct drxd_config fe_terratec_dvbt_1 = {
	.index          = 1,
	.demod_address  = 0x71,
	.demod_revision = 0xa2,
	.demoda_address = 0x00,
	.pll_address    = 0x60,
	.pll_type       = DRXD_PLL_DTT7520X,
	.clock          = 20000,
	.pll_set        = ngene_pll_set_th_dtt7520x,
	.osc_deviation  = osc_deviation,
};

static struct ngene_info ngene_info_terratec = {
	.type           = NGENE_TERRATEC,
	.name           = "Terratec Integra/Cinergy2400i Dual DVB-T",
	.io_type        = {NGENE_IO_TSIN, NGENE_IO_TSIN},
	.demod_attach   = {demod_attach_drxd, demod_attach_drxd},
	.fe_config      = {&fe_terratec_dvbt_0, &fe_terratec_dvbt_1},
	.i2c_access     = 1,
};

/****************************************************************************/

static struct mt2060_config tuner_python_0 = {
	.i2c_address    = 0x60,
	.clock_out      = 3,
	.input          = 0
};

static struct mt2060_config tuner_python_1 = {
	.i2c_address    = 0x61,
	.clock_out      = 3,
	.input          = 1
};

static struct drxd_config fe_python_0 = {
	.index          = 0,
	.demod_address  = 0x71,
	.demod_revision = 0xb1,
	.demoda_address = 0x41,
	.clock          = 16000,
	.osc_deviation  = osc_deviation,
};

static struct drxd_config fe_python_1 = {
	.index          = 1,
	.demod_address  = 0x70,
	.demod_revision = 0xb1,
	.demoda_address = 0x45,
	.clock          = 16000,
	.osc_deviation  = osc_deviation,
};

static struct ngene_info ngene_info_python = {
	.type           = NGENE_PYTHON,
	.name           = "Micronas MicPython/Hedgehog Dual DVB-T",
	.io_type        = {NGENE_IO_TSIN | NGENE_IO_TV,
			   NGENE_IO_TSIN | NGENE_IO_TV,
			   NGENE_IO_AIN, NGENE_IO_AIN},
	.demod_attach   = {demod_attach_drxd, demod_attach_drxd},
	.tuner_attach   = {tuner_attach_mt2060, tuner_attach_mt2060},
	.fe_config      = {&fe_python_0, &fe_python_1},
	.tuner_config   = {&tuner_python_0, &tuner_python_1},
	.avf            = {0x43, 0x47},
	.msp            = {0x40, 0x42},
	.demoda         = {0x41, 0x45},
	.gate_ctrl      = python_gate_ctrl,
	.switch_ctrl    = python_switch_ctrl,
};

/****************************************************************************/

static struct drxd_config fe_appb_dvbt_0 = {
	.index          = 0,
	.demod_address  = 0x71,
	.demod_revision = 0xa2,
	.demoda_address = 0x41,
	.pll_address    = 0x63,
	.pll_type       = DRXD_PLL_MT3X0823,
	.clock          = 20000,
	.pll_set        = ngene_pll_set_mt_3x0823,
	.osc_deviation  = osc_deviation,
};

static struct drxd_config fe_appb_dvbt_1 = {
	.index          = 1,
	.demod_address  = 0x70,
	.demod_revision = 0xa2,
	.demoda_address = 0x45,
	.pll_address    = 0x60,
	.pll_type       = DRXD_PLL_MT3X0823,
	.clock          = 20000,
	.pll_set        = ngene_pll_set_mt_3x0823,
	.osc_deviation  = osc_deviation,
};

static struct ngene_info ngene_info_appboard = {
	.type           = NGENE_APP,
	.name           = "Micronas Application Board Dual DVB-T",
	.io_type        = {NGENE_IO_TSIN, NGENE_IO_TSIN},
	.demod_attach   = {demod_attach_drxd, demod_attach_drxd},
	.fe_config      = {&fe_appb_dvbt_0, &fe_appb_dvbt_1},
	.avf            = {0x43, 0x47},
};

static struct ngene_info ngene_info_appboard_ntsc = {
	.type           = NGENE_APP,
	.name           = "Micronas Application Board Dual DVB-T",
	.io_type        = {NGENE_IO_TSIN, NGENE_IO_TSIN},
	.demod_attach   = {demod_attach_drxd, demod_attach_drxd},
	.fe_config      = {&fe_appb_dvbt_0, &fe_appb_dvbt_1},
	.avf            = {0x43, 0x47},
	.ntsc           = 1,
};

/****************************************************************************/

static struct stb0899_config fe_sidewinder_0 = {
	.demod_address  = 0x68,
	.pll_address    = 0x63,
};

static struct stb0899_config fe_sidewinder_1 = {
	.demod_address  = 0x6b,
	.pll_address    = 0x60,
};

static struct ngene_info ngene_info_sidewinder = {
	.type           = NGENE_SIDEWINDER,
	.name           = "Micronas MicSquirrel/Sidewinder Dual DVB-S2",
	.io_type        = {NGENE_IO_TSIN, NGENE_IO_TSIN},
	.demod_attach   = {demod_attach_stb0899, demod_attach_stb0899},
	.fe_config      = {&fe_sidewinder_0, &fe_sidewinder_1},
	.lnb            = {0x0b, 0x08},
};

/****************************************************************************/
/* Yet unnamed S2 card with dual DVB-S2 demod                               */
/****************************************************************************/

static struct stv0900_config fe_s2_0 = {
	.addr           = 0x68,
	.pll            = 0x63,
	.pll_type       = 0,
	.nr             = 0,
};

static struct stv0900_config fe_s2_1 = {
	.addr           = 0x68,
	.pll            = 0x60,
	.pll_type       = 0,
	.nr             = 1,
};

static struct ngene_info ngene_info_s2 = {
	.type           = NGENE_SIDEWINDER,
	.name           = "S2",
	.io_type        = {NGENE_IO_TSIN, NGENE_IO_TSIN,
			   NGENE_IO_TSIN, NGENE_IO_TSIN},
	.demod_attach   = {demod_attach_stv0900, demod_attach_stv0900},
	.fe_config      = {&fe_s2_0, &fe_s2_1},
	.lnb            = {0x0b, 0x08},
	.tsf            = {3, 3},
	.fw_version     = 15,
};

static struct stv0900_config fe_s2b_0 = {
	.addr           = 0x68,
	.pll            = 0x60,
	.pll_type       = 0x10,
	.nr             = 0,
};

static struct stv0900_config fe_s2b_1 = {
	.addr           = 0x68,
	.pll            = 0x63,
	.pll_type       = 0x10,
	.nr             = 1,
};

static struct ngene_info ngene_info_s2_b = {
	.type           = NGENE_SIDEWINDER,
	.name           = "S2 V2",
	.io_type        = {NGENE_IO_TSIN, NGENE_IO_TSIN,
			   NGENE_IO_TSIN, NGENE_IO_TSIN},
	.demod_attach   = {demod_attach_stv0900, demod_attach_stv0900},
	.fe_config      = {&fe_s2b_0, &fe_s2b_1},
	.lnb            = {0x0b, 0x08},
	.tsf            = {3, 3},
	.fw_version     = 17,
};

/****************************************************************************/

static struct xc3028_config tuner_viper_0 = {
	.adr            = 0x61,
	.reset          = viper_reset_xc
};

static struct xc3028_config tuner_viper_1 = {
	.adr            = 0x64,
	.reset          = viper_reset_xc
};

static struct drxh_config fe_viper_h_0 = {.adr = 0x2b};

static struct drxh_config fe_viper_h_1 = {.adr = 0x29};

static struct drxh_config fe_viper_l_0 = {.adr = 0x2b, .type = 3931};

static struct drxh_config fe_viper_l_1 = {.adr = 0x29, .type = 3931};

static struct ngene_info ngene_info_viper_v1 = {
	.type           = NGENE_VIPER,
	.name           = "Micronas MicViper Dual ATSC DRXH",
	.io_type        = {NGENE_IO_TSIN | NGENE_IO_TV,
			   NGENE_IO_TSIN | NGENE_IO_TV,
			   NGENE_IO_AIN, NGENE_IO_AIN},
	.demod_attach   = {demod_attach_drxh, demod_attach_drxh},
	.fe_config      = {&fe_viper_h_0, &fe_viper_h_1},
	.tuner_config   = {&tuner_viper_0, &tuner_viper_1},
	.tuner_attach   = {tuner_attach_xc3028, tuner_attach_xc3028},
	.avf            = {0x43, 0x47},
	.msp            = {0x40, 0x42},
	.exp            = 0x20,
	.exp_init       = 0xf5,
	.gate_ctrl      = viper_gate_ctrl,
	.switch_ctrl    = viper_switch_ctrl,
	.tsf            = {2, 2},
};

static struct ngene_info ngene_info_viper_v2 = {
	.type           = NGENE_VIPER,
	.name           = "Micronas MicViper Dual ATSC DRXL",
	.io_type        = {NGENE_IO_TSIN | NGENE_IO_TV,
			   NGENE_IO_TSIN | NGENE_IO_TV,
			   NGENE_IO_AIN, NGENE_IO_AIN},
	.demod_attach   = {demod_attach_drxh, demod_attach_drxh},
	.fe_config      = {&fe_viper_l_0, &fe_viper_l_1},
	.tuner_config   = {&tuner_viper_0, &tuner_viper_1},
	.tuner_attach   = {tuner_attach_xc3028, tuner_attach_xc3028},
	.avf            = {0x43, 0x47},
	.msp            = {0x40, 0x42},
	.exp            = 0x38,
	.exp_init       = 0xf5,
	.gate_ctrl      = viper_gate_ctrl,
	.switch_ctrl    = viper_switch_ctrl,
	.tsf            = {2, 2},
};

/****************************************************************************/

static struct ngene_info ngene_info_vbox_v1 = {
	.type           = NGENE_VBOX_V1,
	.name           = "VBox Cat's Eye 164E",
	.io_type        = {NGENE_IO_TSIN | NGENE_IO_TV,
			   NGENE_IO_TSIN | NGENE_IO_TV,
			   NGENE_IO_AIN, NGENE_IO_AIN},
	.demod_attach   = {demod_attach_drxh, demod_attach_drxh},
	.fe_config      = {&fe_viper_h_0, &fe_viper_h_1},
	.tuner_config   = {&tuner_viper_0, &tuner_viper_1},
	.tuner_attach   = {tuner_attach_xc3028, tuner_attach_xc3028},
	.avf            = {0x43, 0x47},
	.msp            = {0x40, 0x42},
	.exp            = 0x20,
	.exp_init       = 0xf5,
	.gate_ctrl      = viper_gate_ctrl,
	.switch_ctrl    = viper_switch_ctrl,
	.tsf            = {2, 2},
};

/****************************************************************************/

static struct ngene_info ngene_info_vbox_v2 = {
	.type           = NGENE_VBOX_V2,
	.name           = "VBox Cat's Eye 164E",
	.io_type        = {NGENE_IO_TSIN | NGENE_IO_TV,
			   NGENE_IO_TSIN | NGENE_IO_TV,
			   NGENE_IO_AIN, NGENE_IO_AIN},
	.demod_attach   = {demod_attach_drxh, demod_attach_drxh},
	.fe_config      = {&fe_viper_h_0, &fe_viper_h_1},
	.tuner_config   = {&tuner_viper_0, &tuner_viper_1},
	.tuner_attach   = {tuner_attach_xc3028, tuner_attach_xc3028},
	.avf            = {0x43, 0x47},
	.msp            = {0x40, 0x42},
	.exp            = 0x20,
	.exp_init       = 0xf5,
	.gate_ctrl      = viper_gate_ctrl,
	.switch_ctrl    = viper_switch_ctrl2,
	.tsf            = {2, 2},
};

/****************************************************************************/

static struct ngene_info ngene_info_racer = {
	.type           = NGENE_RACER,
	.name           = "Micronas MicRacer HDTV Decoder Card",
	.io_type        = {NGENE_IO_HDTV, NGENE_IO_NONE,
			   NGENE_IO_AIN, NGENE_IO_NONE,
			   NGENE_IO_TSOUT},
	.i2s            = {0, 0, 1, 0},
	.fw_version     = 17,
};
#endif


/****************************************************************************/
/* PCI Subsystem ID *********************************************************/
/****************************************************************************/

#define NGENE_ID(_subvend, _subdev, _driverdata) { \
	.vendor = NGENE_VID, .device = NGENE_PID, \
	.subvendor = _subvend, .subdevice = _subdev, \
	.driver_data = (unsigned long) &_driverdata }

/****************************************************************************/

static const struct pci_device_id ngene_id_tbl[] = {
	NGENE_ID(0x18c3, 0xab04, ngene_info_cineS2),
	NGENE_ID(0x18c3, 0xab05, ngene_info_cineS2v5),
	NGENE_ID(0x18c3, 0xabc3, ngene_info_cineS2),
	NGENE_ID(0x18c3, 0xabc4, ngene_info_cineS2),
	NGENE_ID(0x18c3, 0xdb01, ngene_info_satixS2),
	NGENE_ID(0x18c3, 0xdb02, ngene_info_satixS2v2),
	NGENE_ID(0x18c3, 0xdd00, ngene_info_cineS2v5),
	NGENE_ID(0x18c3, 0xdd10, ngene_info_duoFlex),
	NGENE_ID(0x18c3, 0xdd20, ngene_info_duoFlex),
	NGENE_ID(0x1461, 0x062e, ngene_info_m780),
#if 0 /* not (yet?) supported */
	NGENE_ID(0x18c3, 0x0000, ngene_info_appboard),
	NGENE_ID(0x18c3, 0x0004, ngene_info_appboard),
	NGENE_ID(0x18c3, 0x8011, ngene_info_appboard),
	NGENE_ID(0x18c3, 0x8015, ngene_info_appboard_ntsc),
	NGENE_ID(0x153b, 0x1167, ngene_info_terratec),
	NGENE_ID(0x18c3, 0x0030, ngene_info_python),
	NGENE_ID(0x18c3, 0x0052, ngene_info_sidewinder),
	NGENE_ID(0x18c3, 0x8f00, ngene_info_racer),
	NGENE_ID(0x18c3, 0x0041, ngene_info_viper_v1),
	NGENE_ID(0x18c3, 0x0042, ngene_info_viper_v2),
	NGENE_ID(0x14f3, 0x0041, ngene_info_vbox_v1),
	NGENE_ID(0x14f3, 0x0043, ngene_info_vbox_v2),
	NGENE_ID(0x18c3, 0xabcd, ngene_info_s2),
	NGENE_ID(0x18c3, 0xabc2, ngene_info_s2_b),
	NGENE_ID(0x18c3, 0xabc3, ngene_info_s2_b),
	NGENE_ID(0x18c3, 0x0001, ngene_info_appboard),
	NGENE_ID(0x18c3, 0x0005, ngene_info_appboard),
	NGENE_ID(0x18c3, 0x0009, ngene_info_appboard_atsc),
	NGENE_ID(0x18c3, 0x000b, ngene_info_appboard_atsc),
	NGENE_ID(0x18c3, 0x0010, ngene_info_shrek_50_fp),
	NGENE_ID(0x18c3, 0x0011, ngene_info_shrek_60_fp),
	NGENE_ID(0x18c3, 0x0012, ngene_info_shrek_50),
	NGENE_ID(0x18c3, 0x0013, ngene_info_shrek_60),
	NGENE_ID(0x18c3, 0x0000, ngene_info_hognose),
#endif
	{0}
};
MODULE_DEVICE_TABLE(pci, ngene_id_tbl);

/****************************************************************************/
/* Init/Exit ****************************************************************/
/****************************************************************************/

static pci_ers_result_t ngene_error_detected(struct pci_dev *dev,
					     enum pci_channel_state state)
{
	printk(KERN_ERR DEVICE_NAME ": PCI error\n");
	if (state == pci_channel_io_perm_failure)
		return PCI_ERS_RESULT_DISCONNECT;
	if (state == pci_channel_io_frozen)
		return PCI_ERS_RESULT_NEED_RESET;
	return PCI_ERS_RESULT_CAN_RECOVER;
}

static pci_ers_result_t ngene_link_reset(struct pci_dev *dev)
{
	printk(KERN_INFO DEVICE_NAME ": link reset\n");
	return 0;
}

static pci_ers_result_t ngene_slot_reset(struct pci_dev *dev)
{
	printk(KERN_INFO DEVICE_NAME ": slot reset\n");
	return 0;
}

static void ngene_resume(struct pci_dev *dev)
{
	printk(KERN_INFO DEVICE_NAME ": resume\n");
}

static struct pci_error_handlers ngene_errors = {
	.error_detected = ngene_error_detected,
	.link_reset = ngene_link_reset,
	.slot_reset = ngene_slot_reset,
	.resume = ngene_resume,
};

static struct pci_driver ngene_pci_driver = {
	.name        = "ngene",
	.id_table    = ngene_id_tbl,
	.probe       = ngene_probe,
	.remove      = ngene_remove,
	.err_handler = &ngene_errors,
	.shutdown    = ngene_shutdown,
};

static __init int module_init_ngene(void)
{
	printk(KERN_INFO
	       "nGene PCIE bridge driver, Copyright (C) 2005-2007 Micronas\n");
	return pci_register_driver(&ngene_pci_driver);
}

static __exit void module_exit_ngene(void)
{
	pci_unregister_driver(&ngene_pci_driver);
}

module_init(module_init_ngene);
module_exit(module_exit_ngene);

MODULE_DESCRIPTION("nGene");
MODULE_AUTHOR("Micronas, Ralph Metzler, Manfred Voelkel");
MODULE_LICENSE("GPL");
