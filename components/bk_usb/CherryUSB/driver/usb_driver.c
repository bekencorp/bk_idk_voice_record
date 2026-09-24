#include <os/os.h>
#include <os/mem.h>
#include <common/bk_err.h>
#include <driver/int_types.h>
#include <driver/int.h>
#include <driver/gpio.h>
#include <driver/gpio_types.h>
#include "gpio_driver.h"
#include "sys_driver.h"
#include "sys_types.h"
#include "sys_rtos.h"
#include "usb_driver.h"
#include "usb_regs_address.h"
#if CONFIG_USB_HOST
#include <components/cherryusb/usbh_core.h>
#endif
#if CONFIG_USB_DEVICE
#include <components/cherryusb/usbd_core.h>
#endif
#if CONFIG_USB_HUB
#include <components/cherryusb/usbh_hub.h>
#endif

#if CONFIG_USB_UAC
#include <components/cherryusb/usbh_audio.h>
#endif
#if CONFIG_USB_UVC
#include <components/cherryusb/usbh_video.h>
#endif

#if CONFIG_USB_CDC_ACM_DEMO
#include "usbh_cdc_acm.h"
#include "bk_cherry_usb_cdc_acm_api.h"
#endif
#if CONFIG_USBH_SERIAL_CH340
#include "usbh_ch34x.h"
#endif


static beken_thread_t  s_usb_drv_thread_hdl = NULL;
static beken_queue_t s_usb_drv_msg_que = NULL;
static beken_mutex_t s_usb_drv_task_mutex = NULL;
static bk_usb_driver_comprehensive_ops *s_usb_driver_ops = NULL;

static bool s_usb_driver_init_flag = 0;
static bool s_usb_power_on_flag = 0;
static bool s_usb_open_close_flag = 0;
static bool s_usbh_device_connect_flag = 0;

static bk_err_t usb_driver_sw_deinit();

#define USB_DRIVER_RETURN_NOT_INIT() do {\
	if(!s_usb_driver_init_flag) {\
			return BK_FAIL;\
		}\
	} while(0)


#define USB_DRIVER_RETURN_NOT_DEINIT() do {\
	if(s_usb_driver_init_flag) {\
			return BK_FAIL;\
		}\
	} while(0)

#define USB_RETURN_NOT_POWERED_ON() do {\
		if(!s_usb_power_on_flag) {\
			return BK_ERR_USB_NOT_POWER;\
		}\
	} while(0)


#define USB_RETURN_NOT_POWERED_DOWN() do {\
		if(s_usb_power_on_flag) {\
			return BK_ERR_USB_NOT_POWER;\
		}\
	} while(0)


#define USB_RETURN_NOT_OPENED() do {\
		if(!s_usb_open_close_flag) {\
			return BK_ERR_USB_NOT_OPEN;\
		}\
	} while(0)

#define USB_RETURN_NOT_CLOSED() do {\
		if(s_usb_open_close_flag) {\
			return BK_ERR_USB_NOT_CLOSE;\
		}\
	} while(0)

static uint32_t bk_usb_set_config(bk_usb_driver_comprehensive_ops *usb_driver)
{
	return BK_FAIL;
}

static void bk_usb_init_all_device_driver_sw(void)
{
#if (CONFIG_USB_HUB && (!CONFIG_USBH_ONLY_USE_THE_ROOTHUB_PORT_ENABLE))
	extern void usbh_hub_class_register();
	usbh_hub_class_register();
#endif

#if CONFIG_USB_UVC
	extern void usbh_uvc_class_register();
	usbh_uvc_class_register();
#endif
#if CONFIG_USB_UAC
	extern void usbh_uac_class_register();
	usbh_uac_class_register();
#endif

#if CONFIG_USB_MSD
	extern void usbh_msc_register();
	usbh_msc_register();
#endif

#if CONFIG_USB_CDC
	extern void usbh_cdc_acm_class_register();
	usbh_cdc_acm_class_register();
#endif
}

static void bk_usb_deinit_all_device_driver_sw(void)
{

}

