	/*
 * BNO055_Driver.c
 *
 *  Created on: 22 Eyl 2026
 *      Author: Mansu
 */
#include "BNO055_Driver.h"
#include "MadgwickAHRS.h"


void BNO055_Init(sensor_info_t *sensor_info , OPR_Mode_select *mode , Output_data_t *output_data_info ,  filter_calc_t *f, test_value *test)
{
	HAL_GPIO_WritePin(BNO_RST_GPIO_Port, BNO_RST_Pin, GPIO_PIN_RESET);
	HAL_Delay(10);

	HAL_GPIO_WritePin(BNO_RST_GPIO_Port, BNO_RST_Pin, GPIO_PIN_SET);
	HAL_Delay(1000);

	HAL_I2C_Mem_Read(sensor_info->i2c, BNO055_I2C_address, WHO_AM_I, I2C_MEMADD_SIZE_8BIT, &sensor_info->who_i_am, 1, 100);


    BNO055_Config_Mode(sensor_info, mode);

    HAL_I2C_Mem_Write(sensor_info->i2c, BNO055_I2C_address, UNIT_SEL, I2C_MEMADD_SIZE_8BIT, &output_data_info->unit_sel, sizeof(output_data_info->unit_sel), 100);

    axis_remap(sensor_info);

    /* --- GYRO BANT GENİŞLİĞİNİ 230 Hz YAPTIK --- */
    uint8_t page1 = PAGE_1;
    uint8_t page0 = PAGE_0;
    uint8_t gyr_conf = 0x08;
    uint8_t mag_conf = 0x1f;

    HAL_I2C_Mem_Write(sensor_info->i2c, BNO055_I2C_address, PAGE_ID_ADDR, I2C_MEMADD_SIZE_8BIT, &page1, 1, 100);

    HAL_I2C_Mem_Write(sensor_info->i2c, BNO055_I2C_address, GYR_CONFIG_0_ADDR, I2C_MEMADD_SIZE_8BIT, &gyr_conf, 1, 100);

    HAL_I2C_Mem_Write(sensor_info->i2c, BNO055_I2C_address, MAG_Config, I2C_MEMADD_SIZE_8BIT, &mag_conf, 1 , 100);

    HAL_I2C_Mem_Write(sensor_info->i2c, BNO055_I2C_address, PAGE_ID_ADDR, I2C_MEMADD_SIZE_8BIT, &page0, 1, 100);
    /* ------------------------------------------ */

    AMG_Mode(sensor_info, mode);
    sensor_init(sensor_info , test);
    filter_Init(sensor_info, 50.0f, 5.0f, f);
}

void BNO055_Config_Mode(sensor_info_t *sensor_info , OPR_Mode_select *mode)
{

	mode->OPR_mode_bits.reserved = 0x00;
	mode->OPR_mode_bits.opr_mode = 0x00;

	HAL_I2C_Mem_Write(sensor_info->i2c, BNO055_I2C_address, OPR_MODE_ID, I2C_MEMADD_SIZE_8BIT, &mode->mode_value, sizeof(mode->mode_value), 100);

	HAL_Delay(20);
}

void AMG_Mode(sensor_info_t *sensor_info , OPR_Mode_select *mode)
{
	mode->OPR_mode_bits.reserved = 0x00;
	mode->OPR_mode_bits.opr_mode = 0x07;
	HAL_I2C_Mem_Write(sensor_info->i2c, BNO055_I2C_address, OPR_MODE_ID, I2C_MEMADD_SIZE_8BIT, &mode->mode_value, sizeof(mode->mode_value), 100);
	HAL_Delay(20);

}

void axis_remap(sensor_info_t *sensor_info)
{
	uint8_t data[2] =  {0x24 , 0x00};

	HAL_I2C_Mem_Write(sensor_info->i2c, BNO055_I2C_address, AXIS_MAP_CONFIG_ADDR, I2C_MEMADD_SIZE_8BIT, data, sizeof(data), 100);

	HAL_Delay(20);


}

