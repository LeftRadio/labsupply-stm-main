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
#include <string.h>

#include "stm32f1xx_hal.h"
#include "spi.h"
#include "tim.h"
#include "encoder.h"
#include "max7301.h"


/* Private define ------------------------------------------------------------*/
#define MAX_READ_ADDR_BIT                   ((uint8_t)(0x01<<7))

#define MAX_NORMAL_OPERATION_BIT 		    ((uint8_t)0x01)
#define MAX_TRANSITION_DETECT_BIT 		    ((uint8_t)(0x01<<7))

#define MAX_PORT_OUT_PP    				    ((uint8_t)0x55)
#define MAX_PORT_IN_WITH_PU                 ((uint8_t)0xFF)

#define MAX_PIN_OUT_PP                      ((uint8_t)0x01)
#define MAX_PIN_IN_WITH_PU                  ((uint8_t)0x03)

#define MAX_MAINCONFIG_REG                  ((uint8_t)0x04)
#define MAX_CNF_P7_P6_P5_P4_REG             ((uint8_t)0x09)
#define MAX_CNF_P11_P10_P9_P8_REG           ((uint8_t)0x0A)
#define MAX_CNF_P15_P14_P13_P12_REG         ((uint8_t)0x0B)
#define MAX_CNF_P19_P18_P17_P16_REG         ((uint8_t)0x0C)
#define MAX_CNF_P23_P22_P21_P20_REG         ((uint8_t)0x0D)
#define MAX_CNF_P27_P26_P25_P24_REG         ((uint8_t)0x0E)
#define MAX_CNF_P31_P30_P29_P28_REG         ((uint8_t)0x0F)

#define SEGMENTS_DATA_CNF_REG_0             MAX_CNF_P11_P10_P9_P8_REG
#define SEGMENTS_DATA_CNF_REG_1             MAX_CNF_P15_P14_P13_P12_REG

#define SEGMENTS_DATA_REG             		((uint8_t)0x48)

#define SEGMENT_STROBE_CNF_REG_0            MAX_CNF_P19_P18_P17_P16_REG
#define SEGMENT_STROBE_CNF_REG_1            MAX_CNF_P23_P22_P21_P20_REG
#define SEGMENT_STROBE_CNF_REG_2            MAX_CNF_P27_P26_P25_P24_REG

#define MAX_CS_SET_LOW()                    ( MAX_NSS_GPIO_Port->BSRR = (uint32_t)MAX_NSS_Pin << 16 )
#define MAX_CS_SET_HIGHT()                  ( MAX_NSS_GPIO_Port->BSRR = (uint32_t)MAX_NSS_Pin )


/* Private macro -------------------------------------------------------------*/
/* Private typedef -----------------------------------------------------------*/
typedef struct {

    const uint8_t strobe_addrs[MAX_SEGMENTS_COUNT];

    const uint8_t num_codes[12];
    uint8_t dots[12];
    const uint8_t dot_code;

    uint8_t segments_data[15];
    uint32_t segments_mask;
    uint8_t active_segment;

    uint8_t tx_data[50];
    uint8_t rx_data[50];
    uint16_t txrx_sequence_words;
    uint8_t busy;

    uint8_t init_state;

} MAX7301_TypeDef;

/* Private variables ---------------------------------------------------------*/
MAX7301_TypeDef hmax7301 = {

        { 0x3B, 0x3A, 0x39, 0x38, 0x37, 0x36, 0x30, 0x31, 0x32, 0x33, 0x35, 0x34, 0x26  },   // strobe addrs

        { 0x06, 0x5F, 0x2A, 0x0B, 0x53, 0x83, 0x82, 0x1F, 0x02, 0x03, 0x00, 0x00 },		// num_codes
        { 0xFF },		// dots
        ~0x02,   		// dot_code


		{ 0x00 },		// segments_data
        0xFFFFFFFF ,    // segments_mask
		0,				// active_segment

		{ 0 },			// tx_data
		{ 0 },			// rx_data
		0,				// txrx_sequence_words
        0,              // busy flag

        0				// init_state
};


