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
#include "encoder.h"


/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/
/* Private typedef -----------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
HW_Encoder_TypeDef encoder = {
  ENC_STOP,
  ENC_BTN_OFF
};

/* Private function prototypes ----------------------------------------------*/
/* Functions ----------------------------------------------------------------*/

/**
  * @brief  ReadEncoder
  * @param
  * @retval None
  */
void UpdateEncoderState(uint8_t data, uint8_t btn)
{
  /* ---------- ENCODER ---------- */
  static uint8_t enc_state = 0x00;

  switch (enc_state) {

  case 0:
    if(data == 0x01) enc_state = 1;
    else if(data == 0x02) enc_state = 5;
  break;

  /* left */
  case 1:
    if(data == 0x03) enc_state = 2;
    else if (data == 0x02) enc_state = 0;
  break;

  case 2:
    if(data == 0x02) enc_state = 3;
    else if (data == 0x01) enc_state = 0;
  break;

  case 3:
    if(data == 0x00) {
      encoder.direction = ENC_DIR_LEFT;
      enc_state = 0;
    }
    else if (data == 0x03) enc_state = 0;
  break;

  /* right */
  case 5:
    if(data == 0x03) enc_state = 6;
    else if (data == 0x01) enc_state = 0;
  break;

  case 6:
    if(data == 0x01) enc_state = 7;
    else if (data == 0x02) enc_state = 0;
  break;

  case 7:
    if(data == 0x00) {
      encoder.direction = ENC_DIR_RIGHT;
      enc_state = 0;
    }
    else if (data == 0x03) enc_state = 0;
  break;

  }

  /* ---------- BUTTON ---------- */
  static uint8_t btn_state = 0x00;
  static uint16_t btn_push_cnt = 0;

  switch(btn_state) {

  case 0:
    if(btn == 0x01) {
      btn_state = 1;
    }
    break;

  case 1:
    if(btn == 0x01) {
    	btn_push_cnt++;
    	if(btn_push_cnt > 1000) {
    		encoder.button = ENC_BTN_LONG;
    		btn_state = 2;
    	}

    }
    else {
    	if(btn_push_cnt < 1000) {
    		encoder.button = ENC_BTN_SHORT;
    		btn_state = 2;
    	}
    }
    break;

  case 2:
    if(btn == 0x00) {
      btn_state = 0;
      btn_push_cnt = 0;
    }
    break;
  }
}



HW_Encoder_TypeDef* GetEncoder(void)
{
  return &encoder;
}
