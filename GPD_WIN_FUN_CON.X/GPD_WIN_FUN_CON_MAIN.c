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

//HI-TECH�̃w�b�_���C���N���[�h
#include <pic.h>
#include <pic16f887.h>

/*�f�B���C��`*/
#define _XTAL_FREQ 4000000
#define  _XTAL_FREQ_LOW 32000
#define __delay_ms(x) _delay((unsigned long)((x)*(_XTAL_FREQ/4000.0)))
#define __delay_us(x) _delay((unsigned long)((x)*(_XTAL_FREQ/4000000.0)))
#define __low_delay_ms(x) _delay((unsigned long)((x)*(_XTAL_FREQ_LOW/4000.0)))
#define __low_delay_us(x) _delay((unsigned long)((x)*(_XTAL_FREQ_LOW/4000000.0)))

// ********** �ǂꂩ���L����(�R�����g�A�E�g���O��) *******
//#define SLEEP
//#define ONESHOT
#define NORM
//#define SPS
// ***********************************************************

/*�O��PIN��`*/
#define LED1 RA4    //LED��
#define LED2 RA5    //LED��
#define SDA RC4     //SDA
#define SCL RC3     //SCL
#define ADADD 0x48    //AD ADDRESS
#define FUN1 RC6
#define FUN2 RC5

int TMR1ON_Flag = 0; //�^�C�}�[�t���O

//�R���t�B�O�ݒ�
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

void interrupt InterTimer(){//���荞�݊֐�
    if (TMR1IF == 1) {  // �^�C�}�[1�̃t���O�����邩�H
        TMR1ON_Flag = 0 ;// �^�C�}�[�t���O���Z�b�g
        TMR1IF = 0 ;
    }
}

oscsel(int sel1) {//�����I�V���[�g�Z���N�g
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

i2c_Tx(char date) {//I2C1byte��������
    SSPIF = 0;
    SSPBUF = date;
    while (BF);
    while (!SSPIF);
}

i2c_byte_write(int sel, char add, char cmd, char date1, char date2) {//I2C��������
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

i2c_byte_read(unsigned int sel, char add, char regadd) {//I2C���[�h�֐�
    char read_data[2];
    SSPADD = 0x09; //I2C�ʐM���x�Ē�`
    SEN = 1; //�X�^�[�g�R���f�V�����Z�b�g
    while (SEN); //���M�����܂ő҂�
    i2c_Tx(add); //IC�A�h���X���M
    i2c_Tx(regadd); //���W�X�^�A�h���X���M
    RSEN = 1; //���X�^�[�g�R���f�B�V�����Z�b�g
    while (RSEN); //���M�����܂ő҂�
    add = add << 1;
    i2c_Tx(add + 0x01); //IC�A�h���X�����[�h���[�h�ŏ�������
    SSPIF = 0; //�Ȃ������Ă����t���O�i��������Ȃ��j
    RCEN = 1; //�}�X�^�����[�h���[�h��
    while (RCEN); //���M�����܂ő҂�
    while (!BF); //�o�b�t�@���t����
    read_data[0] = SSPBUF; //�o�b�t�@�̃f�[�^��Ҕ�
    ACKDT = 0; //ACK���Z�b�g
    ACKEN = 1; //ACK�𑗐M
    while (ACKEN); //���M�����܂ő҂�
    SSPIF = 0; //�Ȃ������Ă����t���O�i��������Ȃ��j
    RCEN = 1; //�}�X�^�����[�h���[�h��
    while (RCEN); //���M�����܂ő҂�
    while (!BF); //�o�b�t�@���t����
    read_data[1] = SSPBUF; //�o�b�t�@�̃f�[�^��Ҕ�
    ACKDT = 1; //NO-ACK���Z�b�g
    ACKEN = 1; //NO-ACK�𑗐M
    while (ACKEN); //���M�����܂ő҂�
    SSPIF = 0; //�Ȃ������Ă����t���O�i��������Ȃ��j
    PEN = 1; //�X�g�b�v�R���f�V�����Z�b�g
    while (PEN);//���M�����܂ő҂�
    SSPADD = 0x09; //I2C���x�Ē�`
    return (read_data[sel]);
}


int main(void) {
    unsigned int val=0x0000;
    float tmp;
    int faltemp = 38;
    oscsel(0); //4MHz��
    ANSEL = 0x00; //I/O�f�W�^��
    ANSELH = 0x00;
    TRISA = 0b11001111; //A�|�[�g����
    TRISB = 0b11111111; //B�|�[�g�ݒ�
    TRISC = 0b00011011; //C�|�[�gSAD��SCL����͂�
    TRISD = 0x00; //D�|�[�g�ݒ�
    SSPCON = 0b00101000; //I2C�̐ݒ� Fosc/(4*(SSPADD+1))
    SSPSTAT = 0b10000000; //SSPSTAT�ݒ�
    SSPADD = 0x09; // I2C�ʐM���g��100KHz for 4MHz
    T1CON = 0b01110000; //�^�C�}�[1�ݒ�
    TMR1IF = 0;
    TMR1IE = 1;
    TMR1ON = 0;
    PEIE = 1;
    GIE = 1;
    i2c_byte_write(0, ADADD, 0x03, 0x40, 0x00);
    oscsel(1); //32khz��
    LED1 = 0;
    LED2 = 1;
    FUN1 = 0;
    FUN2 = 1;
    __low_delay_ms(3000);//1�b

    LED1 = 0;
    LED2 = 0;

   for(;;){
       oscsel(0); //4Mhz��
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

