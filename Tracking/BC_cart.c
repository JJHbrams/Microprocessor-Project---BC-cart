/*****************************************************
This program was produced by the
CodeWizardAVR V2.05.0 Professional
Automatic Program Generator
� Copyright 1998-2010 Pavel Haiduc, HP InfoTech s.r.l.
http://www.hpinfotech.com

Project : BC cart
Version : 2.2.1
Date    : 2017-11-29
Author  : Mrjohd
Company : Univ. Chungnam
Comments: HOLY FUCKING SHIT


Chip type               : ATmega128
Program type            : Application
AVR Core Clock frequency: 16.000000 MHz
Memory model            : Small
External RAM size       : 0
Data Stack size         : 1024
*****************************************************/
         //PSD0�� 80�� �������� ������
/*****************************************************/

#include <mega128.h>
#include <delay.h>
#include <stdio.h>
#include <math.h> // LUT �����!!!
#include "AccTable.h"

// Alphanumeric LCD Module functions
//#include <alcd.h>
#include <lcd.h>
#asm
 .equ __lcd_port = 0x12 //PORTD 8
#endasm
// About ADC
#define ADC_VREF_TYPE 0x60
//About Switch
#define Left_switch_on    (!PINE.4)
#define Middle_switch_on  (!PINE.5)
#define Right_switch_on   (!PINE.6)
#define Left_switch_off   (PINE.4)
#define Middle_switch_off (PINE.5)
#define Right_switch_off  (PINE.6)
//*****************************************************************************************************************
//About Motor
#define Motor_on  ETIMSK = 0x10, TIMSK = 0x10 
#define Motor_off ETIMSK = 0x00, TIMSK = 0x00
//*****************************************************************************************************************
// ****** Declare your global variables here  ******
unsigned char h = 0; // counting variable for ADC interrupt
unsigned char sam_num = 0; // counting variable for ADC interrupt
unsigned char i = 0; // counting variable for function
unsigned char j = 0; // counting variable for function
//*****************************************************************************************************************
// LCD
unsigned char lcd_data[40];   
//*****************************************************************************************************************
//About position                                        
signed int position = 0;
//signed int position_array[8] = {0, -25, -15, -5, 5, 15, 25, 0}; //position�� �ݴ��� ��� �� �迭�� �ٲټ���  
unsigned char Pgain=60;
//*****************************************************************************************************************
// Motor                               
unsigned char RMotorPhase[8] = {0x90, 0x80, 0xa0, 0x20, 0x60, 0x40, 0x50, 0x10};
unsigned char LMotorPhase[8] = {0x01, 0x05, 0x04, 0x06, 0x02, 0x0A, 0x08, 0x09};
unsigned char RPhaseIndex = 0;
unsigned char LPhaseIndex = 0;
signed long  RaccTableIndex = 0;
signed long  LaccTableIndex = 0;
signed long  SearchTableIndex = 1100;
signed long  TableIndexTarget = 1100;
signed long  OCRr = 65535;
signed long  OCRl = 65535;
signed long temp = 0;
signed long  templ=0;   
signed long  tempr=0;
unsigned char stop_flag = 0;
signed int  denominator = 0;
unsigned char step = 0;
unsigned char minustop = 3;

//About Control flag
unsigned char Timer_flag = 1;
unsigned char stop_condition = 0;
//*****************************************************************************************************************
// ADC
//unsigned char adc_data[4][100] = {0}; //adc �� IR/�з¼���/cds���� �����
unsigned char mux = 4; 
unsigned char num_sample = 100;
unsigned char d_flag = 0;

// * PSD    
unsigned char dist_data[3][100] = {0}; //adc��ȯ ���� PSD���� ����Ǵ� �迭
unsigned int dist_sum[3]={0}; 
unsigned char dist_mean[3]={0};
unsigned char dist_max[3] = {0, 0, 0}; //tuning���� �ִ밪 �� �ּҰ��� �ֱ� ���� �迭
unsigned char dist_min[3] = {255, 255, 255};

// * cds
unsigned char cds_data[100] = {0}; //adc �� cds���� �����
unsigned int cds_sum = 0; 
unsigned char cds_mean = 0;
unsigned char cds_max = 0; //tuning���� �ִ밪 �� �ּҰ��� �ֱ� ���� �迭
unsigned char cds_min = 255;

//*****************************************************************************************************************
// Position
unsigned char leave_flag = 0; 
unsigned char approach = 0;
unsigned char detach = 0;
//unsigned char delta = 0;
unsigned long degree_factor = 0; // ���� ���� �Ǵ�
unsigned long head_reach = 0; 
unsigned long distance_Max;  // PSD�� & �����Ÿ��� �ݺ��
unsigned long distance_min;
signed int dir = 0;
unsigned char mode = 0;

