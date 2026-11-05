/*
 * BNO055_Driver.h
 *
 *  Created on: 22 Eyl 2026
 *      Author: Mansu
 */

#ifndef INC_BNO055_DRIVER_H_
#define INC_BNO055_DRIVER_H_

#include "main.h"
#include "stdio.h"
#include <string.h>
#include "math.h"
#include "MadgwickAHRS.h"


#define BNO055_I2C_address 			(0x28<<1) // alternatif 0x29
#define WHO_AM_I 					0x00  //0xA0 okumam lazım

#define AXIS_MAP_CONFIG_ADDR 		0x41

#define AXIS_MAP_SIGN_ADDR    		0x42

#define ACC_DATA_X_LSB				0x08

#define UNIT_SEL					0x3B

#define OPR_MODE_ID					0x3D

#define AMG_MODE					0x07

#define POWER_MODE					0x3E

#define	TEMP_SOURCE					0x40
#define READ_TEMPERATURE			0x34

#define ACC_Config_address			0x08

#define PAGE_ID_ADDR            	0x07
#define PAGE_0                  	0x00
#define PAGE_1                 		0x01
#define GYR_CONFIG_0_ADDR       	0x0A

#define MAG_Config				    0x09


typedef union
{
	uint8_t unit_sel;

	struct
	{
		uint8_t acceleration_unit : 1;
		uint8_t angular_rate_unit : 1;
		uint8_t euler_angle_unit  : 1;
		uint8_t reserved_bit3     : 1;
		uint8_t temperature_unit  : 1;
		uint8_t reserved_bits     : 2;
		uint8_t output_format     : 1;

	}bits;

}Output_data_t;


typedef union
{

	uint8_t mode_value;
	struct
	{
		 uint8_t opr_mode : 4;
		 uint8_t reserved : 4;

	}OPR_mode_bits;

}OPR_Mode_select;

typedef struct
{
	I2C_HandleTypeDef *i2c;
	float  manyetometre[3];
	float  ivme[3];
	float  jiroskop[3];

	uint8_t who_i_am;

	float yaw;
	float roll;
	float pitch;

	float roll_acc;
	float pitch_acc;

	float kayma_jiroskop[3];

	float filtreli_ivme_eksen[3];
	float filtreli_manyometre_eksen[3];
	float kaymasi_temizlenmis_jiroskop[3];
	float manyetometre_offset[3];

	float donme_miktar[3];
	float dt;
	float alpha;
	float beta;
	float ivme_agirligi;

	float D;
	float w;

	float baslangic_aci;
	float birlesik_aci, tahmin_aci;

	float ivme_min[3];
	float ivme_max[3];
	float jiroskop_min[3];
	float jiroskop_max[3];

	float ivme_degisim[3];
	float jiroskop_degisim[3];

	float ornek_sayi;
	float kayma[3];
	float kayma_jiroskop_yedek[3];

	uint8_t sabit_flag;

	uint8_t filtre_baslatildi;
	uint8_t roll_pitch_baslatildi;

	float mag_hard_iron[3];
	float mag_soft_iron[3];
	float mag_gecici;
	float hata;
	uint16_t baslangic_sayac;

}sensor_info_t;


typedef struct
{
	float alfa;
	float a0;
	float b0;
	float b1;
	float a1;
	float a2;

	float x0[3],x1[3], x2[3];
	float x3[3],x4[3], x5[3];
	float x6[3],x7[3], x8[3];
	float y1[3], y2[3];
	float y3[3], y4[3];
	float y5[3], y6[3];

}filter_calc_t;

typedef struct
{

	uint16_t ornek_sayi2;
	float manyometre_min[3];
	float manyometre_max[3];
	float hard_iron_offset[3];
	float soft_iron_katsay[3];
	float toplam_Yaricap;
	uint8_t test_calib;



}test_value;


void BNO055_Init(sensor_info_t *sensor_info , OPR_Mode_select *mode , Output_data_t *output_data_info ,  filter_calc_t *f, test_value *test);

void BNO055_Config_Mode(sensor_info_t *sensor_info , OPR_Mode_select *mode);

void axis_remap(sensor_info_t *sensor_info);

void AMG_Mode(sensor_info_t *sensor_info , OPR_Mode_select *mode);

uint8_t Read_BNO055(sensor_info_t *sensor_info , filter_calc_t *f , test_value *test );

void filter_ivme(sensor_info_t *sensor_info , filter_calc_t *f);

void filter_manyometre(sensor_info_t *sensor_info , filter_calc_t *f, test_value *test);

void filter_jiroskop(sensor_info_t *sensor_info , filter_calc_t *f);

void filter_Init(sensor_info_t *sensor_info , float fs, float fc, filter_calc_t *f);

void jiroskop_kayma(sensor_info_t *sensor_info , filter_calc_t *f);

void jiroskop_calc(sensor_info_t *sensor_info );

void sensor_init(sensor_info_t *sensor_info, test_value *test);

void kuzey_hesap(sensor_info_t *sensor_info);

void test_calibration(sensor_info_t *sensor_info , test_value *test );

void I2C_Bus_Clear(void);

void complementary_Filter(sensor_info_t *sensor_info);

void soft_iron_calc(sensor_info_t *sensor_info , test_value *test);



#endif /* INC_BNO055_DRIVER_H_ */
