#include "adc.h"
#include "delay.h"
#include "sys.h"
#define CAL_PPM 20  // 校准环境中PPM值
#define RL			5		// RL阻值
extern u8 times;
static float R0; // 元件在洁净空气中的阻值
u16 adcx;
float Vrl=0;
float RS=0;
float MQ2_GetPPM(void)
{   
	
	  adcx=Get_Adc_Average(ADC_Channel_1,30);//ADC1,取30次的平均值
       Vrl = 3.3f * adcx / 4096.f;//3.3v的参考电压，4096份
	    Vrl = ( (float)( (int)( (Vrl+0.005)*100 ) ) )/100;
       RS = (3.3f - Vrl) / Vrl * RL;
	  
      if(times<6) // 获取系统执行时间，3s前进行校准，用到了定时器
       {
		  R0 = RS / pow(CAL_PPM / 613.9f, 1 / -2.074f);//校准R0
       } 
	  float ppm = 613.9f * pow(RS/R0, -2.074f);

      return  ppm;
}
