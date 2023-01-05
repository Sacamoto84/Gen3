#include "Page.h"
#include "global_define.h"
#include "stdio.h"

#include "menu_typedef.h"

#include "Generator.h"
extern GENERATOR Gen;

#include "global_typedef.h"
extern Encoder_typedef Encoder;

float utils_fr_decrement_fr(float in)
{
	if (in >= 200.0F){
		in-=100;
		return in;
	}

	if (in >= 10.0F)  {in-=1.0F;    return in;}
	if (in >= 0.1F)
	{
		in-=0.1F;
	    if(in<0.1F)
	    	in = 0.1F;
	    return in;
	}

	return 9999.9F;

}

float utils_fr_increment(float in)
{
	if (in >= 100.0F){
		in+=100;

		if (in > 10000.0F ) in=10000.0F;
		return in;
	}

	if (in >= 10.0F)  {in+=1.0F;   return in;}
	if (in >= 0.1F)   {in+=0.1F;   return in;}
	return 9999.9F;
}


void generator_pre_CH1_FR(uint32_t index) {

	uint32_t temp;

	if (menu_generator.field.encoder_block) {

		///////////////////////////////////////
		if (index == INDEX_CH1_FR) //CH1 Carrier Fr
		{

			temp = Gen.CH1.Carrier_fr;
			if (Encoder.Left) {
				Encoder.Left = 0;
				temp -= 100;
				if (temp < 200)
					temp = 200;
				Gen.CH1.Carrier_fr = temp;
			}

			if (Encoder.Right) {
				Encoder.Right = 0;
				temp += 100;
				if (temp > 10000)
					temp = 10000;
				Gen.CH1.Carrier_fr = temp;
			}
			sprintf(item_generator[INDEX_CH1_FR].text, "* %d *",
					Gen.CH1.Carrier_fr);
		}

	} else {
		sprintf(item_generator[INDEX_CH1_FR].text, " %d Hz",
				Gen.CH1.Carrier_fr);
	}

}

void generator_pre_CH2_FR(uint32_t index) {

	uint32_t temp;

	if (menu_generator.field.encoder_block) {

		if (index == INDEX_CH2_FR) //CH2 Carrier Fr
		{
			temp = Gen.CH2.Carrier_fr;
			if (Encoder.Left) {
				Encoder.Left = 0;
				temp -= 100;
				if (temp < 200)
					temp = 200;
				Gen.CH2.Carrier_fr = temp;
			}

			if (Encoder.Right) {
				Encoder.Right = 0;
				temp += 100;
				if (temp > 4000)
					temp = 4000;
				Gen.CH2.Carrier_fr = temp;
			}
			sprintf(item_generator[INDEX_CH2_FR].text, "* %d *",
					Gen.CH2.Carrier_fr);
		}
		menu_generator.field.needUpdate = 1;
	} else {
		sprintf(item_generator[INDEX_CH2_FR].text, " %d Hz",
				Gen.CH2.Carrier_fr);
	}

}

void generator_pre_CH1_AM_FR(uint32_t index) {

	float tempf;

	if (menu_generator.field.encoder_block) {

		///////////////////////////////////////
		if (index == INDEX_CH1_AM_FR) //CH1 AM Fr
		{
			tempf = Gen.CH1.AM_fr;

			if (Encoder.Left) {
				Encoder.Left = 0;

				if (tempf <= 10.0F)
					tempf -= 0.1F;
				else
					tempf -= 1.0F;

				if (tempf < 0.1F)
					tempf = 0.1F;

				Gen.CH1.AM_fr = tempf;
			}

			if (Encoder.Right) {
				Encoder.Right = 0;

				if (tempf < 10.0F)
					tempf += 0.1F;
				else
					tempf += 1.0F;

				if (tempf > 100.0F)
					tempf = 100.0F;

				Gen.CH1.AM_fr = tempf;
			}
			sprintf(item_generator[INDEX_CH1_AM_FR].text, "> %.1f Hz <",
					Gen.CH1.AM_fr);
		}
		menu_generator.field.needUpdate = 1;
	} else {
		sprintf(item_generator[INDEX_CH1_AM_FR].text, " %.1f Hz",
				Gen.CH1.AM_fr);
	}

}

void generator_pre_CH2_AM_FR(uint32_t index) {

	float tempf;

	if (menu_generator.field.encoder_block) {

		///////////////////////////////////////
		if (index == INDEX_CH2_AM_FR) //CH1 AM Fr
		{
			tempf = Gen.CH2.AM_fr;

			if (Encoder.Left) {
				Encoder.Left = 0;

				if (tempf <= 10.0F)
					tempf -= 0.1F;
				else
					tempf -= 1.0F;

				if (tempf < 0.1F)
					tempf = 0.1F;

				Gen.CH2.AM_fr = tempf;
			}

			if (Encoder.Right) {
				Encoder.Right = 0;

				if (tempf < 10.0F)
					tempf += 0.1F;
				else
					tempf += 1.0F;

				if (tempf > 100.0F)
					tempf = 100.0F;

				Gen.CH2.AM_fr = tempf;
			}
			sprintf(item_generator[INDEX_CH2_AM_FR].text, "> %.1f Hz <",
					Gen.CH2.AM_fr);
		}
		menu_generator.field.needUpdate = 1;
	} else {
		sprintf(item_generator[INDEX_CH2_AM_FR].text, " %.1f Hz",
				Gen.CH2.AM_fr);
	}

}

