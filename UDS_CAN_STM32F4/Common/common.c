#include "common.h"

/*********************************************************************************
************************�������� STM32F407���Ŀ�����******************************
**********************************************************************************
* �ļ�����: common.c                                                             *
* �ļ���������������������õĹ����ļ�                                           *
* �������ڣ�2015.03.03                                                           *
* ��    ����V1.0                                                                 *
* ��    �ߣ�Clever                                                               *
* ˵    ���������������Ͷ��塢IO��λ���塢λ�ζ�������ʱ��������                 * 
**********************************************************************************
*********************************************************************************/	  

/****************************************************************************
* ��    ��: void GPIO_group_OUT(_gpio_group *group,u16 outdata)
* ��    �ܣ�ʹ������16��IO�����һ��16λ���������
* ��ڲ�����*group�� ����16��IO��ΪԪ�صĽṹ��ָ��
            outdata: 16λ�����ֵ
* ���ز�������
* ˵    ����outdata�Ӹ�λ��ʼ��ֵ
****************************************************************************/
void GPIO_group_OUT(_gpio_group *group,u16 outdata)
{
  u8 t;
	for(t=0;t<16;t++)
    {               
        if((outdata&0x8000)>>15)  
				{
						switch(t)
						{
								case 0:	   group->data15=1; break;
								case 1:	   group->data14=1; break;
								case 2:	   group->data13=1; break;
								case 3:	   group->data12=1; break;
								case 4:	   group->data11=1; break;
								case 5:	   group->data10=1; break;
								case 6:	   group->data9=1;  break;
								case 7:	   group->data8=1;  break;
								case 8:	   group->data7=1;  break;
								case 9:	   group->data6=1;  break;
								case 10:	 group->data5=1;  break;
								case 11:	 group->data4=1;  break;
								case 12:	 group->data3=1;  break;
								case 13:	 group->data2=1;  break;
								case 14:	 group->data1=1;  break;
								case 15:	 group->data0=1;  break;
						}
				}
				else
				{
				  switch(t)
						{
								case 0:	   group->data15=0; break;
								case 1:	   group->data14=0; break;
								case 2:	   group->data13=0; break;
								case 3:	   group->data12=0; break;
								case 4:	   group->data11=0; break;
								case 5:	   group->data10=0; break;
								case 6:	   group->data9=0;  break;
								case 7:	   group->data8=0;  break;
								case 8:	   group->data7=0;  break;
								case 9:	   group->data6=0;  break;
								case 10:	 group->data5=0;  break;
								case 11:	 group->data4=0;  break;
								case 12:	 group->data3=0;  break;
								case 13:	 group->data2=0;  break;
								case 14:	 group->data1=0;  break;
								case 15:	 group->data0=0;  break;
						}
				}
     outdata<<=1; 	
	  }
}
/****************************************************************************
* ��    ��: void GPIO_bits_OUT(GPIO_TypeDef* GPIOx, u8 start_bit, u8 bit_size,u16 outdata)
* ��    �ܣ�λ�β���ʵ�֣�ͬһIO�ڵļ�λ�����������
* ��ڲ�����* GPIOx��  ��Ӧ��IO��
*           start_bit: �����������ʼλ
*           bit_size:  Ҫ���������λ��
* ���ز�������
* ˵    ����start_bit: 0~14
            bit_size:  1~16 
            bit_size<=16-start_bit
****************************************************************************/
void GPIO_bits_OUT(GPIO_TypeDef* GPIOx, u8 start_bit, u8 bit_size,u16 outdata)
{
  u8 i=0;
	u16 bu1=0;u16 middata=1;

	if( bit_size>(16-start_bit) ) 
     bit_size=16-start_bit;
	
	i=start_bit;
	if(i>0)
		 {
			 while(i--)
         { bu1+=middata; middata*=2;}
		 }
	
   assert_param(IS_GPIO_ALL_PERIPH(GPIOx));
   
	 GPIOx->ODR&=(  ( (0xffff<<bit_size) <<start_bit ) |bu1   ); 
	 GPIOx->ODR|=(outdata<<start_bit);		 
}