bk_err_t bk_usb_power_ops(uint32_t gpio_id, bool ops)
{
	if (ops)
	{
		USB_RETURN_NOT_POWERED_DOWN();
		USB_DRIVER_LOGI("%s gpio_id:%d ops:%d s_usb_power_on_flag:%d\r\n", __func__,gpio_id, ops, s_usb_power_on_flag);
		bk_gpio_ctrl_external_ldo(GPIO_CTRL_LDO_MODULE_USB, gpio_id, GPIO_OUTPUT_STATE_HIGH);
		s_usb_power_on_flag = ops;
	}
	else
	{
		USB_DRIVER_LOGD("%s s_usb_power_on_flag:%d\r\n", __func__, s_usb_power_on_flag);
		USB_RETURN_NOT_POWERED_ON();
		bk_gpio_ctrl_external_ldo(GPIO_CTRL_LDO_MODULE_USB, gpio_id, GPIO_OUTPUT_STATE_LOW);
		USB_DRIVER_LOGD("%s setup.last POWER DOWN\r\n", __func__);
		s_usb_power_on_flag = ops;
	}

	return BK_OK;
}

bk_err_t bk_usb_driver_init(void)
{
	USB_DRIVER_RETURN_NOT_DEINIT();
	USB_DRIVER_LOGD("[+]%s\r\n",__func__);

	bk_usb_init_all_device_driver_sw();

#if CONFIG_USB_MAILBOX_SLAVE
	bk_usb_mailbox_sw_slave_init();
#endif
	s_usb_driver_init_flag = 1;

	USB_DRIVER_LOGD("[-]%s\r\n",__func__);

	return BK_OK;
}

bk_err_t bk_usb_driver_deinit(void)
{
	USB_DRIVER_RETURN_NOT_INIT();

	sys_drv_int_disable(USB_INTERRUPT_CTRL_BIT);
	bk_int_isr_unregister(INT_SRC_USB);
	sys_drv_dev_clk_pwr_up(CLK_PWR_ID_USB_1, CLK_PWR_CTRL_PWR_DOWN);
	bk_usb_deinit_all_device_driver_sw();
	s_usb_driver_init_flag = 0;
	return BK_OK;
}

extern void delay(INT32 num);
static void bk_usb_host_custom_register_set()
{
	REG_USB_USR_SOFT_RESETEN &= ~(R708_USB_USR_SOFT_RESETN);
	REG_USB_USR_CONFIG &= ~(R710_USB_USR_RESET);
	delay(100);

	uint32_t config_reg = 0;
    config_reg = R710_USB_USR_TML | R710_USB_USR_CFG_RSTN | R710_USB_USR_REFCLK_MODE | 
                 R710_USB_USR_PLL_EN | R710_USB_USR_DATA_BUSL6_8 | R710_USB_USR_OTG_SUSPENDM |
                 R710_USB_USR_ID_DIG_SEL | R710_USB_USR_OTG_AVALID_REG | R710_USB_USR_OTG_AVALID_SEL |
				 R710_USB_USR_OTG_VBUSVALID_REG | R710_USB_USR_OTG_VBUSVALID_SEL | R710_USB_USR_OTG_SESSEND_SEL;
	config_reg &= ~R710_USB_USR_OTG_SESSEND_REG;
	REG_USB_USR_CONFIG = config_reg;

	REG_USB_USR_CONFIG |= R710_USB_USR_RESET;
	REG_USB_USR_SOFT_RESETEN |=	R708_USB_USR_SOFT_RESETN;
}

static void bk_usb_device_custom_register_set()
{
	REG_USB_USR_SOFT_RESETEN &= ~(R708_USB_USR_SOFT_RESETN);
	REG_USB_USR_CONFIG &= ~(R710_USB_USR_RESET);
    //REG_USB_USR_CONFIG |= (0x0<< 0);
	uint32_t config_reg = 0;
    config_reg = R710_USB_USR_REFCLK_MODE | R710_USB_USR_PLL_EN | R710_USB_USR_RESET|
	             R710_USB_USR_DATA_BUSL6_8 | R710_USB_USR_OTG_SUSPENDM | R710_USB_USR_ID_DIG_REG |
                 R710_USB_USR_ID_DIG_SEL | R710_USB_USR_OTG_AVALID_REG | R710_USB_USR_OTG_AVALID_SEL |
				 R710_USB_USR_OTG_VBUSVALID_REG | R710_USB_USR_OTG_VBUSVALID_SEL | R710_USB_USR_OTG_SESSEND_SEL;
	config_reg &= ~R710_USB_USR_OTG_SESSEND_REG;
	REG_USB_USR_CONFIG = config_reg;

    REG_USB_USR_SOFT_RESETEN |=	R708_USB_USR_SOFT_RESETN;
}