//*****************************************************************************************************************
// Timer1 output compare A interrupt service routine
interrupt [TIM1_COMPA] void timer1_compa_isr(void)
{
    // Place your code here 
    RPhaseIndex++;
    RPhaseIndex &= 0x07;      
         
    if(TableIndexTarget > RaccTableIndex) RaccTableIndex++;
    if(TableIndexTarget < RaccTableIndex) //RaccTableIndex--; 
    {
        if(stop_flag == 1) RaccTableIndex -= minustop;
        else RaccTableIndex--;
    } 
    
    if(RaccTableIndex <= 0) RaccTableIndex = 0;      
    
    if(!mode)    OCRr = table[RaccTableIndex]; // Motor TEST mode
    if(mode)     OCRr = tempr;   // Motor running mode
    OCR1AH = OCRr >> 8;
    OCR1AL = OCRr;
    
    PORTC = RMotorPhase[RPhaseIndex] | LMotorPhase[LPhaseIndex];
    
    step++;

}

// Timer3 output compare A interrupt service routine
interrupt [TIM3_COMPA] void timer3_compa_isr(void)
{
    // Place your code here  
    LPhaseIndex++;
    LPhaseIndex &= 0x07;  
    
    if(TableIndexTarget > LaccTableIndex) LaccTableIndex++;
    if(TableIndexTarget < LaccTableIndex) //LaccTableIndex--;
    {
        if(stop_flag == 1) LaccTableIndex -= minustop;
        else LaccTableIndex--;
    }      
    
    if(LaccTableIndex <= 0) LaccTableIndex = 0;  
    
    if(!mode)    OCRl = table[LaccTableIndex]; // Motor TEST mode   
    if(mode)     OCRl = templ;   // Motor running mode
    OCR3AH = OCRl >> 8;
    OCR3AL = OCRl;
    PORTC = RMotorPhase[RPhaseIndex] | LMotorPhase[LPhaseIndex];
    
    step++;
}

// ********************************* ADC interrupt service routine ************************************************
interrupt [ADC_INT] void adc_isr(void)
{  

    // Read the AD conversion result   
    for (h = 0; h<=6; h++);   
    sam_num++;   
    if(mux>4) dist_data[mux-5][sam_num] = ADCH; 
    else if(mux == 4) cds_data[sam_num] = ADCH;  //ADC���� high���� ����� 
    else; 
    
    if(sam_num == num_sample) 
    {
        mux++;
        sam_num=0;
        d_flag=1;
    }
        
    if(mux > 7) mux = 4;    // PSD : PF5, 6, 7
    ADMUX = mux | 0x60;  
    ADCSRA |= 0x40;
}

// ************************************** About PSD *************************************************
// Side distance mean
void mean_dist(void)
{
    unsigned char psd_num = 0; // counting variable for function
    unsigned char num = 0; // counting variable for function 
    while(!d_flag);
    for(psd_num = 0; psd_num < 3; psd_num++)
    {
        for(num = 0; num < num_sample; num++)
            dist_sum[psd_num] += dist_data[psd_num][num];
            
        dist_mean[psd_num] = dist_sum[psd_num]/num_sample; 
        dist_sum[psd_num] = 0;
    } 
    d_flag=0;
    //delay_ms(10);                 
}
//PSD test
void PSD_test(void)
{
    unsigned char m = 0; 
    delay_ms(300); 
    
    while(Middle_switch_off) 
    {
        mean_dist();
        
        lcd_clear();
        if(Left_switch_on) m++;
        if(Right_switch_on) m--;  
        if(m>2)  m = 0;
        
        lcd_clear();
        lcd_gotoxy(0, 0);
        lcd_putsf("Testing"); 
                    
        lcd_gotoxy(0, 1);
        sprintf(lcd_data, "%d : ", m);
        lcd_puts(lcd_data);
         
        lcd_gotoxy(5, 1);
        sprintf(lcd_data, "%d", dist_mean[m]);
        lcd_puts(lcd_data);
                    
        delay_ms(100);    
    }
}

