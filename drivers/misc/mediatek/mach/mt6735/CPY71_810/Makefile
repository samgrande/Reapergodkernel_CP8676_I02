
include $(srctree)/drivers/misc/mediatek/Makefile.custom

ifneq ($(CONFIG_ARCH_MTK_PROJECT),)
ARCH_MTK_PROJECT := $(shell echo $(CONFIG_ARCH_MTK_PROJECT))
obj-y   += core/
obj-y   += dct/dct/
endif

ifeq ($(CONFIG_MTK_HDMI_SUPPORT),y)
obj-y	+=  hdmi/
endif

#obj-y   +=  lcm/
obj-y   +=  camera/camera/
obj-$(CONFIG_MTK_VIBRATOR)   +=  vibrator/
obj-$(CONFIG_MTK_LEDS)   +=  leds/
obj-y   +=  keypad/
#obj-y   +=  sound/
obj-y   +=  accdet/
# add wangyufei@yulong begin 20150112
ifeq ($(CONFIG_MTK_SENSOR_SUPPORT),y)
ifeq ($(CONFIG_MTK_LTR559_AUTO),y)
obj-y	+=  alsps/ltr559_auto/
endif

ifeq ($(CONFIG_MTK_PA12200001_AUTO),y)
obj-y	+=  alsps/PA12200001_auto/
endif

ifeq ($(CONFIG_MTK_TMD27713_AUTO),y)
obj-y	+=  alsps/tmd27713_auto/
endif

ifeq ($(CONFIG_MTK_BMA250_AUTO),y)
obj-y   +=  accelerometer/bma250_auto/
endif
ifeq ($(CONFIG_MTK_KXTJ2_1009_AUTO),y)
obj-y   +=  accelerometer/kxtj2_1009_auto/
endif
ifeq ($(CONFIG__MTK_KXTJ2_1009_NEW),y)
obj-y   +=  accelerometer/KXTJ2_1009-new/
endif
# add wangyufei@yulong end 20150112
ifeq ($(CONFIG_MTK_MC3410_NEW),y)
obj-y   +=  accelerometer/mc3410-new/
endif

ifeq ($(CONFIG_MTK_ITG1010_NEW),y)
obj-y   +=  gyroscope/ITG1010-new/
endif

ifeq ($(CONFIG_MTK_AKM09911_NEW),y)
obj-y	+=  magnetometer/akm09911-new/
endif
endif