static void bk_analog_layer_usb_sys_related_ops(uint32_t usb_mode, bool ops)
{
	extern void delay(INT32 num);

	if(ops){
		sys_drv_usb_clock_ctrl(true, NULL);
		delay(100);

		sys_drv_usb_analog_dp_capability(USB_DP_CAPABILITY_VALUE);
		sys_drv_usb_analog_dn_capability(USB_DN_CAPABILITY_VALUE);
		if(!sys_hal_psram_ldo_status()) {
			sys_drv_psram_ldo_enable(1);
		}
		sys_drv_usb_analog_phy_en(1, NULL);

		if(usb_mode == USB_HOST_MODE) {
			bk_usb_host_custom_register_set();
		} else {
			bk_usb_device_custom_register_set();
		}
	} else {
		sys_drv_usb_analog_phy_en(0, NULL);
		sys_drv_usb_clock_ctrl(false, NULL);
	}
}

void bk_usb_phy_register_refresh()
{
#if CONFIG_USB_HOST
	sys_drv_int_disable(USB_INTERRUPT_CTRL_BIT);
	bk_gpio_set_output_low(CONFIG_USB_VBAT_CONTROL_GPIO_ID);
	bk_analog_layer_usb_sys_related_ops(USB_HOST_MODE, false);
	bk_analog_layer_usb_sys_related_ops(USB_HOST_MODE, true);
	extern int usb_hc_mhdrc_register_init(void);
	usb_hc_mhdrc_register_init();
	sys_drv_int_enable(USB_INTERRUPT_CTRL_BIT);
	if(s_usb_power_on_flag) {
		bk_gpio_set_output_high(CONFIG_USB_VBAT_CONTROL_GPIO_ID);
	}
#endif

}

void bk_usb_enum_fail_all_reg_reset()
{
#if CONFIG_USB_HOST
	sys_drv_int_disable(USB_INTERRUPT_CTRL_BIT);
	bk_gpio_set_output_low(CONFIG_USB_VBAT_CONTROL_GPIO_ID);
	rtos_delay_milliseconds(10);
	bk_analog_layer_usb_sys_related_ops(USB_HOST_MODE, false);
	rtos_delay_milliseconds(10);
	bk_analog_layer_usb_sys_related_ops(USB_HOST_MODE, true);
	rtos_delay_milliseconds(10);
	extern int usb_hc_mhdrc_register_init(void);
	usb_hc_mhdrc_register_init();
	sys_drv_int_enable(USB_INTERRUPT_CTRL_BIT);
	bk_gpio_set_output_high(CONFIG_USB_VBAT_CONTROL_GPIO_ID);
#endif

}

void bk_usb_device_set_using_status(bool use_or_no, E_USB_DEVICE_T dev)
{

}

bk_err_t bk_usb_drv_send_msg(int op, void *param)
{
	bk_err_t ret;
	bk_usb_drv_msg_t msg;

	msg.op = op;
	msg.param = param;
	if (s_usb_drv_msg_que) {
		ret = rtos_push_to_queue(&s_usb_drv_msg_que, &msg, BEKEN_NO_WAIT);
		if (kNoErr != ret) {
			USB_DRIVER_LOGE("usb_driver_send_msg fail ret:%d op:%d\r\n", ret, op);
			return BK_FAIL;
		}

		return ret;
	}
	return BK_OK;
}

