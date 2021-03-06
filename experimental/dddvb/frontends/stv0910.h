#ifndef _STV0910_H_
#define _STV0910_H_

#include <linux/types.h>
#include <linux/i2c.h>

struct stv0910_cfg {
	u32 clk;
	u8  adr;
	u8  parallel;
	u8  rptlvl;
};

#if defined(CONFIG_DVB_STV0910) || \
	(defined(CONFIG_DVB_STV0910_MODULE) && defined(MODULE))

extern struct dvb_frontend *stv0910_attach(struct i2c_adapter *i2c,
					   struct stv0910_cfg *cfg, int nr);
#else

static inline struct dvb_frontend *stv0910_attach(struct i2c_adapter *i2c,
						  struct stv0910_cfg *cfg,
						  int nr)
{
	pr_warn("%s: driver disabled by Kconfig\n", __func__);
	return NULL;
}

#endif

#endif
