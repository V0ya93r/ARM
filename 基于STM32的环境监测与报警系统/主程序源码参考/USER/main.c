#include "sys.h"
#include "delay.h"
#include "usart.h"
#include "led.h"
#include "dht11.h"
#include "beep.h"
#include "esp8266.h"
#include "onenet.h"
#include "stdio.h"
#include "bh1750.h"
#include "adc.h"
#include "timer.h"
#include "mq2.h"
#include "relay.h"
#include "fs.h"
#include "pump.h"
/************************************************
 ALIENTEK精英STM32开发板实验1
 跑马灯实验
 技术支持：www.openedv.com
 淘宝店铺：http://eboard.taobao.com
 关注微信公众平台微信号："正点原子"，免费获取STM32资料。
 广州市星翼电子科技有限公司
 作者：正点原子 @ALIENTEK
************************************************/

u8 humidityH;	  //湿度整数部分
u8 humidityL;	  //湿度小数部分
u8 temperatureH;   //温度整数部分
u8 temperatureL;   //温度小数部分
u8 alarmFlag = 0;//是否报警的标志
u8 alarm_is_free = 10;//报警器是否被手动操作，如果被手动操作即设置为0
u8 ESP8266_INIT_OK = 0;//esp8266初始化完成标志
u8 Led_Status = 0;//led状态
//u16 adcx=0;//adc的初始值
u16 adcx1;
u16 shidu;
char PUB_BUF[256];//上传数据的buf
unsigned char HDMI_BUF[64];//HDMI发送数据buf
float Light = 0; //光照度
float co2 = 0; //二氧化碳浓度
float temp = 0;
float hum = 0;
const char* devSubTopic[] = {"/mysmartfarm/sub"};
const char devPubTopic[] = "/mysmartfarm/pub";
int main(void)
{
    unsigned short timeCount = 0;	//发送间隔变量
    unsigned char* dataPtr = NULL;
    delay_init();	    //延时函数初始化
    LED_Init();		  	//初始化与LED连接的硬件接口
    DHT11_Init();//DHT11初始化
    BEEP_Init();//蜂鸣器初始化
    RELAY_Init();//继电器初始化
    BH1750_Init();//光照传感器初始化
    Adc_Init();//adc初始化
    Adc2_Init();//adc2初始化
    Usart1_Init(115200);//串口初始化
    Usart2_Init(115200);//esp8266通讯串口初始化
    Usart3_Init(9600);//HDMI串口屏初始化
    ESP8266_Init();//esp8266初始化
    fs_Init();
    pump_Init();
    HMISendstart();//HDMI清空
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2); //设置NVIC中断分组2:2位抢占优先级，2位响应优先级
    TIM3_Int_Init(4999, 7199); //10Khz的计数频率，计数到5000为500ms
    while(OneNet_DevLink())
        delay_ms(500);
    // BEEP=0;//鸣叫表示接入成功
    // delay_ms(250);
    // BEEP=1;

    OneNet_Subscribe(devSubTopic, 1);
    while(1)
    {
//			delay_ms(100);
        adcx1 = Get_Adc_Average(ADC_Channel_10, 10);
        shidu = ((4095 - adcx1) * 100) / 3292;
//		UsartPrintf(USART1,"adcx1：%d",adcx1);
//		UsartPrintf(USART1,"shidu: %d",shidu);
        DHT11_Read_Data(&humidityH, &humidityL, &temperatureH, &temperatureL);
        temp = temperatureH + temperatureL * 0.01;
        sprintf((char*)HDMI_BUF, "page0.t0.txt=\"%.1f\"", temp);
        HMISends((char*)HDMI_BUF);
        HMISendb(0xff);

        sprintf((char*)HDMI_BUF, "page0.t2.txt=\"%d\"", shidu);
        HMISends((char*)HDMI_BUF);
        HMISendb(0xff);

        Light = LIght_Intensity();
        sprintf((char*)HDMI_BUF, "page0.t1.txt=\"%.1f\"", Light);
        HMISends((char*)HDMI_BUF);
        HMISendb(0xff);

        co2 = MQ2_GetPPM();
        sprintf((char*)HDMI_BUF, "page0.t3.txt=\"%.1f\"", co2);
        HMISends((char*)HDMI_BUF);
        HMISendb(0xff);

        if(++timeCount >= 200)									//发送间隔5s
        {
            Led_Status = GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_5); //读取LED0的状态

            // hum=humidityH+humidityL*0.01;

//

//			UsartPrintf(USART1,"湿度高位：%d",humidityH);
//		UsartPrintf(USART1,"湿度低位：%d",humidityL);
//		UsartPrintf(USART1,"湿度：%.1f",hum);


            //adcx=Get_Adc_Average(ADC_Channel_1,10);

            UsartPrintf(USART1, "湿度:%d温度:%d.%d 光照强度：%.1f ppm值：%.1f", shidu, temperatureH, temperatureL, Light, co2); //c格式化字符串
            UsartPrintf(USART_DEBUG, "OneNet_Publish\r\n");
            sprintf(PUB_BUF, "{\"Hum\":%d,\"Temp\":%d.%d,\"Light\":%.1f,\"Co2\":%.1f,\"Led\":%d,\"Beep\":%d}",
                    shidu, temperatureH, temperatureL, Light, co2, Led_Status ? 0 : 1, alarmFlag);
            //OneNet_Publish("pcTopic", "MQTT Publish Test");
//
            OneNet_Publish(devPubTopic, PUB_BUF);
            timeCount = 0;
            ESP8266_Clear();
        }
//
        dataPtr = ESP8266_GetIPD(3);
        if(dataPtr != NULL)
            OneNet_RevPro(dataPtr);
        //Light=LIght_Intensity();
        //DHT11_Read_Data(&humidityH,&humidityL,&temperatureH,&temperatureL);
        // UsartPrintf(USART1," 光照强度：%.1f",Light);



//		delay_ms(10);
//		LED0=0;
//		LED1=1;
//		delay_ms(300);	 //延时300ms
//		LED0=1;
//		LED1=0;
//		delay_ms(300);	//延时300ms

    }
}