/*****************************************************************************
**********************���´���ο����ϣ�����ѧϰ�ο�**************************
*****************************************************************************/
//THUMBָ�֧�ֻ������
//�������·���ʵ��ִ�л��ָ��WFI  
// GCC compatible inline assembly
void WFI_SET(void)
{
    __asm volatile ("WFI");
}

void INTX_DISABLE(void)
{
    __asm volatile ("CPSID I");
}

void INTX_ENABLE(void)
{
    __asm volatile ("CPSIE I");
}

void MSR_MSP(u32 addr)
{
    __asm volatile ("MSR MSP, %0" : : "r" (addr));
}

//����ϵͳ�δ�ʱ����д����ʱ����

static u8  fac_us=0; //us��ʱ������			   
static u16 fac_ms=0; //ms��ʱ������,��ucos��,����ÿ�����ĵ�ms��

/****************************************************************************
* ��    ��: delay_init()
* ��    �ܣ���ʱ������ʼ��
* ��ڲ�������
* ���ز�������
* ˵    ����
****************************************************************************/
void delay_init()
{
 	SysTick_CLKSourceConfig(SysTick_CLKSource_HCLK_Div8);
	fac_us=SYSCLK/8;	 
	fac_ms=(u16)fac_us*1000; //ÿ��ms��Ҫ��systickʱ����   
}								    

/****************************************************************************
* ��    ��: void delay_us(u32 nus)
* ��    �ܣ���ʱnus
* ��ڲ�����Ҫ��ʱ��΢����
* ���ز�������
* ˵    ����nus��ֵ,��Ҫ����798915us
****************************************************************************/
void delay_us(u32 nus)
{		
	u32 midtime;	    	 
	SysTick->LOAD=nus*fac_us; //ʱ�����	  		 
	SysTick->VAL=0x00;        //��ռ�����
	SysTick->CTRL|=SysTick_CTRL_ENABLE_Msk ;          //��ʼ���� 
	do
	{
		midtime=SysTick->CTRL;
	}
	while((midtime&0x01)&&!(midtime&(1<<16)));//�ȴ�ʱ�䵽��   
	SysTick->CTRL&=~SysTick_CTRL_ENABLE_Msk;       //�رռ�����
	SysTick->VAL =0X00;       //��ռ�����	 
}

/****************************************************************************
* ��    ��: void delay_xms(u16 nms)
* ��    �ܣ���ʱnms
* ��ڲ�����Ҫ��ʱ�ĺ�����
* ���ز�������
* ˵    ����SysTick->LOADΪ24λ�Ĵ���,����,�����ʱΪ: nms<=0xffffff*8*1000/SYSCLK
            ��168M������,nms<=798ms 
****************************************************************************/
void delay_xms(u16 nms)
{	 		  	  
	u32 midtime;		   
	SysTick->LOAD=(u32)nms*fac_ms;//ʱ�����(SysTick->LOADΪ24bit)
	SysTick->VAL =0x00;           //��ռ�����
	SysTick->CTRL|=SysTick_CTRL_ENABLE_Msk ;          //��ʼ����  
	do
	{
		midtime=SysTick->CTRL;
	}
	while((midtime&0x01)&&!(midtime&(1<<16)));//�ȴ�ʱ�䵽��   
	SysTick->CTRL&=~SysTick_CTRL_ENABLE_Msk;       //�رռ�����
	SysTick->VAL =0X00;       //��ռ�����	  	    
} 

/****************************************************************************
* ��    ��: void delay_ms(u16 nms)
* ��    �ܣ���ʱnms
* ��ڲ�����Ҫ��ʱ�ĺ�����
* ���ز�������
* ˵    ����nms:0~65535
****************************************************************************/
void delay_ms(u16 nms)
{	 	 
	u8 repeat=nms/540;	//������540,�ǿ��ǵ�ĳЩ�ͻ����ܳ�Ƶʹ��,
						          //���糬Ƶ��248M��ʱ��,delay_xms���ֻ����ʱ541ms������
	u16 remain=nms%540;
	while(repeat)
	{
		delay_xms(540);
		repeat--;
	}
	if(remain)delay_xms(remain);
} 

			 