void generator_pre_CH1_FM_BASE(uint32_t index) {

	uint32_t temp;

	if (menu_generator.field.encoder_block) {

		if (index == INDEX_CH1_FM_BASE) //CH1 FM Base
		{
			if (Encoder.Left) {
				Encoder.Left = 0;
				temp = Gen.CH1.FM_Base;
				temp -= 100;
				if (temp < 200)
					temp = 200;
				Gen.CH1.FM_Base = temp;
				Gen.Refresh_FM_Modulation1();
			}

			if (Encoder.Right) {
				Encoder.Right = 0;
				temp = Gen.CH1.FM_Base;
				temp += 100;
				if (temp > 5000)
					temp = 5000;
				Gen.CH1.FM_Base = temp;
				Gen.Refresh_FM_Modulation1();
			}
			sprintf(item_generator[INDEX_CH1_FM_BASE].text, " Base * %d *",
					Gen.CH1.FM_Base);



		}
		menu_generator.field.needUpdate = 1;
	} else {
		sprintf(item_generator[INDEX_CH1_FM_BASE].text, " Base %d Hz",
				Gen.CH1.FM_Base);
	}
}

void generator_pre_CH2_FM_BASE(uint32_t index) {

	uint32_t temp;

	if (menu_generator.field.encoder_block) {

		if (index == INDEX_CH2_FM_BASE) //CH1 FM Base
		{
			if (Encoder.Left) {
				Encoder.Left = 0;
				temp = Gen.CH2.FM_Base;
				temp -= 100;
				if (temp < 200)
					temp = 200;
				Gen.CH2.FM_Base = temp;
				Gen.Refresh_FM_Modulation2();
			}

			if (Encoder.Right) {
				Encoder.Right = 0;
				temp = Gen.CH2.FM_Base;
				temp += 100;
				if (temp > 5000)
					temp = 5000;
				Gen.CH2.FM_Base = temp;
				Gen.Refresh_FM_Modulation2();
			}
			sprintf(item_generator[INDEX_CH2_FM_BASE].text, " Base * %d *",
					Gen.CH2.FM_Base);



		}
		menu_generator.field.needUpdate = 1;
	} else {
		sprintf(item_generator[INDEX_CH2_FM_BASE].text, " Base %d Hz",
				Gen.CH2.FM_Base);
	}
}

void generator_pre_CH1_FM_DEV(uint32_t index) {

	float tempf;

	if (menu_generator.field.encoder_block) {

		if (index == INDEX_CH1_FM_DEV) //CH1 FM Dev
		{
			if (Encoder.Left) {
				Encoder.Left = 0;
				tempf = Gen.CH1.FM_Dev;
				tempf = utils_fr_decrement_fr(tempf);
				Gen.CH1.FM_Dev = tempf;
				Gen.Refresh_FM_Modulation1();
			}

			if (Encoder.Right) {
				Encoder.Right = 0;
				tempf = Gen.CH1.FM_Dev;
				tempf = utils_fr_increment(tempf);
				Gen.CH1.FM_Dev = tempf;
				Gen.Refresh_FM_Modulation1();
			}

            if (Gen.CH1.FM_Dev >= 10.0F)
			  sprintf(item_generator[INDEX_CH1_FM_DEV].text, " Dev * %.0f *", Gen.CH1.FM_Dev);
            else
              sprintf(item_generator[INDEX_CH1_FM_DEV].text, " Dev * %.1f *", Gen.CH1.FM_Dev);

		}
		menu_generator.field.needUpdate = 1;


	}
	else
	{
		if (Gen.CH1.FM_Dev >= 10.0F)
		  sprintf(item_generator[INDEX_CH1_FM_DEV].text, " Dev %.0f Hz", Gen.CH1.FM_Dev);
		else
		  sprintf(item_generator[INDEX_CH1_FM_DEV].text, " Dev %.1f Hz", Gen.CH1.FM_Dev);
	}

}

void generator_pre_CH2_FM_DEV(uint32_t index) {

	float tempf;

	if (menu_generator.field.encoder_block) {

		if (index == INDEX_CH2_FM_DEV) //CH1 FM Dev
		{
			if (Encoder.Left) {
				Encoder.Left = 0;
				tempf = Gen.CH2.FM_Dev;
				tempf = utils_fr_decrement_fr(tempf);
				Gen.CH2.FM_Dev = tempf;
				Gen.Refresh_FM_Modulation2();
			}

			if (Encoder.Right) {
				Encoder.Right = 0;
				tempf = Gen.CH2.FM_Dev;
				tempf = utils_fr_increment(tempf);
				Gen.CH2.FM_Dev = tempf;
				Gen.Refresh_FM_Modulation2();
			}

            if (Gen.CH2.FM_Dev >= 10.0F)
			  sprintf(item_generator[INDEX_CH2_FM_DEV].text, " Dev * %.0f *", Gen.CH2.FM_Dev);
            else
              sprintf(item_generator[INDEX_CH2_FM_DEV].text, " Dev * %.1f *", Gen.CH2.FM_Dev);

		}
		menu_generator.field.needUpdate = 1;


	}
	else
	{
		if (Gen.CH2.FM_Dev >= 10.0F)
		  sprintf(item_generator[INDEX_CH2_FM_DEV].text, " Dev %.0f Hz", Gen.CH2.FM_Dev);
		else
		  sprintf(item_generator[INDEX_CH2_FM_DEV].text, " Dev %.1f Hz", Gen.CH2.FM_Dev);
	}
}

