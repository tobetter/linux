obj-m	+= aml_swdmx.o

ccflags-y += -I$(srctree)/drivers/media/dvb-core -I$(srctree)/drivers/gpio -I$(srctree)/include

aml_swdmx-objs += aml_dvb.o
aml_swdmx-objs += aml_dsc.o
aml_swdmx-objs += aml_dmx.o
aml_swdmx-objs += aml_dmx_ext.o

aml_swdmx-objs += sw_demux/swdmx_aes_cbc.o
aml_swdmx-objs += sw_demux/swdmx_aes_ecb.o
aml_swdmx-objs += sw_demux/swdmx_cb_list.o
aml_swdmx-objs += sw_demux/swdmx_crc32.o
aml_swdmx-objs += sw_demux/swdmx_demux.o
aml_swdmx-objs += sw_demux/swdmx_descrambler.o
aml_swdmx-objs += sw_demux/swdmx_dvbcsa2.o
aml_swdmx-objs += sw_demux/swdmx_ts_parser.o
aml_swdmx-objs += sw_demux/dvbcsa2/dvbcsa_algo.o
aml_swdmx-objs += sw_demux/dvbcsa2/dvbcsa_block.o
aml_swdmx-objs += sw_demux/dvbcsa2/dvbcsa_bs_algo.o
aml_swdmx-objs += sw_demux/dvbcsa2/dvbcsa_bs_block.o
aml_swdmx-objs += sw_demux/dvbcsa2/dvbcsa_bs_key.o
aml_swdmx-objs += sw_demux/dvbcsa2/dvbcsa_bs_stream.o
aml_swdmx-objs += sw_demux/dvbcsa2/dvbcsa_key.o
aml_swdmx-objs += sw_demux/dvbcsa2/dvbcsa_stream.o

aml_swdmx-objs += sw_demux/dvbcsa2/dvbcsa_bs_transpose.o

aml_swdmx-objs += sw_demux/dvbcsa2/dvbcsa_bs_transpose32.o

aml_swdmx-objs += hw_demux/hwdemux.o
aml_swdmx-objs += hw_demux/frontend.o
aml_swdmx-objs += hw_demux/s2p.o
aml_swdmx-objs += hw_demux/asyncfifo.o
aml_swdmx-objs += hw_demux/hwdemux_inject.o

#obj-y += dvb_ci/