//PSD tuning
void PSD_tuning(void)
{
    unsigned char psd = 1;  
    unsigned char mode = 0;
     
    delay_ms(500);
    
    while(Middle_switch_off)
    { 
        mean_dist();
        
        lcd_clear();
        if(Left_switch_on) mode++;
        if(Right_switch_on) mode--;  
        if(mode>2)  mode = 0;
         
        lcd_gotoxy(0, 0);
        sprintf(lcd_data, "%d", mode);
        lcd_puts(lcd_data);
              
        lcd_gotoxy(1, 0);
        lcd_putsf("MAX");
            
        lcd_gotoxy(5, 0);
        lcd_putsf("min");
            
        lcd_gotoxy(0, 1);
        sprintf(lcd_data, "%d", dist_max[mode]);
        lcd_puts(lcd_data);  
            
        lcd_gotoxy(5, 1);
        sprintf(lcd_data, "%d", dist_min[mode]);
        lcd_puts(lcd_data);  
        
        if(dist_mean[psd] < dist_min[psd]) dist_min[psd] = dist_mean[psd];
        if(dist_mean[psd] > dist_max[psd]) dist_max[psd] = dist_mean[psd];  
        
        delay_ms(300);
        psd++;
        if(psd > 2) psd=0;
    }   
}

// ************************************* About cds *******************************************************
void mean_cds(void)
{
    unsigned char num = 0; // counting variable for function 
    while(!d_flag);
    for(num = 0; num < num_sample; num++)
        cds_sum += cds_data[num];
            
    cds_mean = cds_sum/num_sample; 
    cds_sum = 0; 
    
    d_flag=0;
    //delay_ms(10);                 
}

void cds_test(void)
{
    delay_ms(300);   
    
    while(Middle_switch_off) 
    {
        mean_cds();
        
        lcd_clear();
        lcd_gotoxy(0, 0);
        lcd_putsf("Testing"); 
                    
        lcd_gotoxy(0, 1);
        sprintf(lcd_data, "%d", cds_mean);
        lcd_puts(lcd_data);
                    
        delay_ms(200);    
    }
}

//cds tuning
void cds_tuning(void)
{ 
    delay_ms(500);
    
    while(Middle_switch_off)
    { 
        mean_cds();
        
        lcd_clear();
        lcd_gotoxy(0, 0);
        lcd_putsf("MAX");
            
        lcd_gotoxy(5, 0);
        lcd_putsf("min");
            
        lcd_gotoxy(0, 1);
        sprintf(lcd_data, "%d", cds_max);
        lcd_puts(lcd_data);  
            
        lcd_gotoxy(5, 1);
        sprintf(lcd_data, "%d", cds_min);
        lcd_puts(lcd_data);  
        
        if(cds_mean < cds_min) cds_min = cds_mean;
        if(cds_mean > cds_max) cds_max = cds_mean;  
        
        delay_ms(100);
    }   
}

// *********************************** About Motor **********************************************
void initiation(void)
{
    //Ÿ�̸� ���� �ʱ�ȭ
    OCRr = 65535;
    OCRl = 65535;
    // Step initiation
    step = 0;
    Motor_on;
} 

void motor_off(void)
{
    TableIndexTarget = 0; 
    stop_flag = 1; 
    while((LaccTableIndex>0) || (RaccTableIndex>0))
    {
        TableIndexTarget = 0;
    }
    stop_flag = 0; 
    Motor_off;
    
    PORTC = 0x00; 
    TableIndexTarget = SearchTableIndex;
    stop_condition = 0;
}

void motor_test(void)
{  
    lcd_clear();
    delay_ms(500);
    lcd_gotoxy(0, 0);
    lcd_putsf("TESTing");
    
    initiation();
    while(Middle_switch_off && !stop_condition)
    {  
        PORTA = 0x01;  
        mean_cds();
        lcd_clear();
        
       //STOP condition
       if(step > 2000)             stop_condition = 1;
       if(cds_mean < cds_min+10)   stop_condition = 1;      
    }; 
    motor_off(); 
    PORTA=0x00;
}

//*************************************** About Running ***************************************************************
// Update position
void update_position(void)
{    
    distance_Max = (10000/dist_min[1]);  // PSD�� & �����Ÿ��� �ݺ��
    distance_min = (10000/dist_max[1]); 
    //mean_dist();
    // �ڽ� ��Ż �Ǵ�  
    if(dist_mean[1] < dist_min[1] || dist_mean[1] > dist_max[1]) 
        leave_flag = 1;
        
    else
    {
        // ����˵� ����
        detach = 0;
        approach = 0;
        leave_flag = 0; 
        //degree_factor = 0;
    }
 
    if(leave_flag) 
    {
        // ********** Degree of head ********** 
        degree_factor = (unsigned long)(100*(2*dist_mean[0])/(dist_min[0]+dist_max[0]));
           
        if(degree_factor > 100) degree_factor = 100; 
        head_reach = (unsigned long)(100*degree_factor/dist_mean[1]);  
        
        // ******** Right/Left posiotion ********
        // Detaching from wall
        if(head_reach > distance_Max)
        {                                                                                                                                                                                                                                                                          
            detach = 1; 
            approach = 0;
            //delta = dist_min[0] - dist_mean[0];    
        } 
            
        // Approaching wall
        if(head_reach < distance_min)
        {
            approach = 1; 
            detach = 0;
            //delta = dist_mean[0] - dist_max[0];    
        } 
    } 
    if(detach)  dir = -1;
    else if(approach) dir = 1; 
    else;
    // ��� : degree_factor(100*cos(theta)), detach, approach(direction)
             
}