void generator_pre_CH1_FM_FR(uint32_t index) {

	float tempf;

	if (menu_generator.field.encoder_block) {

		if (index == INDEX_CH1_FM_FR) //CH1 FM_mod_fr
		{
			tempf = Gen.CH1.FM_mod_fr;

			if (Encoder.Left) {
				Encoder.Left = 0;
				if (tempf <= 10.0F)
					tempf -= 0.1F;
				else
					tempf -= 1.0F;
				if (tempf < 0.1F)
					tempf = 0.1F;
				Gen.CH1.FM_mod_fr = tempf;
			}

			if (Encoder.Right) {
				Encoder.Right = 0;

				if (tempf < 10.0F)
					tempf += 0.1F;
				else
					tempf += 1.0F;
				if (tempf > 100.0F)
					tempf = 100.0F;
				Gen.CH1.FM_mod_fr = tempf;
			}
			sprintf(item_generator[INDEX_CH1_FM_FR].text, "* %.1f Hz *",
					Gen.CH1.FM_mod_fr);
		}

		menu_generator.field.needUpdate = 1;

	} else {
		sprintf(item_generator[INDEX_CH1_FM_FR].text, " %.1f Hz",
				Gen.CH1.FM_mod_fr);
	}
}

void generator_pre_CH2_FM_FR(uint32_t index) {

	float tempf;

	if (menu_generator.field.encoder_block) {

		if (index == INDEX_CH2_FM_FR) //CH1 FM_mod_fr
		{
			tempf = Gen.CH2.FM_mod_fr;

			if (Encoder.Left) {
				Encoder.Left = 0;
				if (tempf <= 10.0F)
					tempf -= 0.1F;
				else
					tempf -= 1.0F;
				if (tempf < 0.1F)
					tempf = 0.1F;
				Gen.CH2.FM_mod_fr = tempf;
			}

			if (Encoder.Right) {
				Encoder.Right = 0;
				if (tempf < 10.0F)
					tempf += 0.1F;
				else
					tempf += 1.0F;
				if (tempf > 100.0F)
					tempf = 100.0F;
				Gen.CH2.FM_mod_fr = tempf;
			}
			sprintf(item_generator[INDEX_CH2_FM_FR].text, "* %.1f Hz *",
					Gen.CH2.FM_mod_fr);
		}
		menu_generator.field.needUpdate = 1;
	} else {
		sprintf(item_generator[INDEX_CH2_FM_FR].text, " %.1f Hz",
				Gen.CH2.FM_mod_fr);
	}
}

void generator_pre_CH1_EN(uint32_t index)
{
	if (Gen.CH1.CH_EN)
	  sprintf(item_generator[INDEX_CH1_EN].text,"          CH1 --");
	else
	  sprintf(item_generator[INDEX_CH1_EN].text,"          CH1 --");
}

void generator_pre_CH2_EN(uint32_t index)
{
	if (Gen.CH2.CH_EN)
	  sprintf(item_generator[INDEX_CH2_EN].text,"         -- CH 2");
	else
	  sprintf(item_generator[INDEX_CH2_EN].text,"         -- CH 2");
}

void generator_pre_CH1_AM_EN(uint32_t index)
{
	if (Gen.CH1.AM_EN)
	  sprintf(item_generator[INDEX_CH1_AM_EN].text,"         -- AM 1");
	else
	  sprintf(item_generator[INDEX_CH1_AM_EN].text,"         -- AM 1");
}

void generator_pre_CH2_AM_EN(uint32_t index)
{
	if (Gen.CH2.AM_EN)
	  sprintf(item_generator[INDEX_CH2_AM_EN].text,"         -- AM 2");
	else
	  sprintf(item_generator[INDEX_CH2_AM_EN].text,"         -- AM 2");
}

void generator_pre_CH1_FM_EN(uint32_t index)
{
	if (Gen.CH1.FM_EN)
	  sprintf(item_generator[INDEX_CH1_FM_EN].text,"         -- FM 1");
	else
	  sprintf(item_generator[INDEX_CH1_FM_EN].text,"         -- FM 1");
}

void generator_pre_CH2_FM_EN(uint32_t index)
{
	if (Gen.CH2.FM_EN)
	  sprintf(item_generator[INDEX_CH2_FM_EN].text,"         -- FM 2");
	else
	  sprintf(item_generator[INDEX_CH2_FM_EN].text,"         -- FM 2");
}




