/*****************************************************
This program was produced by the
CodeWizardAVR V2.05.0 Professional
Automatic Program Generator
� Copyright 1998-2010 Pavel Haiduc, HP InfoTech s.r.l.
http://www.hpinfotech.com

Project : BC cart
Version : 2.4.1
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
***************** Version 2.4.1 ������� ********************//* 

- ����/�������¸� ��Ÿ���� ��������, state�� ���X  
    
*//************************************************************/

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
#define Left_switch_on    (!PINE.6)
#define Middle_switch_on  (!PINE.5)
#define Right_switch_on   (!PINE.4)
#define Left_switch_off   (PINE.6)
#define Middle_switch_off (PINE.5)
#define Right_switch_off  (PINE.4)
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
//About PID                                        
unsigned int Pgain=42;
//*****************************************************************************************************************
// Motor   
// Actual phase array
unsigned char RMotorPhase_real[8] = {0};
unsigned char LMotorPhase_real[8] = {0};   
// Backward phase                         
unsigned char RMotorPhase_B[8] = {0x90, 0x80, 0xa0, 0x20, 0x60, 0x40, 0x50, 0x10};
unsigned char LMotorPhase_B[8] = {0x01, 0x05, 0x04, 0x06, 0x02, 0x0A, 0x08, 0x09};
// Forward phase
unsigned char RMotorPhase_F[8] = {0x10, 0x50, 0x40, 0x60, 0x20, 0xA0, 0x80, 0x90};
unsigned char LMotorPhase_F[8] = {0x09, 0x08, 0x0A, 0x02, 0x06, 0x04, 0x05, 0x01};

unsigned char RPhaseIndex = 0;
unsigned char LPhaseIndex = 0;

signed long  RaccTableIndex = 0;
signed long  LaccTableIndex = 0;

signed long  SearchTableIndex = 200;
signed long  TableIndexTarget = 200 ;

signed long  OCRr = 65535;
signed long  OCRl = 65535;

signed long temp = 0;
signed long  templ=0;   
signed long  tempr=0;
unsigned long MAX_temp; 
unsigned long min_temp;
unsigned char stop_flag = 0;
signed int  denominator = 0;
unsigned long step = 0;
unsigned long stop_step = 6000;
unsigned char minustop = 3;

//About Control flag
unsigned char Timer_flag = 1;
unsigned char stop_condition = 0;
//*****************************************************************************************************************
// ADC
//unsigned char adc_data[4][100] = {0}; //adc �� IR/�з¼���/cds���� �����
unsigned char mux = 4; 
unsigned char num_sample = 20;
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
unsigned long distance_Max0;  // PSD�� & �����Ÿ��� �ݺ��
unsigned long distance_min0;
unsigned long distance_Max1;  
unsigned long distance_min1;
unsigned long distance_now0;
unsigned long distance_now1;
signed int dir = 0;
unsigned char mode = 0;
unsigned char compensate = 1000;

//*****************************************************************************************************************
// Timer1 output compare A interrupt service routine
interrupt [TIM1_COMPA] void timer1_compa_isr(void)
{
    // Place your code here 
    RPhaseIndex++;
    RPhaseIndex &= 0x07;      
         
    if(TableIndexTarget > RaccTableIndex) RaccTableIndex++;
    if(TableIndexTarget < RaccTableIndex) 
    {
        if(stop_flag == 1) RaccTableIndex -= minustop;
        else RaccTableIndex--;
    } 
    
    if(RaccTableIndex <= 0) RaccTableIndex = 0;      
    
    if(!mode)    OCRr = table[RaccTableIndex]; // Motor TEST mode
    if(mode)     OCRr = tempr;   // Motor running mode
    OCR1AH = OCRr >> 8;
    OCR1AL = OCRr;
    
    PORTC = RMotorPhase_real[RPhaseIndex] | LMotorPhase_real[LPhaseIndex];
    
    step++;
}

