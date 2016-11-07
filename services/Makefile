obj-y := audioflinger/ bt_profiles/ fs/ bt_app/ overlay/ resources/ ../utils/crc32/
ifeq ($(BLE),1)
obj-y += bluedroid/ bridge/
endif

ifneq ($(FPGA),1)
obj-y += nvrecord/
endif

ifeq ($(PCTOOLS_SUPPT),1)
obj-y += rt_rwburn/
endif