/* Private function prototypes ----------------------------------------------*/
/* Functions ----------------------------------------------------------------*/

/**
  * @brief  MAX7301_Configurate
  * @param  None
  * @retval None
  */
__inline static void __msetData(uint8_t reg, uint8_t data, uint8_t *out)
{
	out[0] = data;
	out[1] = reg;
}

/**
  * @brief  MAX7301_StartCommunicate
  * @param  None
  * @retval None
  */
static void MAX7301_StartCommunicate(void)
{
    MAX_CS_SET_LOW();
    HAL_SPI_TransmitReceive_IT(
        &hspi1,
        hmax7301.tx_data,
        hmax7301.rx_data,
        1
    );
}

///**
//  * @brief  MAX7301_SetDot
//  * @param  None
//  * @retval None
//  */
//static void MAX7301_SetDot(uint8_t dot, uint8_t segment)
//{
//  if(segment < MAX_SEGMENTS_COUNT) {
//    if(dot != 0) hmax7301.dots[segment] = hmax7301.dot_code;
//    else hmax7301.dots[segment] = 0xFF;
//  }
//}

/**
  * @brief  MAX7301_Configurate
  * @param  None
  * @retval None
  */
void MAX7301_Configurate(void)
{
	hmax7301.busy = 1;
	hmax7301.txrx_sequence_words = 8;

	/* MAX7301 normal operation and transition detection disabled */
	__msetData(MAX_MAINCONFIG_REG, MAX_NORMAL_OPERATION_BIT, &hmax7301.tx_data[0]);

    // Config P8-P11 and P12-P15 as PP output
	__msetData(SEGMENTS_DATA_CNF_REG_0, MAX_PORT_OUT_PP, &hmax7301.tx_data[2]);
	__msetData(SEGMENTS_DATA_CNF_REG_1, MAX_PORT_OUT_PP, &hmax7301.tx_data[4]);

    // Config P16-P27 as PP output
    __msetData(SEGMENT_STROBE_CNF_REG_0, MAX_PORT_OUT_PP, &hmax7301.tx_data[6]);
    __msetData(SEGMENT_STROBE_CNF_REG_1, MAX_PORT_OUT_PP, &hmax7301.tx_data[8]);
    __msetData(SEGMENT_STROBE_CNF_REG_2, MAX_PORT_OUT_PP, &hmax7301.tx_data[10]);

    // Config other inputs/outputs
    __msetData(MAX_CNF_P7_P6_P5_P4_REG, MAX_PORT_OUT_PP, &hmax7301.tx_data[12]);
    __msetData(MAX_CNF_P31_P30_P29_P28_REG, MAX_PORT_IN_WITH_PU, &hmax7301.tx_data[14]);


    memset(hmax7301.dots, 0xFF, 12);
    hmax7301.dots[1] = hmax7301.dot_code;
    hmax7301.dots[3] = hmax7301.dot_code;
    hmax7301.dots[7] = hmax7301.dot_code;
    hmax7301.dots[9] = hmax7301.dot_code;

//    MAX7301_SetDot(1, 1);
//    MAX7301_SetDot(1, 3);
//    MAX7301_SetDot(1, 7);
//    MAX7301_SetDot(1, 9);

    // reset CS signal
    MAX_CS_SET_HIGHT();

    /* Start read/write to MAX7301 */
    MAX7301_StartCommunicate();
}


/**
  * @brief  MAX7301_SetMask
  * @param  None
  * @retval None
  */
void MAX7301_SetMask(uint32_t val)
{
	hmax7301.segments_mask = val;
}

/**
  * @brief  MAX7301_GetMask
  * @param  None
  * @retval None
  */
uint32_t MAX7301_GetMask(void)
{
  return hmax7301.segments_mask;
}

/**
  * @brief  MAX7301_SetData
  * @param  None
  * @retval None
  */
void MAX7301_SetData(uint8_t val, uint8_t segment)
{
  if(segment == 0xFF) {
    memset(hmax7301.segments_data, val, MAX_SEGMENTS_COUNT);
  }
  else if(segment < MAX_SEGMENTS_COUNT) {
    hmax7301.segments_data[segment] = val;
  }
}