// Timer3 output compare A interrupt service routine
interrupt [TIM3_COMPA] void timer3_compa_isr(void)
{
    // Place your code here  
    LPhaseIndex++;
    LPhaseIndex &= 0x07;  
    
    if(TableIndexTarget > LaccTableIndex) LaccTableIndex++;
    if(TableIndexTarget < LaccTableIndex)
    {
        if(stop_flag == 1) LaccTableIndex -= minustop;
        else LaccTableIndex--;
    }      
    
    if(LaccTableIndex <= 0) LaccTableIndex = 0;  
    
    if(!mode)    OCRl = table[LaccTableIndex]; // Motor TEST mode   
    if(mode)     OCRl = templ;   // Motor running mode
    OCR3AH = OCRl >> 8;
    OCR3AL = OCRl;
    PORTC = RMotorPhase_real[RPhaseIndex] | LMotorPhase_real[LPhaseIndex];
    
    step++;
}

// ********************************* ADC interrupt service routine ************************************************
interrupt [ADC_INT] void adc_isr(void)
{  
    // Read the AD conversion result   
    //for (h = 0; h<=6; h++);      
    if(mux>4) dist_data[mux-5][sam_num] = ADCH;     
    else if(mux == 4) cds_data[sam_num] = ADCH;  //ADC���� high���� ����� 
    else; 
    
    if(sam_num == num_sample) 
    {
        mux++;
        sam_num=0;
        d_flag=1;
    }
        
    if(mux > 7)  mux = 4;    // PSD : PF5, 6, 7 
    ADMUX = mux | 0x60;  
    ADCSRA |= 0x40;  
    sam_num++;
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
                    
        delay_ms(300);    
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
        
        delay_ms(100);
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
void motor_phase_setting(char p_mode)
{
    int phasing;
    // p_mode = 0 : forward
    // p_mode = 1 : backward
    if(!p_mode)
    {
        for(phasing = 0; phasing < 8; phasing++)
        {
            RMotorPhase_real[phasing] = RMotorPhase_F[phasing];
            LMotorPhase_real[phasing] = LMotorPhase_F[phasing];     
        }
    }  
    
    else
    {
        for(phasing = 0; phasing < 8; phasing++)
        {
            RMotorPhase_real[phasing] = RMotorPhase_B[phasing];
            LMotorPhase_real[phasing] = LMotorPhase_B[phasing];     
        }
    } 
}

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
    long temp_step;
    
    lcd_clear();
    delay_ms(500);
    lcd_gotoxy(0, 0);
    lcd_putsf("TESTing");  
    
    motor_phase_setting(0); 
    initiation();
      
    /*
    while(Middle_switch_off && !stop_condition)
    {  
        PORTA.0 = 1;  
        mean_cds();
        
       if(step > stop_step)        
       {
            motor_off();  
            delay_ms(500); 
            state = ~state;
            motor_phase_setting(state); 
            step = 0; 
            initiation(); 
       }   
       //STOP condition
       if(cds_mean < cds_min+10)   stop_condition = 1;      
    }
    */ 
    
    while(Middle_switch_off)
    {  
        mean_cds();
        //Back condition  
        if(cds_mean < cds_min+10)   
        {
            motor_off(); 
            stop_condition = 1;
            motor_phase_setting(1);
            temp_step = step;
            delay_ms(500); //�̺κ� ��ſ� å������ �۾� �߰�
            initiation();
        }   
        else; 
        
        // �������� ��ȯ                              
          
        if((step > (temp_step+100)) && (stop_condition))  //100�� ����� ������ ���� ������      
        {
            motor_off(); 
            motor_phase_setting(0); 
            temp_step = 0;
        } 
        
    }
    
    motor_off(); 
    PORTA.0=0;
}

