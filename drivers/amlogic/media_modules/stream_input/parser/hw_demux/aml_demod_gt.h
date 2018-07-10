#ifndef __AML_DEMOD_GT_H__
#define __AML_DEMOD_GT_H__

#include  "dvb_frontend.h"

struct amlfe_exp_config {
	/*config by aml_fe ?*/
	/* */
	int set_mode;
};

struct amlfe_demod_config {
	int	dev_id;
	u32 ts;
	struct i2c_adapter *i2c_adap;
	int i2c_addr;
	int	reset_gpio;
	int	reset_value;
};

static inline struct dvb_frontend* aml_dtvdm_attach (const struct amlfe_exp_config *config) {
	return NULL;
}

static inline struct dvb_frontend* si2151_attach (struct dvb_frontend *fe,struct i2c_adapter *i2c,u8 i2c_addr/*,
				     struct si2151_config *cfg*/)
{
	return NULL;
}
static inline struct dvb_frontend* mxl661_attach (struct dvb_frontend *fe,struct i2c_adapter *i2c,u8 i2c_addr/*,
				     struct mxl661_config *cfg*/)
{
	return NULL;
}
static inline struct dvb_frontend* si2159_attach (struct dvb_frontend *fe,struct i2c_adapter *i2c,u8 i2c_addr/*,
                      struct si2159_config *cfg*/)
{
    return NULL;
}

static inline struct dvb_frontend* r842_attach (struct dvb_frontend *fe, struct i2c_adapter *i2c, u8 i2c_addr/*,
                      struct r842_config *cfg*/)
{
    return NULL;
}

static inline struct dvb_frontend* r840_attach (struct dvb_frontend *fe, struct i2c_adapter *i2c, u8 i2c_addr/*,
                      struct r840_config *cfg*/)
{
    return NULL;
}

static inline struct dvb_frontend* atbm8881_attach (const struct amlfe_demod_config *config)
{
	return NULL;
}

#endif	/*__AML_DEMOD_GT_H__*/