uint8_t Read_BNO055(sensor_info_t *sensor_info , filter_calc_t *f , test_value *test )
{
	uint8_t raw_data[18];

	if (HAL_I2C_Mem_Read(sensor_info->i2c, BNO055_I2C_address, ACC_DATA_X_LSB, I2C_MEMADD_SIZE_8BIT, raw_data, sizeof(raw_data), 100) == HAL_OK)
	{
	   sensor_info->ivme[0]         = ((int16_t)(((uint16_t)raw_data[1] << 8) | raw_data[0])) / 100.0f;
	   sensor_info->ivme[1]         = ((int16_t)(((uint16_t)raw_data[3] << 8) | raw_data[2])) / 100.0f;
	   sensor_info->ivme[2]         = ((int16_t)(((uint16_t)raw_data[5] << 8) | raw_data[4])) / 100.0f;

	   filter_ivme(sensor_info, f);

	   sensor_info->manyetometre[0] = ((int16_t)(((uint16_t)raw_data[7]  << 8) | raw_data[6])) / 16.0f;
	   sensor_info->manyetometre[1] = ((int16_t)(((uint16_t)raw_data[9]  << 8) | raw_data[8])) / 16.0f;
	   sensor_info->manyetometre[2] = ((int16_t)(((uint16_t)raw_data[11] << 8) | raw_data[10])) / 16.0f;

	   soft_iron_calc(sensor_info, test);

	  filter_manyometre(sensor_info, f, test);

	   sensor_info->jiroskop[0]     = ((int16_t)(((uint16_t)raw_data[13] << 8) | raw_data[12])) / 16.0f;
	   sensor_info->jiroskop[1]     = ((int16_t)(((uint16_t)raw_data[15] << 8) | raw_data[14])) / 16.0f;
	   sensor_info->jiroskop[2]     = ((int16_t)(((uint16_t)raw_data[17] << 8) | raw_data[16])) / 16.0f;

	   return 1;

	}
	else
	{

		 HAL_I2C_DeInit(sensor_info->i2c);

		 I2C_Bus_Clear();

		 HAL_I2C_Init(sensor_info->i2c);

		 extern OPR_Mode_select mode;
		 extern Output_data_t output_config;

		 BNO055_Init(sensor_info, &mode, &output_config, f, test);

		 sensor_info->filtre_baslatildi = 0;
		 sensor_info->roll_pitch_baslatildi = 0;
		 sensor_info->filtre_baslatildi = 0;
		 sensor_info->baslangic_sayac = 0;

		 return 0;
	 }

}


void filter_Init(sensor_info_t *sensor_info, float fs, float fc, filter_calc_t *f)
{
    memset(f, 0, sizeof(filter_calc_t));

    float pi_number = 3.14159265f;
    float omega = 2.0f * pi_number * (fc / fs);

    f->alfa = sinf(omega) / 1.41421356f;

    float a0 = 1.0f + f->alfa;

    f->b0 = (1.0f - cosf(omega)) / (2.0f * a0);
    f->b1 = (1.0f - cosf(omega)) / a0;  // 2 * b0

    f->a1 = (-2.0f * cosf(omega)) / a0;
    f->a2 = (1.0f - f->alfa) / a0;
}



void filter_ivme(sensor_info_t *sensor_info , filter_calc_t *f)
{

	for(int i = 0; i< 3; i++)
	{
	    f->x0[i] = sensor_info->ivme[i];

	    sensor_info->filtreli_ivme_eksen[i] = (f->b0 * f->x0[i]) + (f->b1 * f->x1[i]) + (f->b0 * f->x2[i]) - (f->a1 * f->y1[i]) - (f->a2 * f->y2[i]);

	    f->x2[i] = f->x1[i];
	    f->x1[i] = f->x0[i];

	    f->y2[i] = f->y1[i];
	    f->y1[i] = sensor_info->filtreli_ivme_eksen[i];
	}

}
void filter_manyometre(sensor_info_t *sensor_info , filter_calc_t *f, test_value *test)
{
	for(int i = 0; i< 3; i++)
	{
		f->x3[i] = sensor_info->manyetometre_offset[i];

		sensor_info->filtreli_manyometre_eksen[i] = (f->b0 * f->x3[i]) + (f->b1 * f->x4[i])  + (f->b0 * f->x5[i]) - (f->a1 * f->y3[i]) - (f->a2 * f->y4[i]);

		f->x5[i] = f->x4[i];
		f->x4[i] = f->x3[i];

		f->y4[i] = f->y3[i];
		f->y3[i] =  sensor_info->filtreli_manyometre_eksen[i];
	}

}


