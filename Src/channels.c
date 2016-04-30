/**
  ******************************************************************************
  * @file       channels.c
  * @author     Neil Lab :: Left Radio
  * @version    v1.0.0
  * @date
  * @brief
  ******************************************************************************
**/

/* Includes ------------------------------------------------------------------*/
#include "stm32f1xx_hal.h"
#include "adc.h"
#include "dac.h"
#include "tim.h"
#include "usart.h"
#include "channels.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define ADC_SAMPLES_CNT             ((uint8_t)20)




/* Private macro -------------------------------------------------------------*/
/* Extern function -----------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
static void GetOut_Self(void);
static void SetOut_Self(int8_t sign);
static void UpdateLimits_Self(void);
static void ProtectionReset_Self(void);

static void GetOut_Ex(void);
static void SetOut_Ex(int8_t sign);
static void UpdateLimits_Ex(void);
static void ProtectionReset_Ex(void);

/* Private variables ---------------------------------------------------------*/
HW_Channel_TypeDef hw_channel_self = {
  { 0, 0 },   	// V/I data
  { 300, 250 },   // V/I limits
  0,			// hard protection state
  GetOut_Self,
  SetOut_Self,
  UpdateLimits_Self,
  ProtectionReset_Self
};

HW_Channel_TypeDef hw_channel_external = {
  { 0, 0 },   	// V/I data
  { 300, 250 },   // V/I limits
  0,			// hard protection state
  GetOut_Ex,
  SetOut_Ex,
  UpdateLimits_Ex,
  ProtectionReset_Ex
};

uint32_t adc_sampled_data[ADC_SAMPLES_CNT];
HW_Data_TypeDef adc_sum = {0, 0};


/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Channel collect out data complite event callback.
  * @param  channel: pointer to a HW_Channel_TypeDef structure
  * @retval None
  */
__weak void HW_Channel_GetDataCallback(HW_Channel_TypeDef* channel)
{
  /*    This function Should not be modified, when the callback is needed,
          the HW_Channel_GetDataCallback could be implemented in the user file.
   */
}





///**
//  * @brief Channel_ControlLimits
//  * @param
//  * @retval None
//  */
//void Channel_ControlLimits(void)
//{
//	HAL_DAC_SetValue(&hdac, DAC_CHANNEL_1, DAC_ALIGN_12B_R, 4095);
//}

/**
  * @}
  */

/** @defgroup   HW_Channel_TypeDef hw_channel_external group
  * @brief      Self hardware channel functions
  *
@verbatim
 ===============================================================================
                 ##### Self hardware channel functions #####
 ===============================================================================

 [..] This section provides functions implementing get/set out values for self channel

@endverbatim
  * @{
  */

/**
  * @brief  Calc_ADC_UI
  * @param
  * @retval None
  */
static __inline void Calc_ADC_UI(uint8_t start, uint8_t end)
{
	for(int i = start; i < end; i++) {
		adc_sum.v += adc_sampled_data[i*2];
		adc_sum.i += adc_sampled_data[(i*2)+1];
	}
}

/**
  * @brief  HAL_ADC_ConvCpltCallback
  *     Get U/I output values
  * @param  ADC Handle
  * @retval None
  */
void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef* hadc)
{
  adc_sum.v = 0;
  adc_sum.i = 0;
  Calc_ADC_UI(0, ADC_SAMPLES_CNT/4);
}

/**
  * @brief  HAL_ADC_ConvCpltCallback
  *     Get U/I output values
  * @param  ADC Handle
  * @retval None
  */
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
{
  /* Stop DMA */
  HAL_ADC_Stop_DMA(&hadc1);
  /**/
  Calc_ADC_UI(ADC_SAMPLES_CNT/4, ADC_SAMPLES_CNT/2);

  /* Convert U/I values */
  float U = ( (float)adc_sum.v * 32.2F * 13.48F ) / 4095.0F;  // Convert, value = [ (adcdata * vdda * k) / 2^12 ], were k = 12k/1k + 1 = 13
  float I = ( ((float)adc_sum.i * 322.0F * (1.0F/0.047F) ) / (4095.0F * 9.2F) );  // Convert, value to mA

  hw_channel_self.data.v = (uint16_t)( U / (float)(ADC_SAMPLES_CNT/2) );
  hw_channel_self.data.i = (uint16_t)( I / (float)(ADC_SAMPLES_CNT/2) ) - 1;

  /* Complite data collection for channel callback */
  HW_Channel_GetDataCallback(&hw_channel_self);
}

/**
  * @brief  GetOut_Self
  * @param  None
  * @retval None
  */
static void GetOut_Self(void)
{

  HAL_ADC_Start_DMA(&hadc1, (uint32_t*)&adc_sampled_data[0], ADC_SAMPLES_CNT);
}

/**
  * @brief  SetOut_Self
  * @param  None
  * @retval None
  */
static void SetOut_Self(int8_t sign)
{
  int32_t pwm = htim3.Instance->CCR1;

  if (hw_channel_self.data.v > hw_channel_self.lim.v) sign = -1;

  pwm += sign * 10;
  if(pwm < 0) pwm = 0;
  else if(pwm >= 3400) pwm = 3399;

  htim3.Instance->CCR1 = (uint16_t)pwm;
}

