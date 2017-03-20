include $(srctree)/drivers/misc/mediatek/Makefile.custom

# Linux driver folder
#ccflags-y += -I$(srctree)/arch/arm/mach-$(MTK_PLATFORM)/$(ARCH_MTK_PROJECT)/touchpanel/ft5446/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/mach/$(MTK_PLATFORM)/$(ARCH_MTK_PROJECT)/touchpanel/ft5446/
ccflags-y += -I$(srctree)/drivers/input/touchscreen/mediatek/ft5446/
ccflags-y += -I$(srctree)/drivers/input/touchscreen/mediatek/

#obj-y   +=  focaltech_driver.o
#obj-y   +=  focaltech_ctl.o
#obj-y   +=  focaltech_ex_fun.o
#obj-y   +=  touchscreen.o

obj-$(CONFIG_TOUCHSCREEN_MTK_FT5X46) += focaltech.o
focaltech-objs += focaltech_driver.o focaltech_ctl.o focaltech_ex_fun.o touchscreen.o test_64bit_lib_v1.a
