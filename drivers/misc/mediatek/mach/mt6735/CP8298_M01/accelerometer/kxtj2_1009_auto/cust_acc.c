#include <linux/types.h>
#include <cust_acc.h>
#include <mach/mt_pm_ldo.h>

/*---------------------------------------------------------------------------*/
int kxtj2_cust_acc_power(struct acc_hw *hw, unsigned int on, char* devname)
{
#ifndef FPGA_EARLY_PORTING
    if (hw->power_id == MT65XX_POWER_NONE)
        return 0;
    if (on)
        return hwPowerOn(hw->power_id, hw->power_vol, devname);
    else
        return hwPowerDown(hw->power_id, devname); 
#else
    return 0;
#endif
}
/*---------------------------------------------------------------------------*/
static struct acc_hw kxtj2_cust_acc_hw = {
    .i2c_num = 2,
    .direction = 5,  //wangyufei@yulong.com modify for gsensor direction at 20150512
    .power_id = MT65XX_POWER_NONE,  /*!< LDO is not used */
    .power_vol= VOL_DEFAULT,        /*!< LDO is not used */
    .firlen = 0, //old value 16                /*!< don't enable low pass fileter */
    .is_batch_supported = false,
};
/*---------------------------------------------------------------------------*/
struct acc_hw* kxtj2_get_cust_acc_hw(void) 
{
    return &kxtj2_cust_acc_hw;
}