/**
  * @brief  UpdateLimits_Self
  * @param  None
  * @retval None
  */
static void UpdateLimits_Self(void)
{
	// Nothing for limit V, out V controlled in SetOut_Self

	// Set DAC reference for limit I, used hardware comparator
	float I_Amps = (float)hw_channel_self.lim.i / 100.0F;
	uint16_t dac_data = (uint16_t)( (4095.0F * (0.047 * I_Amps * 9.2F)) / 3.22F );
	HAL_DAC_SetValue(&hdac, DAC_CHANNEL_1, DAC_ALIGN_12B_R, dac_data);
}

/**
  * @brief  ProtectionReset_Self
  * @param  None
  * @retval None
  */
static void ProtectionReset_Self(void) {
	GPIOB->BSRR = (uint32_t)GPIO_PIN_11;
	hw_channel_self.hard_protect_state = 0;
}




/** @defgroup   HW_Channel_TypeDef hw_channel_external group
  * @brief      External hardware channel functions
  *
@verbatim
 ===============================================================================
                 ##### External hardware channel functions #####
 ===============================================================================

 [..] This section provides functions implementing get/set out values for external channel

@endverbatim
  * @{
  */

__IO uint8_t rx_rdata[20] = { 0x00 };
__IO uint8_t rx_complite = 0;

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	uint8_t csum = 0, i;
	uint8_t len = rx_rdata[1]+2;

	for (i = 0; i < len; i++) csum += rx_rdata[i];

	if( (!rx_complite) && (rx_rdata[0] == 0x5B && rx_rdata[len] == csum) ) {

		switch(rx_rdata[2]) {

			case 0x00:
				break;

			case 0x0A:
				hw_channel_external.data.v = (uint16_t)rx_rdata[3] | ((uint16_t)rx_rdata[4] << 8);
				hw_channel_external.data.i = (uint16_t)rx_rdata[5] | ((uint16_t)rx_rdata[6] << 8);
				hw_channel_external.hard_protect_state = rx_rdata[7];
				HW_Channel_GetDataCallback(&hw_channel_external);
				break;

			case 0x0B:
				break;

			case 0x0C:
				break;

			case 0xEA:
				break;

			default: break;
		}

		rx_complite = 1;
	}
}

/**
  * @brief  GetOut_Self
  * @param  None
  * @retval None
  */
static uint8_t UsartSendPacked(uint8_t* data, uint8_t wlen, uint8_t rlen)
{
  static uint8_t tx_data[10];
  static uint8_t rx_timeout_cnt = 0;

  if(rx_complite || rx_timeout_cnt >= 10) {
	  rx_timeout_cnt = rx_complite = 0;
	  HAL_UART_DMAStop(&huart1);

	  tx_data[0] = 0x5B;
	  tx_data[1] = wlen;

	  tx_data[wlen+3] = tx_data[0] + tx_data[1];
	  for (uint8_t i = 0; i < wlen; i++) {
		  tx_data[i+2] = data[i];
		  tx_data[wlen+2] += data[i];
	  }

	  HAL_UART_Transmit_DMA(&huart1, (uint8_t*)tx_data, wlen+3);
	  HAL_UART_Receive_DMA(&huart1, (uint8_t*)rx_rdata, rlen);
	  return 1;
  }
  else {
	  rx_timeout_cnt++;
  }
  return 0;
}

/**
  * @brief  GetOut_Self
  * @param  None
  * @retval None
  */
static void GetOut_Ex(void)
{
	uint8_t temp = 0x0A;
	UsartSendPacked(&temp, 1, 9);
}

/**
  * @brief  SetOut_Self
  * @param  None
  * @retval None
  */
static void SetOut_Ex(int8_t sign)
{
	static uint8_t data[2] = { 0x0B, 0x00 };
	if (hw_channel_external.data.v > hw_channel_external.lim.v) sign = -1;
	data[1] = (uint8_t)sign;
	UsartSendPacked(data, 2, 5);
}

/**
  * @brief  UpdateLimits_Ex
  * @param  None
  * @retval None
  */
static void UpdateLimits_Ex(void)
{
	uint8_t data[5] = { 0x0C };
	data[1] = (uint8_t)(hw_channel_external.lim.v >> 8);
	data[2] = (uint8_t)hw_channel_external.lim.v;
	data[3] = (uint8_t)(hw_channel_external.lim.i >> 8);
	data[4] = (uint8_t)hw_channel_external.lim.i;
	UsartSendPacked(data, 5, 8);
}

/**
  * @brief  ProtectionReset_Ex
  * @param  None
  * @retval None
  */
static void ProtectionReset_Ex(void) {
	uint8_t temp = 0xEA;
	while( UsartSendPacked(&temp, 1, 4) == 0 );
	hw_channel_external.hard_protect_state = 0;
}




/*********************************************************************************************************
      END FILE
*********************************************************************************************************/
