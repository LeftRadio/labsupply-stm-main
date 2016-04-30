/**
  ******************************************************************************
  * @file    max7301.h
  * @author  LeftRadio
  * @version V1.0.0
  * @date
  * @brief   header
  ******************************************************************************
**/

#ifndef __MAX7301___H
#define __MAX7301___H

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>

/* Exported define -----------------------------------------------------------*/
#define MAX_SEGMENTS_COUNT                  ((uint8_t)13)

/* Exported typedef ----------------------------------------------------------*/
/* Exported variables --------------------------------------------------------*/
/* Exported function ---------------------------------------------------------*/
void MAX7301_Configurate(void);
void MAX7301_UpdateValue(uint16_t value, uint8_t position);

void MAX7301_SetData(uint8_t val, uint8_t segment);
uint8_t MAX7301_GetData(uint8_t segment);
void MAX7301_SetMask(uint32_t val);
uint32_t MAX7301_GetMask(void);

void MAX7320_UpdateCallback(void);



#endif /* __MAX7301___H */
/*********************************************************************************************************
      END FILE
*********************************************************************************************************/