//*************************************** About Running ***************************************************************
// Update position
void update_position(signed int direction)
{    
    distance_Max0 = (10000/dist_min[0]);  // PSD�� & �����Ÿ��� �ݺ��
    distance_min0 = (10000/dist_max[0]);
    distance_now0 = (10000/dist_mean[0]);
     
    distance_Max1 = (10000/dist_min[1]);  // PSD�� & �����Ÿ��� �ݺ��
    distance_min1 = (10000/dist_max[1]);  
    distance_now1 = (10000/dist_mean[1]);
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
        degree_factor = 200;
    }
 
    if(leave_flag) 
    {
        // ********** Degree of head ********** 
        //degree_factor = (unsigned long)(200*(2*dist_mean[0])/(dist_min[0]+dist_max[0]));
        degree_factor = (unsigned long)(compensate * distance_min0/distance_now0);
           
        if(degree_factor > 1000) degree_factor = 1000; 
        //head_reach = (unsigned long)(50*degree_factor/dist_mean[1]); 
        head_reach = (unsigned long)(degree_factor * distance_now1 /compensate);  
        
        // ******** Right/Left posiotion ********
        // Detaching from wall
        if(head_reach > distance_Max1)
        {                                                                                                                                                                                                                                                                          
            detach = 1; 
            approach = 0;    
        } 
            
        // Approaching wall
        if(head_reach < distance_min1)
        {
            approach = 1; 
            detach = 0;   
        } 
    } 
    if(detach)  dir = -1;
    else if(approach) dir = 1; 
    // ��� : degree_factor(200*cos(theta)), detach, approach(direction)
    degree_factor -= compensate; 
    degree_factor = dir * direction;        
}

void check_angle(void)
{
    signed int FB_flag = 1;
    delay_ms(100);
    while(Middle_switch_off)
    {
        mean_dist();
        update_position(FB_flag); 
        lcd_clear();
        lcd_gotoxy(0, 0);
        lcd_putsf("Angle");
        lcd_gotoxy(0, 1);
        sprintf(lcd_data, "%d", degree_factor); 
        lcd_puts(lcd_data);                  
        delay_ms(500);  
    }  
}

void update_PID(void)
{
    // direction = 1 : forward
    // direction = -1 : backward
    temp = (signed long)(Pgain * degree_factor); 
    
    // Saturation
    if(temp > 1111) temp = 1111;                   
    else if(temp < -1111) temp = -1111; 
    
    // ���͵���̹� Ư��
    temp = temp * 9;
    denominator = temp;
    
    // ���ͼӵ� ����
    tempr = (((unsigned long)table[RaccTableIndex]) * 10000 / (denominator+10000));
    if(tempr>18463) tempr = MAX_temp;
    if(tempr<2565) tempr = min_temp;
    
    templ = (((unsigned long)table[LaccTableIndex]) * 10000 / (-denominator+10000));
    if(templ>18463) templ = MAX_temp; //�����ӵ�       
    if(templ<2565) templ = min_temp; //�ְ�ӵ�
}   

