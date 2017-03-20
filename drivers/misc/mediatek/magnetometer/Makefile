include $(srctree)/drivers/misc/mediatek/Makefile.custom

# In case the platform does NOT support this type of sensors
obj-y   +=  mag.o
obj-y   +=  mag_factory.o

ifeq ($(CONFIG_MTK_AKM8963_NEW),y)
obj-y   +=  akm8963-new/
endif

ifeq ($(CONFIG_MTK_BMM050_NEW),y)
obj-y   +=  bmm050-new/
endif

ifeq ($(CONFIG_MTK_BMM056_NEW),y)
obj-y   +=  bmm056-new/
endif
ifeq ($(CONFIG_MTK_AKM8963),y)
obj-y   +=  akm8963/
endif

ifeq ($(CONFIG_MTK_YAS532),y)
obj-y   +=  yamaha532/
endif

ifeq ($(CONFIG_MTK_YAS532_NEW),y)
obj-y   +=  yamaha532-new/
endif

#shihaobin@yulong.com add yas537 sensor 20150306 begin
ifeq ($(CONFIG_MTK_YAS537),y)
obj-y   +=  yamaha537/
endif

ifeq ($(CONFIG_MTK_YAS537_AUTO),y)
obj-y   +=  yamaha537_auto/
endif

ifeq ($(CONFIG_MTK_AKM09911_AUTO),y)
obj-y   +=  akm09911_auto/
endif
#shihaobin@yulong.com add yas537 sensor 20150306 end

ifeq ($(CONFIG_MTK_AKM09911),y)
obj-y   +=  akm09911/
endif

ifeq ($(CONFIG_MTK_AKM09911_NEW),y)
obj-y   +=  akm09911-new/
endif

ifeq ($(CONFIG_MTK_S62X),y)
obj-y   +=  s62x/
endif

ifeq ($(CONFIG_MTK_IST8303),y)
obj-y   +=  ist8303-new/
endif
# wangzhen P31 20150831
ifeq ($(CONFIG_MTK_MMC3524X),y)
obj-y   +=  mmc3524x/
endif
# wangzhen P31 20150831 end