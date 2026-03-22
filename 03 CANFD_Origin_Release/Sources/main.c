/*

*/
#include "Cpu.h"
#include "delay.h"
#include "uart.h"
#include"key.h"
#include "can_fd_wrapper.h"
#include "uds_stack.h"

#define LPIT_CHANNEL        0UL
#define LPIT_Channel_IRQn   LPIT0_Ch0_IRQn

volatile int exit_code = 0;
extern uint32_t recv_buff_tick;
uint32_t tickMs = 0;
void LPIT_ISR(void)
{
    /* Clear LPIT channel flag */
    LPIT_DRV_ClearInterruptFlagTimerChannels(INST_LPIT1, (1 << LPIT_CHANNEL));
    /* Toggle LED0 */
    tickMs++;
    /* 调用UDS定时器中断处�? */
    UdsTimerISR();
}


uint8_t txBuff[64];

/* CAN 测试发送数据 */
uint8_t canTestData[8] = {0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA};
uint32_t lastCanSendTick = 0;

int main(void)
{
  /* Write your local variable definition here */
	uint8_t pinstate;
	int MCU_Freq;
    uint8_t state;
	uint8_t CANRXDATA_STR1[17];
	uint8_t CANRXDATA_STR2[17];
  /*** Processor Expert internal initialization. DON'T REMOVE THIS CODE!!! ***/
  #ifdef PEX_RTOS_INIT
    PEX_RTOS_INIT();                   /* Initialization of the selected RTOS. Macro is defined by the RTOS component. */
  #endif
  /*** End of Processor Expert internal initialization.                    ***/

	CLOCK_SYS_Init(g_clockManConfigsArr, CLOCK_MANAGER_CONFIG_CNT,g_clockManCallbacksArr, CLOCK_MANAGER_CALLBACK_CNT);
	CLOCK_SYS_UpdateConfiguration(0U, CLOCK_MANAGER_POLICY_AGREEMENT);
	MCU_Freq = delay_init();//��ʼ��delay����
	PINS_DRV_Init(NUM_OF_CONFIGURED_PINS, g_pin_mux_InitConfigArr); //��ʼ��IO
	LPUART_DRV_Init(INST_LPUART1, &lpuart1_State, &lpuart1_InitConfig0); //��ʼ������

	CAN0_Init();
	//CAN0 cantrcv normal
	PINS_DRV_WritePin(PTC,11,1);
	PINS_DRV_WritePin(PTC,12,1);
	
	/* 初始化UDS协议�? */
	UdsStackInit();

        /* Initialize LPIT instance 0
     *  -   Reset and enable peripheral
     */
    LPIT_DRV_Init(INST_LPIT1, &lpit1_InitConfig);
    /* Initialize LPIT channel 0 and configure it as a periodic counter
     * which is used to generate an interrupt every second.
     */
    LPIT_DRV_InitChannel(INST_LPIT1, LPIT_CHANNEL, &lpit1_ChnConfig0);

    /* Install LPIT_ISR as LPIT interrupt handler */
    INT_SYS_InstallHandler(LPIT_Channel_IRQn, &LPIT_ISR, (isr_t *)0);

    /* Start LPIT0 channel 0 counter */
    LPIT_DRV_StartTimerChannels(INST_LPIT1, (1 << LPIT_CHANNEL));
    while(1)
    {
		/*��������*/
			pinstate = KEY_Proc (0);
			if(pinstate ==BTN1_PRES )
			{
                state = CanFdSendQuick(&can_pal0_instance, TX_MAILBOX_CAN0,0x777,txBuff,64);
				u1_printf("CAN0���ͱ��� 0x%x \r\n",state);

			}

            if(tickMs % 1000 == 0)
            {
                u1_printf("tick %d %d\r\n",tickMs,recv_buff_tick);
            }
            
            /* 每秒发送 CAN ID 0x301 测试报文 */
            if(tickMs - lastCanSendTick >= 1000)
            {
                lastCanSendTick = tickMs;
                /* 发送 CAN ID 0x301, 8字节数据 0xAA */
                state = CanFdSendQuick(&can_pal0_instance, TX_MAILBOX_CAN0, 0x301, canTestData, 8);
                u1_printf("CAN Send ID:0x301 Data:AA.AA.AA.AA.AA.AA.AA.AA Status:0x%x\r\n", state);
            }
            
            /* UDS主任务处�? */
            UdsMainTask();
			// if (IRQ_CAN0_RX ==1)
			//  {
			// 	 int i;
			// 	 u1_printf("CAN0 RECV ID:0x%x \r\n",recvMsg_CAN0.id);
			// 	 for(i=0; i<recvMsg_CAN0.length;i++)
			// 	 {
			// 		 u1_printf("Data %d : %x\r\n",i,recvMsg_CAN0.data[i]);
			// 		 if(i==recvMsg_CAN0.length-1) u1_printf("***************\r\n");
			// 	 }
			// 	 IRQ_CAN0_RX=0;
			//  }
    }
  /*** Don't write any code pass th5is line, or it will be deleted during code generation. ***/
  /*** RTOS startup code. Macro PEX_RTOS_START is defined by the RTOS component. DON'T MODIFY THIS CODE!!! ***/
  #ifdef PEX_RTOS_START
    PEX_RTOS_START();                  /* Startup of the selected RTOS. Macro is defined by the RTOS component. */
  #endif
  /*** End of RTOS startup code.  ***/
  /*** Processor Expert end of main routine. DON'T MODIFY THIS CODE!!! ***/
  for(;;) {
    if(exit_code != 0) {
      break;
    }
  }
  return exit_code;
  /*** Processor Expert end of main routine. DON'T WRITE CODE BELOW!!! ***/
} /*** End of main routine. DO NOT MODIFY THIS TEXT!!! ***/

/* END main */
/*!
** @}
*/
/*
** ###################################################################
**
**     This file was created by Processor Expert 10.1 [05.21]
**     for the NXP S32K series of microcontrollers.
**
** ###################################################################
*/