void jiroskop_kayma(sensor_info_t *sensor_info, filter_calc_t *f)
{

	for(int i = 0; i < 3 ; i++)
	{
		if (fabsf(sensor_info->jiroskop[i] - sensor_info->kayma_jiroskop[i]) >= 8.5f)
		{
			sensor_info->sabit_flag = 0;
			sensor_info->ornek_sayi = 0;

			for(int i = 0; i < 3; i++)
			{
				sensor_info->kayma[i] = 0;
			}
			return;
		}
	}

    for (int j = 0; j < 3; j++)
    {

        if (sensor_info->ornek_sayi == 0)
        {
            sensor_info->ivme_min[j] = sensor_info->ivme[j];
            sensor_info->ivme_max[j] = sensor_info->ivme[j];

            sensor_info->jiroskop_min[j] = sensor_info->jiroskop[j];
            sensor_info->jiroskop_max[j] = sensor_info->jiroskop[j];
        }
        else
        {

            if (sensor_info->ivme[j] < sensor_info->ivme_min[j])
                sensor_info->ivme_min[j] = sensor_info->ivme[j];

            if (sensor_info->ivme[j] > sensor_info->ivme_max[j])
                sensor_info->ivme_max[j] = sensor_info->ivme[j];

            if (sensor_info->jiroskop[j] < sensor_info->jiroskop_min[j])
                sensor_info->jiroskop_min[j] = sensor_info->jiroskop[j];

            if (sensor_info->jiroskop[j] > sensor_info->jiroskop_max[j])
                sensor_info->jiroskop_max[j] = sensor_info->jiroskop[j];
        }

        sensor_info->kayma[j] += sensor_info->jiroskop[j];
    }

    sensor_info->ornek_sayi += 1;

    if (sensor_info->ornek_sayi >= 200)
    {
        for (int j = 0; j < 3; j++)
        {
            sensor_info->kayma_jiroskop_yedek[j] 	= sensor_info->kayma[j] / sensor_info->ornek_sayi;

            sensor_info->ivme_degisim[j] 			= sensor_info->ivme_max[j] - sensor_info->ivme_min[j];

            sensor_info->jiroskop_degisim[j]		= sensor_info->jiroskop_max[j] - sensor_info->jiroskop_min[j];

            sensor_info->kayma[j] = 0.0f;
        }
        sensor_info->sabit_flag = 1;

        for(int i = 0; i < 3 ; i++)
        {
        	if(sensor_info->ivme_degisim[i] >=  0.25f || sensor_info->jiroskop_degisim[i] >= 5.0f || fabsf(sensor_info->kayma_jiroskop_yedek[i]) >= 0.45f)
        		sensor_info->sabit_flag = 0;
        }

        if (sensor_info->sabit_flag == 1)
        {
            for (int i = 0; i < 3; i++)
            {
                sensor_info->kayma_jiroskop[i] += 0.1f * (sensor_info->kayma_jiroskop_yedek[i] - sensor_info->kayma_jiroskop[i]);
            }
        }

        sensor_info->ornek_sayi = 0;
    }
}




void sensor_init(sensor_info_t *sensor_info, test_value *test)
{
    // Hard-iron ofsetleri
	sensor_info->mag_hard_iron[0] = -10.5625f;
	sensor_info->mag_hard_iron[1] = 16.3125f;
	sensor_info->mag_hard_iron[2] = 14.34375f;

	test->manyometre_max[0] = 38.6875f;
	test->manyometre_max[1] = 58.3125f;
	test->manyometre_max[2] = 57.75f;

	test->manyometre_min[0] = -59.8125f;
	test->manyometre_min[1] = -25.6875f;
	test->manyometre_min[2] = -29.0625f;


}




void test_calibration(sensor_info_t *sensor_info , test_value *test  )
{

	for(int i = 0; i < 3 ;  i++)
	{
		if (test->ornek_sayi2 == 0)
		{
			test->manyometre_min[i] = sensor_info->manyetometre[i];
			test->manyometre_max[i] = sensor_info->manyetometre[i];
		}
		else
		{
			if(test->manyometre_min[i] > sensor_info->manyetometre[i])
				test->manyometre_min[i] = sensor_info->manyetometre[i];
			if(test->manyometre_max[i] < sensor_info->manyetometre[i])
				test->manyometre_max[i] = sensor_info->manyetometre[i];

		}
	}

	test->ornek_sayi2 += 1;


	if(test->ornek_sayi2 >= 1500)
	{
		for(int i = 0; i < 3; i++)
		{

			test->hard_iron_offset[i] = (test->manyometre_max[i] + test->manyometre_min[i]) / 2.0f;
		}

		test->ornek_sayi2 = 0;
		test->test_calib = 1;
	}


}

