#
# Makefile for the ddbridge device driver
#

obj-$(CONFIG_DVB_DDBRIDGE) += ddbridge.o

ccflags-y += -Idrivers/media/dvb-core/
ccflags-y += -Idrivers/media/dvb-frontends/
ccflags-y += -Idrivers/media/tuners/

# For the staging CI driver cxd2099
ccflags-y += -Idrivers/staging/media/cxd2099/