bk_err_t bk_usb_drv_send_msg_front(int op, void *param)
{
	bk_err_t ret;
	bk_usb_drv_msg_t msg;

	msg.op = op;
	msg.param = param;
	if (s_usb_drv_msg_que) {
		ret = rtos_push_to_queue_front(&s_usb_drv_msg_que, &msg, BEKEN_NO_WAIT);
		if (kNoErr != ret) {
			USB_DRIVER_LOGE("%s fail \r\n", __func__);
			return BK_FAIL;
		}

		return ret;
	}
	return BK_OK;
}

void bk_usb_driver_task_lock_mutex()
{
	if(s_usb_drv_task_mutex)
		rtos_lock_mutex(&s_usb_drv_task_mutex);
}

void bk_usb_driver_task_unlock_mutex()
{
	if(s_usb_drv_task_mutex)
		rtos_unlock_mutex(&s_usb_drv_task_mutex);
}

static void usb_driver_connect_handle(bk_usb_driver_comprehensive_ops *usb_driver)
{
	USB_DRIVER_LOGD("[+]%s\r\n",__func__);
	s_usbh_device_connect_flag = true;

	for(E_USB_DEVICE_T dev = USB_UVC_DEVICE; dev < USB_DEVICE_MAX; dev++)
	{
		if(usb_driver->usbh_connect_cb[dev]) {
			usb_driver->usbh_class_connect_status |= (1 << dev);
			usb_driver->usbh_connect_cb[dev]();
		}
	}

	USB_DRIVER_LOGD("[-]%s\r\n",__func__);
}

static void usb_driver_disconnect_handle(bk_usb_driver_comprehensive_ops *usb_driver)
{
	USB_DRIVER_LOGD("[+]%s\r\n",__func__);
	s_usbh_device_connect_flag = false;
	usb_driver->usbh_class_start_status = 0x0;
	for(E_USB_DEVICE_T dev = USB_UVC_DEVICE; dev < USB_DEVICE_MAX; dev++)
	{
		if(usb_driver->usbh_disconnect_cb[dev]) {
			usb_driver->usbh_class_connect_status &= ~(1 << dev);
			usb_driver->usbh_disconnect_cb[dev]();
		}
	}

	USB_DRIVER_LOGD("[-]%s\r\n",__func__);
}

