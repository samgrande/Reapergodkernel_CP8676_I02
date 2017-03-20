include $(srctree)/drivers/misc/mediatek/Makefile.custom

# In case the platform does NOT support this type of sensors

obj-y   +=  accel.o accel_factory.o

ifeq ($(CONFIG_MTK_K2DH),y)
obj-y   +=  k2dh/
endif

ifeq ($(CONFIG_MTK_BMA050),y)
obj-y   +=  bma050/
endif
ifeq ($(CONFIG_MTK_BMA050_NEW),y)
obj-y   +=  bma050-new/
endif
ifeq ($(CONFIG_MTK_BMA222E),y)
obj-y   +=  bma222E/
endif
ifeq ($(CONFIG_MTK_BMA222E_NEW),y)
obj-y   +=  bma222E-new/
endif
ifeq ($(CONFIG_MTK_MPU6050G),y)
obj-y   +=  mpu6050g/
endif
ifeq ($(CONFIG_MTK_MPU6050G_NEW),y)
obj-y   +=  mpu6050g-new/
endif
ifeq ($(CONFIG_MTK_BMA250),y)
obj-y   +=  bma250/
endif
# add wangyufei@yulong.com begin
ifeq ($(CONFIG_MTK_BMA250_AUTO),y)
obj-y   +=  bma250_auto/
endif
ifeq ($(CONFIG_MTK_KXTJ2_1009_AUTO),y)
obj-y   +=  kxtj2_1009_auto/
endif
# add wangyufei@yulong.com begin
ifeq ($(CONFIG_MTK_KXTIK1004),y)
obj-y   +=  kxtik1004/
endif

ifeq ($(CONFIG_MTK_MPU6515A),y)
obj-y   +=  mpu6515/
endif

#shihaobin@yulong.com add 20150203 begin
ifeq ($(CONFIG_MTK_MPU6880_ACC_NEW),y)
obj-y   +=  mpu6880_acc_new/
endif

ifeq ($(CONFIG_MTK_LSM6DS3),y)
obj-y   +=  lsm6ds3/
endif

ifeq ($(CONFIG_MTK_LSM6DS3_AUTO),y)
obj-y   +=  lsm6ds3_auto/
endif

ifeq ($(CONFIG_MTK_BMI160_ACC_AUTO),y)
obj-y   +=  bmi160_acc_auto/
endif
#shihaobin@yulong.com add 20150203 end

ifeq ($(CONFIG_MTK_KXTJ2_1009),y)
obj-y   +=  KXTJ2_1009/
endif

ifeq ($(CONFIG_MTK_KXTJ2_1009_NEW),y)
obj-y   +=  KXTJ2_1009-new/
endif

ifeq ($(CONFIG_MTK_BMA250E),y)
obj-y   +=  bma250e/
endif
ifeq ($(CONFIG_MTK_MC3410_NEW),y)
obj-y   +=  mc3410-new/
endif

ifeq ($(CONFIG_MTK_MC3XXX_AUTO),y)
obj-y   +=  mc3xxx_auto/
endif

ifeq ($(CONFIG_MTK_MPU60X0),y)
obj-y   +=  mpu60x0/
endif
ifeq ($(CONFIG_MTK_BMA255_SDO0),y)
obj-y   +=  bma255-sdo0/
endif

ifeq ($(CONFIG_MTK_BMA255_SDO1),y)
obj-y   +=  bma255-sdo1/
endif
ifeq ($(CONFIG_MTK_BMA056),y)
obj-y   +=  bma056/
endif

ifeq ($(CONFIG_MTK_LSM6DS3),y)
obj-y   +=  lsm6ds3/
endif

ifeq ($(CONFIG_MTK_MXC400X_NEW),y)
obj-y   +=  mxc400x-new/
endif
ifeq ($(CONFIG_MTK_DA213),y)
obj-y   +=  da213/
endif