void soft_iron_calc(sensor_info_t *sensor_info, test_value *test)
{

	test->toplam_Yaricap  =
		((test->manyometre_max[0] - test->manyometre_min[0] ) / 2 ) +
		((test->manyometre_max[1] - test->manyometre_min[1] ) / 2 ) +
		((test->manyometre_max[2] - test->manyometre_min[2] ) / 2 );



	test->toplam_Yaricap /= 3;

	sensor_info->mag_soft_iron[0] = test->toplam_Yaricap / ((test->manyometre_max[0] - test->manyometre_min[0] ) / 2);
	sensor_info->mag_soft_iron[1] = test->toplam_Yaricap / ((test->manyometre_max[1] - test->manyometre_min[1] ) / 2);
	sensor_info->mag_soft_iron[2] = test->toplam_Yaricap / ((test->manyometre_max[2] - test->manyometre_min[2] ) / 2);


	for(int i = 0; i < 3; i++)
	{

		sensor_info->manyetometre_offset[i] = (sensor_info->manyetometre[i] - sensor_info->mag_hard_iron[i]) * sensor_info->mag_soft_iron[i];
	}

	sensor_info->mag_gecici = sqrtf((sensor_info->manyetometre_offset[0] * sensor_info->manyetometre_offset[0]) +
	                                    (sensor_info->manyetometre_offset[1] * sensor_info->manyetometre_offset[1]) +
	                                    (sensor_info->manyetometre_offset[2] * sensor_info->manyetometre_offset[2]));


}

void jiroskop_calc(sensor_info_t *sensor_info )
{

	for(int i = 0 ; i < 3 ; i++)
	{

		sensor_info->kaymasi_temizlenmis_jiroskop[i] = sensor_info->jiroskop[i] - sensor_info->kayma_jiroskop[i];

		if (fabsf(sensor_info->kaymasi_temizlenmis_jiroskop[i]) < 0.1f )
		{
			sensor_info->kaymasi_temizlenmis_jiroskop[i]= 0.0f;
		}

		sensor_info->donme_miktar[i] += sensor_info->kaymasi_temizlenmis_jiroskop[i] * sensor_info->dt;

		if(sensor_info->donme_miktar[i] >= 360.0f)
			sensor_info->donme_miktar[i] -= 360.0f;
		if(sensor_info->donme_miktar[i] < 0.0f)
			sensor_info->donme_miktar[i] += 360.0f;


	}
}

void kuzey_hesap(sensor_info_t *sensor_info)
{

	sensor_info->roll_acc=
			atan2f(sensor_info->filtreli_ivme_eksen[1] , sensor_info->filtreli_ivme_eksen[2]);

	sensor_info->pitch_acc =
			atan2f( -sensor_info->filtreli_ivme_eksen[0] , sqrtf((sensor_info->filtreli_ivme_eksen[1] * sensor_info->filtreli_ivme_eksen[1]) + (sensor_info->filtreli_ivme_eksen[2] * sensor_info->filtreli_ivme_eksen[2])));

	if(sensor_info->roll_pitch_baslatildi == 0)
	{
		sensor_info->roll = sensor_info->roll_acc;
		sensor_info->pitch = sensor_info->pitch_acc;
		sensor_info->roll_pitch_baslatildi = 1;

	}

	sensor_info->beta = sqrtf(( sensor_info->ivme[0] *sensor_info->ivme[0]) +  ( sensor_info->ivme[1] *sensor_info->ivme[1]) + ( sensor_info->ivme[2] *sensor_info->ivme[2]));

	if(sensor_info->beta > 8.8f && sensor_info->beta < 10.8f)
	{
		sensor_info->ivme_agirligi= 0.1f;
	}
	else
	{
		sensor_info->ivme_agirligi= 0;
	}

	sensor_info->roll += sensor_info->kaymasi_temizlenmis_jiroskop[0] * 0.017453f * sensor_info->dt;

	sensor_info->pitch += sensor_info->kaymasi_temizlenmis_jiroskop[1] * 0.017453f * sensor_info->dt;

	sensor_info->pitch = sensor_info->pitch *(1 - sensor_info->ivme_agirligi) + (sensor_info->pitch_acc * sensor_info->ivme_agirligi);

	float roll_hata = sensor_info->roll_acc - sensor_info->roll;
	roll_hata = atan2f(sinf(roll_hata), cosf(roll_hata));

	sensor_info->roll += sensor_info->ivme_agirligi * roll_hata;


	float Yh =
			(sensor_info->filtreli_manyometre_eksen[1] * cosf(sensor_info->roll)) -
			(sensor_info->filtreli_manyometre_eksen[2] * sinf(sensor_info->roll));

	float Xh =
			(sensor_info->filtreli_manyometre_eksen[0] * cosf(sensor_info->pitch)) +
			(sensor_info->filtreli_manyometre_eksen[1] * sinf(sensor_info->roll) * sinf(sensor_info->pitch)) +
			(sensor_info->filtreli_manyometre_eksen[2] * cosf(sensor_info->roll) * sinf(sensor_info->pitch) ) ;


	if(Xh == 0 && Yh == 0)
	{
		return;
	}
	else
	{
		sensor_info->D = -(atan2f(Yh, Xh)) * (180.0f / 3.14159265f);

		sensor_info->D += 5.5f;

		if(sensor_info->D < 0)
			sensor_info->D += 360;

		sensor_info->D = 360.0f - sensor_info->D;

		if(sensor_info->D >= 360.0f)
		    sensor_info->D -= 360.0f;

		complementary_Filter(sensor_info);
	}

}

