/**
  ******************************************************************************
  * @file       channels.h
  * @author     Neil Lab :: Left Radio
  * @version    v1.0.0
  * @date
  * @brief      header
  ******************************************************************************
**/

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __CHANNELS_H
#define __CHANNELS_H

/* Includes ------------------------------------------------------------------*/
/* Exported define -----------------------------------------------------------*/
#define CH_V_LIM_MAX				((uint16_t)320)
#define CH_V_LIM_MIN				((uint8_t)30)
#define CH_I_LIM_MAX				((uint16_t)250)
#define CH_I_LIM_MIN				((uint8_t)5)

/* Exported macro ------------------------------------------------------------*/
/* Exported typedef ----------------------------------------------------------*/
typedef enum { HW_SELF_CH = 0, HW_EXTERN_CH = 1, HW_ALL_CH = 2 } HW_RegMode_TypeDef;
typedef enum { HW_CONTROL_V = 0, HW_CONTROL_I = 1 } HW_ControlType_TypeDef;

typedef struct {
  uint32_t v;
  uint32_t i;
} HW_Data_TypeDef;

typedef struct {
  HW_Data_TypeDef data;
  HW_Data_TypeDef lim;
  uint8_t hard_protect_state;
  void (*GetOut)(void);
  void (*SetOut)(int8_t sign);
  void (*UpdateLimits)(void);
  void (*ProtectionReset)(void);
} HW_Channel_TypeDef;

/* Exported variables --------------------------------------------------------*/
extern HW_Channel_TypeDef hw_channel_self;
extern HW_Channel_TypeDef hw_channel_external;

/* Exported function ---------------------------------------------------------*/
void HW_Channel_GetDataCallback(HW_Channel_TypeDef* channel);


#endif /* __CHANNELS_H */
/*********************************************************************************************************
      END FILE
*********************************************************************************************************/
