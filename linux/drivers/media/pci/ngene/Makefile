#
# Makefile for the nGene device driver
#

ngene-objs := ngene-core.o ngene-i2c.o ngene-cards.o ngene-av.o \
	ngene-eeprom.o ngene-dvb.o

obj-$(CONFIG_DVB_NGENE) += ngene.o

ccflags-y += -Idrivers/media/dvb-core/
ccflags-y += -Idrivers/media/dvb-frontends/
ccflags-y += -Idrivers/media/tuners/

# For the staging CI driver cxd2099
ccflags-y += -Idrivers/staging/media/cxd2099/