void complementary_Filter(sensor_info_t *sensor_info)
{
	float yaw_hizi;


	if (sensor_info->filtre_baslatildi == 0)
	{
	    sensor_info->birlesik_aci = sensor_info->D;
	    sensor_info->tahmin_aci = sensor_info->D;
	    sensor_info->baslangic_aci = sensor_info->D;

	    sensor_info->yaw = 0.0f;   // İlk yanlış D gösterilmesin
	    sensor_info->baslangic_sayac = 0;
	    sensor_info->filtre_baslatildi = 1;

	    return;
	}

	//bunu dene
	sensor_info->w = sqrtf(
			(sensor_info->kaymasi_temizlenmis_jiroskop[0] * sensor_info->kaymasi_temizlenmis_jiroskop[0] ) +
			(sensor_info->kaymasi_temizlenmis_jiroskop[1] * sensor_info->kaymasi_temizlenmis_jiroskop[1] ) +
			(sensor_info->kaymasi_temizlenmis_jiroskop[2] * sensor_info->kaymasi_temizlenmis_jiroskop[2] ));





	if(fabsf(cosf(sensor_info->pitch)) <  0.017f )
	{
		yaw_hizi = 0;
	}
	else
	{
		yaw_hizi = -(
					((sensor_info->kaymasi_temizlenmis_jiroskop[1] * sinf(sensor_info->roll)) +
						(sensor_info->kaymasi_temizlenmis_jiroskop[2] * cosf(sensor_info->roll)) ) / cosf(sensor_info->pitch));
	}
	sensor_info->tahmin_aci = sensor_info->birlesik_aci + yaw_hizi * sensor_info->dt;



	sensor_info->hata = sensor_info->D - sensor_info->tahmin_aci;

	if(sensor_info->hata < -180)
		sensor_info->hata += 360;
	if(sensor_info->hata >= 180)
		sensor_info->hata -= 360;

	if(sensor_info->filtre_baslatildi == 1)
	{
		if(sensor_info->mag_gecici > 43.5f && sensor_info->mag_gecici < 48.5f)
		{
			sensor_info->alpha= 0.1f;

			if(sensor_info->baslangic_sayac >=20)
			{
				sensor_info->birlesik_aci = sensor_info->tahmin_aci + sensor_info->alpha * sensor_info->hata;

				if(sensor_info->birlesik_aci < 0)
					sensor_info->birlesik_aci += 360;
				if(sensor_info->birlesik_aci >= 360)
					sensor_info->birlesik_aci -= 360;

				sensor_info->yaw = sensor_info->birlesik_aci;

				sensor_info->filtre_baslatildi = 2;
			}
			sensor_info->baslangic_sayac++;

		}
		else
		{
			sensor_info->baslangic_sayac = 0;
			sensor_info->alpha = 0;
			return;
		}




	}


		if(sensor_info->w > 3 && sensor_info->filtre_baslatildi == 2)
		{

			if(sensor_info->mag_gecici > 43.5f && sensor_info->mag_gecici < 48.5f)
			{
				sensor_info->alpha = 0.1f;
			}
			else
			{
				sensor_info->alpha = 0;
			}

			sensor_info->birlesik_aci = sensor_info->tahmin_aci + sensor_info->alpha * sensor_info->hata;

			if(sensor_info->birlesik_aci < 0)
				sensor_info->birlesik_aci += 360;
			if(sensor_info->birlesik_aci >= 360)
				sensor_info->birlesik_aci -= 360;


			sensor_info->yaw = sensor_info->birlesik_aci;
		}




}




void I2C_Bus_Clear(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // 1. PB6 (SCL) ve PB7 (SDA) pinlerini Open-Drain çıkış yap
    GPIO_InitStruct.Pin = GPIO_PIN_6 | GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_SET);

    for (int i = 0; i < 9; i++)
    {
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_RESET);
        HAL_Delay(1);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_SET);
        HAL_Delay(1);
    }

    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_RESET);
    HAL_Delay(1);

    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_SET);
    HAL_Delay(1);

    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_SET);
    HAL_Delay(1);
}
