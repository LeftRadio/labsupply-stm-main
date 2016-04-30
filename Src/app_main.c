/**
  ******************************************************************************
  * @file
  * @author  LeftRadio
  * @version V1.0.0
  * @date
  * @brief
  ******************************************************************************
**/

/* Includes ------------------------------------------------------------------*/
#include "stm32f1xx_hal.h"
#include "adc.h"
#include "dac.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"

#include "max7301.h"
#include "encoder.h"
#include "channels.h"


/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private typedef -----------------------------------------------------------*/
typedef enum { HW_NORMAL = 0, HW_SETTINGS = 1 } HW_WorkMode_TypeDef;

typedef struct {
  HW_Channel_TypeDef* channel;
  HW_WorkMode_TypeDef work_mode;
  HW_RegMode_TypeDef reg_channel;
  HW_ControlType_TypeDef control_lim_value;
  uint8_t blocked;
} HW_App_TypeDef;

/* Private variables ---------------------------------------------------------*/
HW_App_TypeDef app = {
  &hw_channel_self,
  HW_NORMAL,
  HW_SELF_CH,
  HW_CONTROL_V,
  0
};

__IO uint32_t segments_mask = 0x00000000;

/* Private function prototypes ----------------------------------------------*/
/* Functions ----------------------------------------------------------------*/



/**
  * @brief  Display_SetValue
  * @param  None
  * @retval None
  */
static void Display_SetValue(uint16_t val, uint8_t pos)
{
    uint8_t data[3] = { 0x00, 0x00, 0x00 };

    while(val >= 100){ val -= 100; data[0]++; }
    while(val >= 10){ val -= 10; data[1]++; }
    data[2] = val;

    for (uint8_t i = 0; i < 3; i++) {
        MAX7301_SetData( data[i], (pos * 3) + i );
    }
}

/**
  * @brief  Display_SetLeds
  * @param  None
  * @retval None
  */
static void Display_SetLeds(uint8_t clrbits, uint8_t setbits)
{
    __IO uint8_t leds_state = MAX7301_GetData(12);

    leds_state |= clrbits;
    leds_state &= (~setbits);

    MAX7301_SetData(leds_state, 12);
}

/**
  * @brief  Set_ChannelsLeds
  * @param  None
  * @retval None
  */
static void Set_ChannelsLeds(void)
{
	static const uint8_t reg_ch_leds[3] = { 0x40, 0x01, 0x40 | 0x01 };
	Display_SetLeds( reg_ch_leds[2], reg_ch_leds[app.reg_channel] );
}

/**
  * @brief  Set_ChannelsLeds
  * @param  None
  * @retval None
  */
static void Channels_UpdateBlynkMask(uint8_t iindx)
{
	segments_mask = 0;
	MAX7301_SetMask(0xFFFFFFFF);

	if (app.reg_channel == HW_ALL_CH) {
		segments_mask |= 0x07 << iindx;
    segments_mask |= 0x07 << (6 + iindx);
	}
	else if (app.reg_channel == HW_SELF_CH) {
		segments_mask |= 0x07 << iindx;
	}
  else {
    segments_mask |= 0x07 << (6 + iindx);
  }
}

/**
  * @brief  Display_Init
  * @param  None
  * @retval None
  */
static void Display_Init(void)
{
	for (uint8_t i = 0; i < 4; i++) {
		Display_SetValue(0, i);
	}
	/* reset all leds */
	Display_SetLeds(0xFF, 0x00);
	/* set ON state for active channel/channels led  */
	Set_ChannelsLeds();
}



/**
  * @brief  SetWorkMode
  * @param  None
  * @retval None
  */
static void SetWorkMode(void)
{
  uint8_t workmode_led = 0x00;

  if(++app.work_mode > HW_SETTINGS) {
    app.work_mode = HW_NORMAL;
  }
  else {
    workmode_led = 0x10;
  }

  Display_SetLeds( 0x10, workmode_led );

  if(app.control_lim_value == HW_CONTROL_V) Channels_UpdateBlynkMask(0);
  else Channels_UpdateBlynkMask(3);
}

/**
  * @brief  SwitchRegChannel
  * @param  None
  * @retval None
  */
static void SwitchRegChannel(int8_t sign)
{
  app.reg_channel += sign;

  if ( (app.reg_channel > HW_ALL_CH) || ((uint8_t)app.reg_channel < 0) ) {
		app.reg_channel = HW_SELF_CH;
    app.channel = &hw_channel_self;
	}
  else if (app.reg_channel == HW_EXTERN_CH) {
    app.channel = &hw_channel_external;
  }
}

/**
  * @brief  SwitchLimitValue
  * @param  None
  * @retval None
  */
static void SwitchLimitValue(void)
{
	uint8_t iindx = 0;

	if(app.control_lim_value == HW_CONTROL_V) {
		app.control_lim_value = HW_CONTROL_I;
		iindx = 3;
	}
	else {
		app.control_lim_value = HW_CONTROL_V;
	}

	Channels_UpdateBlynkMask(iindx);
}


/**
  * @brief Channel_SetLimits
  * @param
  * @retval None
  */
static void Channel_SetLimits(HW_Channel_TypeDef* channel, HW_ControlType_TypeDef limtype, int8_t sign)
{
  if ( limtype == HW_CONTROL_V ) {
    channel->lim.v += sign;
    if (channel->lim.v < CH_V_LIM_MIN) channel->lim.v = CH_V_LIM_MIN;
    else if (channel->lim.v > CH_V_LIM_MAX) channel->lim.v = CH_V_LIM_MAX;
  }
  else {
    channel->lim.i += sign;
    if (channel->lim.i < CH_I_LIM_MIN) channel->lim.i = CH_I_LIM_MIN;
    else if (channel->lim.i > CH_I_LIM_MAX) channel->lim.i = CH_I_LIM_MAX;
  }

  channel->UpdateLimits();
}