void check_angle(void)
{
    delay_ms(100);
    while(Middle_switch_off)
    {
        mean_dist();
        update_position(); 
        lcd_clear();
        lcd_gotoxy(0, 0);
        lcd_putsf("Angle");
        lcd_gotoxy(0, 1);
        sprintf(lcd_data, "%d", dir * (degree_factor-100)); 
        lcd_puts(lcd_data);                  
        delay_ms(500);  
    }  
}

void update_PID(void)
{
    //signed long temp;   
    
    // P-control
    //temp = (signed long)Pgain * degree_factor;  
    //temp = (signed long)(Pgain * dir * (degree_factor-100) * (25/15));
    temp = (signed long)(Pgain * dir * (degree_factor-100));
    
    // Saturation
    if(temp > 1111) temp = 1111;                   
    else if(temp < -1111) temp = -1111; 
    
    // ���͵���̹� Ư��
    temp = temp * 9;
    denominator = temp;
    
    // ���ͼӵ� ����
    tempr = (((unsigned long)table[RaccTableIndex]) * 10000 / (denominator+10000));
    if(tempr>65534) tempr = 65534;
	if(tempr<1) tempr = 1;
    
    templ = (((unsigned long)table[LaccTableIndex]) * 10000 / (-denominator+10000));
    if(templ>65534) templ = 65534;
    if(templ<1) templ = 1;   
}   

void Heading()
{
    delay_ms(100);
    while(Middle_switch_off)
    {
        mean_dist();
        update_position();
         
        lcd_clear();
        lcd_gotoxy(0, 0);
        lcd_putsf("Head?"); 
        if(approach)
        {
            lcd_gotoxy(0, 1);
            lcd_putsf("Approach");
        }
        if(detach)
        {
            lcd_gotoxy(0, 1);
            lcd_putsf("Detach");
        } 
        if(!approach && !detach)
        {
            lcd_gotoxy(0, 1);
            lcd_putsf("Right");
        }
                          
        delay_ms(500); 
        Motor_off; 
    }  
}

//About race
void navigate(void)
{  
    lcd_clear();
    lcd_gotoxy(0, 0);
    lcd_putsf("GO!!"); 
    delay_ms(500);
    mode = 1; 
    
    initiation();  
    while(Middle_switch_off && !stop_condition)
    {  
        mean_dist();
        mean_cds();
        update_position();
        update_PID(); 
            
        //STOP condition                                 
        if(step > 2000)             stop_condition = 1;
        if(cds_mean < cds_min+10)   stop_condition = 1; 
    }
    motor_off();
    mode = 0;
}

void Pgain_adj(void)
{
    delay_ms(100);
    while(Middle_switch_off)
    {
        if(Left_switch_on) Pgain+=5;
        if(Right_switch_on) Pgain-=5;
             
        lcd_clear();
        lcd_gotoxy(0,0);
        lcd_putsf("Pgain!!");
        
        lcd_gotoxy(0, 1);
        sprintf(lcd_data, "%d", Pgain);
        lcd_puts(lcd_data);
        delay_ms(100);
    }
}

void speed_adj(void)
{
    delay_ms(100);
    while(Middle_switch_off)
    {
        if(Left_switch_on) TableIndexTarget+=5;
        if(Right_switch_on) TableIndexTarget-=5;
             
        lcd_clear();
        lcd_gotoxy(0,0);
        lcd_putsf("Pgain!!");
        
        lcd_gotoxy(0, 1);
        sprintf(lcd_data, "%d", TableIndexTarget);
        lcd_puts(lcd_data);
        delay_ms(100);
    }
}