static void usb_drv_task_main(beken_thread_arg_t param_data)
{
	bk_err_t ret = kNoErr;

	while(1) {
		bk_usb_drv_msg_t msg;
		ret = rtos_pop_from_queue(&s_usb_drv_msg_que, &msg, BEKEN_WAIT_FOREVER);
		if (kNoErr == ret) {
			switch (msg.op) {
				case USB_DRV_IDLE:
					break;
				case USB_DRV_EXIT:
					USB_DRIVER_LOGI("USB_DRV_EXIT!\r\n");
					rtos_reset_queue(&s_usb_drv_msg_que);
#if CONFIG_USB_MAILBOX_SLAVE
					usb_mailbox_slave_usb_driver_exit();
#endif
					s_usb_open_close_flag = 0;
					s_usbh_device_connect_flag = false;
					s_usb_driver_ops = NULL;
#if CONFIG_SYS_CPU0
					bk_pm_module_vote_sleep_ctrl(PM_SLEEP_MODULE_NAME_USB_1, 0x1, 100000000);
#endif
					goto usb_driver_exit;
					break;

				case USB_DRV_PRASE_DESCRIPTOR:
					USB_DRIVER_LOGI("USB_DRV_PRASE_DESCRIPTOR!\r\n");
					s_usb_driver_ops = (bk_usb_driver_comprehensive_ops *)msg.param;
					break;

				case USB_DRV_INIT_CONTROL_TRANS:
					break;
				case USB_DRV_DEINIT_CONTROL_TRANS:
					break;
				case USB_DRV_CONTROL_TRANSFER:
					break;
				case USB_DRV_CONNECT_HANDLE:
					break;
				case USB_DRV_DISCONNECT_HANDLE:
					USB_DRIVER_LOGI("USB_DRV_DISCONNECT_HANDLE!\r\n");
					if(!bk_usb_get_device_connect_status()) break;
					bk_usb_driver_task_lock_mutex();
					usb_driver_disconnect_handle((bk_usb_driver_comprehensive_ops *)msg.param);
					rtos_reset_queue(&s_usb_drv_msg_que);

#if CONFIG_USB_CDC_ACM_DEMO
					bk_usb_cdc_exit();
#endif
					bk_usb_phy_register_refresh();
					bk_usb_driver_task_unlock_mutex();
					break;

				case USB_DRV_USB_OPEN:
					USB_DRIVER_LOGI("USB_DRV_USB_OPEN!\r\n");
					if(*((uint32_t *)msg.param) == USB_HOST_MODE) {
#if CONFIG_USB_HOST	
						bk_analog_layer_usb_sys_related_ops(USB_HOST_MODE, true);
						usbh_initialize();
						USB_DRIVER_LOGI("USB HOST\r\n");
#endif
					} else if(*((uint32_t *)msg.param) == USB_DEVICE_MODE){
#if CONFIG_USB_DEVICE
						bk_analog_layer_usb_sys_related_ops(USB_DEVICE_MODE, true);
						usbd_initialize();
						USB_DRIVER_LOGI("USB DEVICE\r\n");
#endif
					} else
						USB_DRIVER_LOGI("PLEASE check USB mode\r\n");

					s_usb_open_close_flag = 1;
					break;
				case USB_DRV_USB_CLOSE:
					USB_DRIVER_LOGI("USB_DRV_USB_CLOSE!\r\n");
					if(!s_usb_open_close_flag) break;

					sys_drv_int_disable(USB_INTERRUPT_CTRL_BIT);
					if(bk_usb_get_device_connect_status()) {
#if CONFIG_USB_CDC_ACM_DEMO
						bk_usb_cdc_free_enumerate_resources();
#endif
					}

#if CONFIG_USB_HOST
					usbh_deinitialize();
					bk_analog_layer_usb_sys_related_ops(USB_HOST_MODE, false);
#endif
#if CONFIG_USB_DEVICE
					usbd_deinitialize();
					bk_analog_layer_usb_sys_related_ops(USB_DEVICE_MODE, false);
#endif
					bk_usb_drv_send_msg_front(USB_DRV_EXIT, NULL);
					break;

				default:
					break;
			}
		}
	}

usb_driver_exit:
	usb_driver_sw_deinit();
}

static bk_err_t usb_driver_sw_init()
{
	uint32_t ret = BK_OK;

	if ((!s_usb_drv_thread_hdl) && (!s_usb_drv_msg_que)) {

		if(!s_usb_drv_task_mutex)
			rtos_init_mutex(&s_usb_drv_task_mutex);

		ret = rtos_init_queue(&s_usb_drv_msg_que,
							  "usb_driver_queue",
							  sizeof(bk_usb_drv_msg_t),
							  USB_DRIVER_QITEM_COUNT);
		if (ret != kNoErr) {
			USB_DRIVER_LOGE("ceate usb driver internal message queue fail \r\n");
			return BK_FAIL;
		}

		//create usb driver task
		ret = rtos_create_thread(&s_usb_drv_thread_hdl,
							 CONFIG_TASK_USB_PRIO,
							 "usb_driver",
							 (beken_thread_function_t)usb_drv_task_main,
							 CONFIG_USB_DRIVER_TASK_SIZE,
							 NULL);
		if (ret != kNoErr) {
			USB_DRIVER_LOGE("create usb driver task fail \r\n");
			rtos_deinit_queue(&s_usb_drv_msg_que);
			s_usb_drv_msg_que = NULL;
			s_usb_drv_thread_hdl = NULL;
		}
		USB_DRIVER_LOGD("create usb driver task complete \r\n");

	} else {
		return BK_FAIL;
	}

	return BK_OK;
}

static bk_err_t usb_driver_sw_deinit()
{
	beken_mutex_t usb_drv_task_mutex = NULL;
	beken_queue_t usb_drv_msg_que = NULL;

	if(s_usb_drv_task_mutex) {
		usb_drv_task_mutex = s_usb_drv_task_mutex;
		s_usb_drv_task_mutex = NULL;
		rtos_deinit_mutex(&usb_drv_task_mutex);
	}

	if(s_usb_drv_msg_que) {
		usb_drv_msg_que = s_usb_drv_msg_que;
		s_usb_drv_msg_que = NULL;
		rtos_deinit_queue(&usb_drv_msg_que);
	}

	if (s_usb_drv_thread_hdl) {
		s_usb_drv_thread_hdl = NULL;
		rtos_delete_thread(NULL);
	}

	return BK_OK;
}

