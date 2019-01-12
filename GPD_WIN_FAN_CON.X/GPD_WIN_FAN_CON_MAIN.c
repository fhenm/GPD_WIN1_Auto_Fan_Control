/* 
 * File:   GPD_WIN_FUN_CON_MAIN.c
 * Author: msk319882
 *
 * Created on 2017/10/10, 22:31
 */

#include <stdio.h>
#include <stdlib.h>

/*
 * 
 */

//HI-TECHのヘッダをインクルード
#include <pic.h>
#include <pic16f887.h>

/*ディレイ定義*/
#define _XTAL_FREQ 4000000
#define  _XTAL_FREQ_LOW 32000
#define __delay_ms(x) _delay((unsigned long)((x)*(_XTAL_FREQ/4000.0)))
#define __delay_us(x) _delay((unsigned long)((x)*(_XTAL_FREQ/4000000.0)))
#define __low_delay_ms(x) _delay((unsigned long)((x)*(_XTAL_FREQ_LOW/4000.0)))
#define __low_delay_us(x) _delay((unsigned long)((x)*(_XTAL_FREQ_LOW/4000000.0)))

// ********** どれか一つを有効化(コメントアウトを外す) *******
//#define SLEEP
//#define ONESHOT
#define NORM
//#define SPS
// ***********************************************************

/*外部PIN定義*/
#define LED1 RA4    //LEDへ
#define LED2 RA5    //LEDへ
#define SDA RC4     //SDA
#define SCL RC3     //SCL
#define ADADD 0x48    //AD ADDRESS
#define FUN1 RC6
#define FUN2 RC5

int TMR1ON_Flag = 0; //タイマーフラグ

//コンフィグ設定
__CONFIG(
        BOREN_OFF
        & WRT_OFF
        & LVP_OFF
        & FCMEN_OFF
        & IESO_OFF
        & CPD_OFF
        & CP_OFF
        & MCLRE_OFF
        & PWRTE_ON
        & FOSC_INTRC_NOCLKOUT
        & WDTE_OFF
        );

void interrupt InterTimer(){//割り込み関数
    if (TMR1IF == 1) {  // タイマー1のフラグがあるか？
        TMR1ON_Flag = 0 ;// タイマーフラグリセット
        TMR1IF = 0 ;
    }
}

oscsel(int sel1) {//内部オシレートセレクト
    switch (sel1) {
        case 0:
            OSCCON = 0b01100000; //4MHz
            break;
        case 1:
            OSCCON = 0b00000000; //32KHz
            break;
        case 2:
            OSCCON = 0b01000000; //1MHz
            break;
        case 3:
            OSCCON = 0b00110000; //500KHz
            break;
    }
}

i2c_Tx(char date) {//I2C1byte書き込み
    SSPIF = 0;
    SSPBUF = date;
    while (BF);
    while (!SSPIF);
}

i2c_byte_write(int sel, char add, char cmd, char date1, char date2) {//I2C書き込み
    SEN = 1;
    while (SEN);
    i2c_Tx(add);
    i2c_Tx(cmd);
    i2c_Tx(date1);
    if (sel == 1)i2c_Tx(date2);
    SSPIF = 0;
    PEN = 1;
    while (PEN);
    __delay_us(27);
}

i2c_byte_read(unsigned int sel, char add, char regadd) {//I2Cリード関数
    char read_data[2];
    SSPADD = 0x09; //I2C通信速度再定義
    SEN = 1; //スタートコンデションセット
    while (SEN); //送信完了まで待ち
    i2c_Tx(add); //ICアドレス送信
    i2c_Tx(regadd); //レジスタアドレス送信
    RSEN = 1; //リスタートコンディションセット
    while (RSEN); //送信完了まで待ち
    add = add << 1;
    i2c_Tx(add + 0x01); //ICアドレスをリードモードで書き込み
    SSPIF = 0; //なぜかつけている謎フラグ（多分いらない）
    RCEN = 1; //マスタをリードモードへ
    while (RCEN); //送信完了まで待ち
    while (!BF); //バッファがフルか
    read_data[0] = SSPBUF; //バッファのデータを待避
    ACKDT = 0; //ACKをセット
    ACKEN = 1; //ACKを送信
    while (ACKEN); //送信完了まで待ち
    SSPIF = 0; //なぜかつけている謎フラグ（多分いらない）
    RCEN = 1; //マスタをリードモードへ
    while (RCEN); //送信完了まで待ち
    while (!BF); //バッファがフルか
    read_data[1] = SSPBUF; //バッファのデータを待避
    ACKDT = 1; //NO-ACKをセット
    ACKEN = 1; //NO-ACKを送信
    while (ACKEN); //送信完了まで待ち
    SSPIF = 0; //なぜかつけている謎フラグ（多分いらない）
    PEN = 1; //ストップコンデションセット
    while (PEN);//送信完了まで待ち
    SSPADD = 0x09; //I2C速度再定義
    return (read_data[sel]);
}


int main(void) {
    unsigned int val=0x0000;
    float tmp;
    int faltemp = 38;
    oscsel(0); //4MHzへ
    ANSEL = 0x00; //I/Oデジタル
    ANSELH = 0x00;
    TRISA = 0b11001111; //Aポート入力
    TRISB = 0b11111111; //Bポート設定
    TRISC = 0b00011011; //CポートSADとSCLを入力に
    TRISD = 0x00; //Dポート設定
    SSPCON = 0b00101000; //I2Cの設定 Fosc/(4*(SSPADD+1))
    SSPSTAT = 0b10000000; //SSPSTAT設定
    SSPADD = 0x09; // I2C通信周波数100KHz for 4MHz
    T1CON = 0b01110000; //タイマー1設定
    TMR1IF = 0;
    TMR1IE = 1;
    TMR1ON = 0;
    PEIE = 1;
    GIE = 1;
    i2c_byte_write(0, ADADD, 0x03, 0x40, 0x00);
    oscsel(1); //32khzへ
    LED1 = 0;
    LED2 = 1;
    FUN1 = 0;
    FUN2 = 1;
    __low_delay_ms(3000);//1秒

    LED1 = 0;
    LED2 = 0;

   for(;;){
       oscsel(0); //4Mhzへ
       LED1 = 1;
       val = i2c_byte_read(0, ADADD, 0x02);
       LED1 = 0;
       if(val >= 0b10000000){
            i2c_byte_write(0, ADADD, 0x03, 0x40, 0x00);
            LED2 = 1;
       }
       if(val <= 0b01111111){
            LED2 = 0;
            val = i2c_byte_read(0, ADADD, 0x00);
            val = val << 8;
            val = val + i2c_byte_read(1, ADADD, 0x00);
            tmp = val / 128;

            if(tmp >= faltemp && tmp <= (faltemp+6)){
                FUN1 = 0;
                FUN2 = 1;
                LED1 = 0;
            }
            else if(tmp < faltemp){
                FUN1 = 0;
                FUN2 = 0;
                LED1 = 0;
            }
            else if(tmp >(faltemp+6)){
                FUN1 = 1;
                FUN2 = 0;
                LED1 = 1;
            }
       }
    }
}