/**
  * @brief  MAX7301_GetData
  * @param  None
  * @retval None
  */
uint8_t MAX7301_GetData(uint8_t segment)
{
  if(segment < MAX_SEGMENTS_COUNT) {
    return hmax7301.segments_data[segment];
  }

  return 0xFF;
}





/**
  * @brief  MAX7301_ReadCallback
  * @param  None
  * @retval None
  */
static void MAX7301_ReadCallback(uint16_t* data)
{
    uint8_t enc_state = ( ( (uint8_t)(*data) & 0x06 ) ^ 0x06 ) >> 1;
    uint8_t btn_state = ( (uint8_t)(*data) & 0x01 ) ^ 0x01;

    UpdateEncoderState(enc_state, btn_state);
}

/**
  * @brief  Update max7301 complite event callback.
  * @param  None
  * @retval None
  */
__weak void MAX7320_UpdateCallback(void)
{
	/*    This function Should not be modified, when the callback is needed,
	          the HW_ChannelGetDataCallback could be implemented in the user file.
	   */
}

/**
  * @brief  HAL_SPI_TxRxCpltCallback
  * @param  None
  * @retval None
  */
void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi)
{
	static int tx_xfer_count = 0;

	MAX_CS_SET_HIGHT();

	if(--hmax7301.txrx_sequence_words > 0) {
		/* continue spi transmmit sequence, sending next data word */
		tx_xfer_count++;
		MAX_CS_SET_LOW();
		HAL_SPI_TransmitReceive_IT(
				&hspi1,
				&hmax7301.tx_data[tx_xfer_count*2],
				&hmax7301.rx_data[tx_xfer_count*2],
				1
		);
	}
	else {
		/* */
        MAX7301_ReadCallback( (uint16_t*)&hmax7301.rx_data[tx_xfer_count*2] );
        /* end spi transmit sequence, no more for sending */
		tx_xfer_count = 0;
        /* reset txrx busy state flag */
        hmax7301.busy = 0;
	}
}

/**
  * @brief  HAL_TIM_PeriodElapsedCallback
  * @param  None
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	static uint8_t upd_cnt = 0;
	uint8_t temp;

    if(hmax7301.busy == 1) return;

    MAX_CS_SET_HIGHT();
    hmax7301.busy = 1;
    hmax7301.txrx_sequence_words = 5;

	/* set active segment strobe OFF */
    __msetData( hmax7301.strobe_addrs[ hmax7301.active_segment ], 0x00, &hmax7301.tx_data[0] );

    /* switch to next strobe */
    hmax7301.active_segment++;
    if(hmax7301.active_segment >= MAX_SEGMENTS_COUNT) {
    	hmax7301.active_segment = 0;
    }

    if(hmax7301.active_segment == MAX_SEGMENTS_COUNT-1) {
      temp = hmax7301.segments_data[ hmax7301.active_segment ];
    }
    else {
    	temp = hmax7301.segments_data[ hmax7301.active_segment ];
    	temp = hmax7301.num_codes[ temp ] & hmax7301.dots[ hmax7301.active_segment ];
    }
    /* set port out data */
    __msetData(SEGMENTS_DATA_REG, temp, &hmax7301.tx_data[2]);

    /* set strobe ON */
    temp = 0x01 & ( hmax7301.segments_mask >> hmax7301.active_segment );
    __msetData(	hmax7301.strobe_addrs[ hmax7301.active_segment ], temp, &hmax7301.tx_data[4] );


	/* read & dummy */
    __msetData(	(0x5D | 0x80), 0x00, &hmax7301.tx_data[6] );
    __msetData(	0x00, 0x00, &hmax7301.tx_data[8] );


    /* starting spi transmmit sequence */
	MAX7301_StartCommunicate();

	if(hmax7301.active_segment == 0) {
		if(++upd_cnt >= 10) {
			upd_cnt = 0;
			MAX7320_UpdateCallback();
		}
	}
}