void Heading()
{
    signed int FB_flag = 1;
    delay_ms(100);
    while(Middle_switch_off)
    {
        mean_dist();
        update_position(FB_flag);
         
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
    long temp_step; 
    signed int FB_flag = 1;
    
    lcd_clear();
    lcd_gotoxy(0, 0);
    lcd_putsf("GO!!"); 
    delay_ms(500);
    mode = 1; 
    
    initiation();
    motor_phase_setting(0);  
    while(Middle_switch_off)
    {  
        if(Left_switch_on) initiation();  //������ȯ �� �ٽ� ������� - TEST (������Ȳ������ ���԰��� Ȥ�� ���ڵ� �ν����� ���� flag�� ����)
        
        mean_dist();
        mean_cds();
        update_position(FB_flag);
        update_PID();             
        
        //Back condition 
        if(cds_mean < cds_min+10)   
        {
            motor_off();
            stop_condition = 1;
            motor_phase_setting(1);
            temp_step = step;
            delay_ms(500); //�̺κ� ��ſ� å������ �۾� �߰�
            initiation();   
            FB_flag = -1;
        }    
        
        // �������� ��ȯ                               
        if((step > temp_step+100) && (stop_condition))  //100�� ����� ������ ���� ������      
        {
            motor_off();    
            motor_phase_setting(0);  
            temp_step=0; 
            FB_flag = 1; 
        } 
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
        SearchTableIndex = TableIndexTarget;
    }
}

void s_step_adj(void)
{
    delay_ms(100);
    while(Middle_switch_off)
    {
        if(Left_switch_on) stop_step+=500;
        if(Right_switch_on) stop_step-=500;
             
        lcd_clear();
        lcd_gotoxy(0,0);
        lcd_putsf("s_stop!!");
        
        lcd_gotoxy(0, 1);
        sprintf(lcd_data, "%d", stop_step);
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
unsigned char menu_Max = 10;

MAX_temp = (unsigned long)((table[TableIndexTarget] * 10000) / (9*Pgain*(-15)+10000)); 
min_temp = (unsigned long)(table[TableIndexTarget]);

PORTA=0xFE;
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
//��� ��Ʈ Pull up 
SFIOR=0x00;

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
                    delay_ms(300);  
                    break;
                    
            case 1:
                    lcd_clear();
                    lcd_gotoxy(0, 0);
                    lcd_putsf("2.CDS");
                    if(Middle_switch_on)    cds_test();    
                    delay_ms(300); 
                    break; 
                    
            case 2:
                    
                    lcd_clear();
                    lcd_gotoxy(0, 0);
                    lcd_putsf("3.PSD Tuning");  
                    if(Middle_switch_on)    PSD_tuning();                      
                    delay_ms(300); 
                    break;  
                    
            case 3:
                    lcd_clear();
                    lcd_gotoxy(0, 0);
                    lcd_putsf("4.cds Tuning");  
                    if(Middle_switch_on)    cds_tuning();                      
                    delay_ms(300); 
                    break; 
                     
                    
            case 4:
                    lcd_clear();
                    lcd_gotoxy(0, 0);
                    lcd_putsf("5.Motor_test");  
                    if(Middle_switch_on)    motor_test();    
                    delay_ms(300); 
                    break;
                    
             case 5:
                    lcd_clear();
                    lcd_gotoxy(0, 0);
                    lcd_putsf("6.Cos");  
                    if(Middle_switch_on)    check_angle();    
                    delay_ms(300); 
                    break; 
                    
             case 6:
                    lcd_clear();
                    lcd_gotoxy(0, 0);
                    lcd_putsf("7.Head");  
                    if(Middle_switch_on)    Heading();    
                    delay_ms(300); 
                    break;   
                    
             case 7:
                    lcd_clear();
                    lcd_gotoxy(0, 0);
                    lcd_putsf("8.Navigate");  
                    if(Middle_switch_on)    navigate();    
                    delay_ms(300); 
                    break; 
                    
             case 8:
                    lcd_clear();
                    lcd_gotoxy(0, 0);
                    lcd_putsf("9.Pgain");  
                    lcd_gotoxy(0, 1);
                    sprintf(lcd_data, "%d", Pgain);
                    lcd_puts(lcd_data);
                    if(Middle_switch_on)    Pgain_adj();    
                    delay_ms(300); 
                    break;
             
             case 9:
                    lcd_clear();
                    lcd_gotoxy(0, 0);
                    lcd_putsf("10.Speed"); 
                    lcd_gotoxy(0, 1);
                    sprintf(lcd_data, "%d", TableIndexTarget);
                    lcd_puts(lcd_data); 
                    if(Middle_switch_on)    speed_adj();    
                    delay_ms(300); 
                    break;  
                    
             case 10:
                    lcd_clear();
                    lcd_gotoxy(0, 0);
                    lcd_putsf("11.S_STEP"); 
                    lcd_gotoxy(0, 1);
                    sprintf(lcd_data, "%d", stop_step);
                    lcd_puts(lcd_data); 
                    if(Middle_switch_on)    s_step_adj();    
                    delay_ms(300); 
                    break; 
                                                 
         }
         //delay_ms(250);   
      }
    
}

