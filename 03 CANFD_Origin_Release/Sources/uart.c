/*
 * 品智科技
 * S32K系列Printf输出函数
 */
#include"cpu.h"
#include"uart.h"

#include <stdio.h>
#include <string.h>
#include "stdarg.h"
#include <stdint.h>
#include <stdbool.h>

char USART1_TX_BUF[200];

   void u1_printf(char* fmt,...)
   {
	   uint32_t bytesRemaining;
   	va_list ap;
   	va_start(ap,fmt);
   	vsprintf((char*)USART1_TX_BUF,fmt,ap);
   	va_end(ap);
   	LPUART_DRV_SendData(INST_LPUART1, (uint8_t *)USART1_TX_BUF, strlen(USART1_TX_BUF)); //发送
    while (LPUART_DRV_GetTransmitStatus(INST_LPUART1, &bytesRemaining)
               != STATUS_SUCCESS)  {}
   }