// ********************************************* main ******************************************************************
void main(void)
{
// Declare your local variables here
// menu
unsigned char menu = 0;
unsigned char menu_Max = 9;


PORTA=0x00;
DDRA=0xFF;
 
PORTB=0x00;
DDRB=0x00;

PORTC=0x00;
DDRC=0xFF;

PORTD=0x00;
DDRD=0xFF;

PORTE=0x00;
DDRE=0x8F;

PORTF=0x00;
DDRF=0x00;

PORTG=0x00;
DDRG=0x00;
/*
// Timer/Counter 0 initialization
// Clock source: System Clock
// Clock value: Timer 0 Stopped
// Mode: Normal top=0xFF
// OC0 output: Disconnected
ASSR=0x00;
TCCR0=0x00;
TCNT0=0x00;
OCR0=0x00;
*/
// Timer/Counter 1 initialization
TCCR1A=0x00;
TCCR1B=0x09;
TCNT1H=0x00;
TCNT1L=0x00;
ICR1H=0x00;
ICR1L=0x00;
OCR1AH=0x00;
OCR1AL=0x00;
OCR1BH=0x00;
OCR1BL=0x00;
OCR1CH=0x00;
OCR1CL=0x00;
/*
// Timer/Counter 2 initialization
// Clock source: System Clock
// Clock value: Timer2 Stopped
// Mode: Normal top=0xFF
// OC2 output: Disconnected
TCCR2=0x00;
TCNT2=0x00;
OCR2=0x00;
*/
TCCR3A=0x00;
TCCR3B=0x09;
TCNT3H=0x00;
TCNT3L=0x00;
ICR3H=0x00;
ICR3L=0x00;
OCR3AH=0x00;
OCR3AL=0x00;
OCR3BH=0x00;
OCR3BL=0x00;
OCR3CH=0x00;
OCR3CL=0x00;

ADMUX=0x25;
ADCSRA=0xCF;

lcd_init(8);
// Global enable interrupts
#asm("sei")
//SREG = 0x80;
while (1)
      { 
        if(Left_switch_on) menu++;
        if(Right_switch_on) menu--;    
        if(menu > menu_Max)    menu = 0;
        if(menu == 0)
            if(Right_switch_on) menu = menu_Max;  
            
        //Distance averaging...
        //mean_dist();
        switch(menu)
        { 
            case 0:
                    lcd_clear();
                    lcd_gotoxy(0, 0);
                    lcd_putsf("1.PSD TEST"); 
                    if(Middle_switch_on) PSD_test(); 
                    delay_ms(50);  
                    break;
                    
            case 1:
                    lcd_clear();
                    lcd_gotoxy(0, 0);
                    lcd_putsf("2.CDS");
                    if(Middle_switch_on)    cds_test();    
                    delay_ms(50); 
                    break; 
                    
            case 2:
                    
                    lcd_clear();
                    lcd_gotoxy(0, 0);
                    lcd_putsf("3.PSD Tuning");  
                    if(Middle_switch_on)    PSD_tuning();                      
                    delay_ms(50); 
                    break;  
                    
            case 3:
                    lcd_clear();
                    lcd_gotoxy(0, 0);
                    lcd_putsf("4.cds Tuning");  
                    if(Middle_switch_on)    cds_tuning();                      
                    delay_ms(50); 
                    break; 
                     
                    
            case 4:
                    lcd_clear();
                    lcd_gotoxy(0, 0);
                    lcd_putsf("5.Motor_test");  
                    if(Middle_switch_on)    motor_test();    
                    delay_ms(50); 
                    break;
                    
             case 5:
                    lcd_clear();
                    lcd_gotoxy(0, 0);
                    lcd_putsf("6.Cos");  
                    if(Middle_switch_on)    check_angle();    
                    delay_ms(50); 
                    break; 
                    
             case 6:
                    lcd_clear();
                    lcd_gotoxy(0, 0);
                    lcd_putsf("7.Head");  
                    if(Middle_switch_on)    Heading();    
                    delay_ms(50); 
                    break;   
                    
             case 7:
                    lcd_clear();
                    lcd_gotoxy(0, 0);
                    lcd_putsf("8.Navigate");  
                    if(Middle_switch_on)    navigate();    
                    delay_ms(50); 
                    break; 
                    
             case 8:
                    lcd_clear();
                    lcd_gotoxy(0, 0);
                    lcd_putsf("9.Pgain");  
                    lcd_gotoxy(0, 1);
                    sprintf(lcd_data, "%d", Pgain);
                    lcd_puts(lcd_data);
                    if(Middle_switch_on)    Pgain_adj();    
                    delay_ms(50); 
                    break;
             
             case 9:
                    lcd_clear();
                    lcd_gotoxy(0, 0);
                    lcd_putsf("10.Speed"); 
                    lcd_gotoxy(0, 1);
                    sprintf(lcd_data, "%d", TableIndexTarget);
                    lcd_puts(lcd_data); 
                    if(Middle_switch_on)    speed_adj();    
                    delay_ms(50); 
                    break;                                
         }
         delay_ms(250);   
      }
    
}
