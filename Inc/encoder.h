/**
  ******************************************************************************
  * @file    encoder.h
  * @author  LeftRadio
  * @version V1.0.0
  * @date
  * @brief   header
  ******************************************************************************
**/

#ifndef __ENCODER___H
#define __ENCODER___H

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>

/* Exported define -----------------------------------------------------------*/
/* Exported typedef ----------------------------------------------------------*/
typedef enum { ENC_STOP, ENC_DIR_LEFT, ENC_DIR_RIGHT } EncoderDirection_TypeDef;
typedef enum { ENC_BTN_OFF, ENC_BTN_SHORT, ENC_BTN_LONG } EncoderButton_TypeDef;

typedef struct {
  EncoderDirection_TypeDef direction;
  EncoderButton_TypeDef button;
} HW_Encoder_TypeDef;

/* Exported variables --------------------------------------------------------*/
/* Exported function ---------------------------------------------------------*/
void UpdateEncoderState(uint8_t data, uint8_t btn);
HW_Encoder_TypeDef* GetEncoder(void);


#endif /* __ENCODER___H */
/*********************************************************************************************************
      END FILE
*********************************************************************************************************/