bk_err_t bk_usb_open(uint32_t usb_mode)
{
	USB_DRIVER_LOGD("[+]%s\r\n", __func__);
#if !CONFIG_USB_MAILBOX
	bk_pm_module_vote_cpu_freq(PM_DEV_ID_USB_1, PM_CPU_FRQ_480M);
	if((s_usbh_device_connect_flag != false) && (s_usb_driver_ops != NULL)) {
		for(E_USB_DEVICE_T dev = USB_UVC_DEVICE; dev < USB_DEVICE_MAX; dev++)
		{
			if(s_usb_driver_ops->usbh_connect_cb[dev]) {
				s_usb_driver_ops->usbh_class_connect_status |= (1 << dev);
				s_usb_driver_ops->usbh_connect_cb[dev]();
			}
		}
	}
#endif

	bk_pm_module_vote_power_ctrl(PM_POWER_SUB_MODULE_NAME_AHBP_USB, PM_POWER_MODULE_STATE_ON);
#if CONFIG_SYS_CPU0
	bk_pm_module_vote_sleep_ctrl(PM_SLEEP_MODULE_NAME_USB_1, 0x0, 0x0);
#endif
	USB_DRIVER_RETURN_NOT_INIT();
	//USB_RETURN_NOT_POWERED_ON();
	USB_RETURN_NOT_CLOSED();

	static uint32_t usb_ip_mode = USB_HOST_MODE;
	usb_ip_mode = usb_mode;

	if(usb_driver_sw_init() == BK_OK) {
		USB_DRIVER_LOGD("usb driver sw init OK\r\n");
	} else {
		USB_DRIVER_LOGE("usb driver sw init ERROR\r\n");
		return BK_FAIL;
	}

	bk_usb_drv_send_msg_front(USB_DRV_USB_OPEN, (void *)&usb_ip_mode);

	USB_DRIVER_LOGD("[-]%s\r\n", __func__);

	return BK_OK;
}

bk_err_t bk_usb_close(void)
{
	USB_DRIVER_RETURN_NOT_INIT();
	USB_RETURN_NOT_OPENED();

	bk_err_t ret = BK_OK;
	ret = bk_usb_drv_send_msg(USB_DRV_USB_CLOSE, NULL);

#if !CONFIG_USB_MAILBOX
	uint8_t wait_close_finish_count = 0;
	while(s_usb_open_close_flag) {
		if(wait_close_finish_count > 10)
		{
			USB_DRIVER_LOGE("[=]%s close timeout\r\n", __func__);
			ret = BK_FAIL;
			break;
		}
		rtos_delay_milliseconds(10);
		wait_close_finish_count++;
	}
#endif
	bk_pm_module_vote_power_ctrl(PM_POWER_SUB_MODULE_NAME_AHBP_USB, PM_POWER_MODULE_STATE_OFF);
	return ret;

}

bk_err_t bk_usb_register_connection_status_notification_callback(bk_usb_driver_comprehensive_ops *usb_driver,
																										E_USB_CONNECTION_STATUS status,
																										E_USB_DEVICE_T dev,
																										void *callback)
{
	if(status == USB_DEVICE_CONNECT) {
		usb_driver->usbh_connect_cb[dev] = (FUNCPTR)callback;
	}
	if(status == USB_DEVICE_DISCONNECT) {
		usb_driver->usbh_disconnect_cb[dev] = (FUNCPTR)callback;
	}

	return BK_OK;

}

bk_err_t bk_usb_check_device_supported (E_USB_DEVICE_T usb_dev)
{
	return BK_FAIL;
}

bk_err_t bk_usb_get_device_descriptor(struct usb_device_descriptor_t *dev_descriptor)
{

    return BK_FAIL;
}

bool bk_usb_get_device_connect_status(void)
{
    return 0;
}

