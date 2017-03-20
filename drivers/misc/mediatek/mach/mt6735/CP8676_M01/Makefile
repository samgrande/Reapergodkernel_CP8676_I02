
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

ifeq ($(CONFIG_MTK_SENSOR_SUPPORT),y)
ifeq ($(CONFIG_MTK_CM36652_NEW),y)
obj-y	+=  alsps/cm36652-new/
endif

ifeq ($(CONFIG_MTK_MC3410_NEW),y)
obj-y   +=  accelerometer/mc3410-new/
endif

ifeq ($(CONFIG_MTK_ITG1010_NEW),y)
obj-y   +=  gyroscope/ITG1010-new/
endif

ifeq ($(CONFIG_MTK_AKM09911_NEW),y)
obj-y	+=  magnetometer/akm09911-new/
endif

#liuxinyuan@yulong.com add begin 20150502
ifeq ($(CONFIG_MTK_pa12200001),y)
obj-y	+=  alsps/pa12200001/
endif


ifeq ($(CONFIG_MTK_MPU6880_ACC_NEW),y)
obj-y   +=  accelerometer/mpu6880_acc_new/
endif


ifeq ($(CONFIG_MTK_LSM6DS3_AUTO),y)
obj-y   +=  accelerometer/lsm6ds3_auto/
endif

ifeq ($(CONFIG_MTK_BMI160_ACC_AUTO),y)
obj-y   +=  accelerometer/bmi160_acc_auto/
endif
ifeq ($(CONFIG_MTK_MPU6880_GYRO_NEW),y)
obj-y   +=  gyroscope/mpu6880_gyro_new/
endif


ifeq ($(CONFIG_MTK_LSM6DS3GY_AUTO),y)
obj-y   +=  gyroscope/lsm6ds3gy_auto/
endif

ifeq ($(CONFIG_MTK_BMI160_GYRO_AUTO),y)
obj-y   +=  gyroscope/bmi160_gyro_auto/
endif

ifeq ($(CONFIG_MTK_AKM09911_AUTO),y)
obj-y +=  magnetometer/akm09911_auto/
endif

ifeq ($(CONFIG_MTK_YAS537_AUTO),y)
obj-y   +=  magnetometer/yamaha537_auto/
endif

#liuxinyuan@yulong.com add end
endif