/**
  * @brief  SwitchChannelValue
  * @param  None
  * @retval None
  */
static void SwitchChannelValue(int8_t sign)
{
    if( app.reg_channel == HW_ALL_CH ) {

      if( app.work_mode == HW_NORMAL ) {
        hw_channel_self.SetOut(sign);
        hw_channel_external.SetOut(sign);
      }
      else {
        Channel_SetLimits(&hw_channel_self, app.control_lim_value, sign);
        Channel_SetLimits(&hw_channel_external, app.control_lim_value, sign);
      }
    }
    else {

      if( app.work_mode == HW_NORMAL ) {
        app.channel->SetOut(sign);
      }
      else {
        Channel_SetLimits(app.channel, app.control_lim_value, sign);
      }
    }
}

/**
  * @brief  UpdateBlockOutState
  * @param  None
  * @retval None
  */
static void UpdateBlockOutState(uint8_t reset)
{
	if(hw_channel_self.hard_protect_state) {
		if(reset) {
			hw_channel_self.ProtectionReset();
		}
		else app.blocked = 1;

		Display_SetLeds(0x00, 0x20);
	}
	else {
		Display_SetLeds(0x20, 0x00);
	}
	if (hw_channel_external.hard_protect_state) {
		if(reset) {
			hw_channel_external.ProtectionReset();
		}
		else app.blocked = 1;

		Display_SetLeds(0x00, 0x02);
	}
	else {
		Display_SetLeds(0x02, 0x00);
	}
}


/**
  * @brief  Application_Configurate
  * @param  None
  * @retval None
  */
static void Application_Configurate(void)
{
	__IO uint16_t dval = 0;

	/* Calibrate and start ADC for U/I measurmenst */
	HAL_ADCEx_Calibration_Start(&hadc1);

	/* Set init dac out value and start DAC, used for I protection based on hardware comparator */
	HAL_DAC_Start(&hdac, DAC_CHANNEL_1);
	HAL_DAC_SetValue(&hdac, DAC_CHANNEL_1, DAC_ALIGN_12B_R, 4095);

	/* Start TIM3 PWM generation for control output V */
	HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
	htim3.Instance->CCR1 = 0;

	/* Enable SPI peripheral */
	__HAL_SPI_ENABLE(&hspi1);

	/* MAX7301 configurate */
	MAX7301_Configurate();

	/* Finally init 7-segments indication and control leds state and start update timer */
	HAL_TIM_Base_Start_IT(&htim2);
	Display_Init();
}

/**
  * @brief  app_main
  * @param  None
  * @retval None
  */
void ApplicationMain(void)
{
	static long block_cnt = 0;

	/* set application init state */
	Application_Configurate();

	/* application main inf work cycle */
	while(1) {

		HW_Encoder_TypeDef* encoder = GetEncoder();

		if( encoder->direction == ENC_DIR_LEFT ) {
			SwitchChannelValue(-1);
			encoder->direction = ENC_STOP;
		}
		else if( encoder->direction == ENC_DIR_RIGHT ) {
			SwitchChannelValue(+1);
			encoder->direction = ENC_STOP;
		}

		if( encoder->button == ENC_BTN_SHORT ) {
			if(app.work_mode == HW_NORMAL) {
				SwitchRegChannel(+1);
				/* set ON state for active channel/channels led  */
				Set_ChannelsLeds();
			}
			else {
				SwitchLimitValue();
			}
			encoder->button = ENC_BTN_OFF;
		}
		else if( encoder->button == ENC_BTN_LONG ) {
			SetWorkMode();
			encoder->button = ENC_BTN_OFF;
		}

		if (app.blocked == 1) {
			if(++block_cnt >= 10000000) {
				UpdateBlockOutState(1); // reset hard protection state
				app.blocked = block_cnt = 0;
			}
		}
	}
}


/**
  * @brief  MAX7320_UpdateCallback
  * @param  None
  * @retval None
  */
void MAX7320_UpdateCallback(void)
{
	if(app.work_mode == HW_NORMAL) {
		hw_channel_self.GetOut();
		hw_channel_external.GetOut();
	}
	else {
		hw_channel_self.data.v = hw_channel_self.lim.v;
		hw_channel_self.data.i = hw_channel_self.lim.i;
		hw_channel_external.data.v = hw_channel_external.lim.v;
		hw_channel_external.data.i = hw_channel_external.lim.i;
		HW_Channel_GetDataCallback(&hw_channel_self);
		HW_Channel_GetDataCallback(&hw_channel_external);
	}
	/* blynk for dispay segments mask */
	uint32_t mask = MAX7301_GetMask();

	if (app.work_mode == HW_SETTINGS) {
		mask ^= segments_mask;
	}
	MAX7301_SetMask(mask);
}

/**
  * @brief  Channel collect out data complite event callback.
  * @param  channel: pointer to a HW_Channel_TypeDef structure
  * @retval None
  */
void HW_Channel_GetDataCallback(HW_Channel_TypeDef* channel)
{
	uint8_t ch_disp_pos = (channel == &hw_channel_self)? 0 : 2;

	Display_SetValue((uint16_t)(channel->data.v), ch_disp_pos);
	Display_SetValue((uint16_t)(channel->data.i), ch_disp_pos+1);

	UpdateBlockOutState(0);
}

/**
  * @brief  EXTI line detection callback
  * @param GPIO_Pin: Specifies the pins connected EXTI line
  * @retval None
  */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	GPIOB->BSRR = (uint32_t)GPIO_PIN_11 << 16;
	hw_channel_self.hard_protect_state = 1;
}
