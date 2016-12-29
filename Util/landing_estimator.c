#include <stdlib.h>
#include <math.h>

#include "landing_estimator.h"

const uint16_t Transit_Times[50]=TRANSIT_INITIALISER;//Generated by matlab script using US1976 std atmosphere at 1km altitude incriments up to 49.5km

uint8_t Current_Bin=0;
int32_t Bin_Entry[2]={};//Entry co-ordinates to the current altitude bin
uint16_t Entry_Time=0;//In seconds
int16_t Horizontal_Drift[2]={};
float SLP_Velocity=5;//This needs to be initialised to the true velocity of the descending payload at SLP - use the init function

//A GPS position is submitted to this function
void process_new_GPS(Ubx_Gps_Type* GPS_pos) {
	uint8_t current_altitude_bin=((GPS_pos->mslaltitude)/1000000);//Altitude in km rounded down
	if(current_altitude_bin>Current_Bin) {//We have entered a new bin
		int32_t delta[2];
		delta[0]=GPS_pos->latitude-Bin_Entry[0];
		delta[1]=GPS_pos->longitude-Bin_Entry[1];
		Bin_Entry[0]=GPS_pos->latitude;
		Bin_Entry[1]=GPS_pos->longitude;
		uint32_t transit_time=(uint32_t)(GPS_pos->time/1000);//Index into the GPS week in seconds
		uint16_t transit_time_=*(uint16_t*)&transit_time;//Lower 16 bits
		int32_t delta_t=(int32_t)transit_time_-(int32_t)Entry_Time;
		Entry_Time=transit_time_;//16bit time
		if(delta_t<0) {//Wraparound occured
			if(!(transit_time&0xFFFF0000))//The current 16bit GPS interval is at the start of a new GPS week
				delta_t+=(24UL*3600UL*7UL)%(1<<16);//remainer at end of GPS week
			else
				delta_t+=(1<<16);//Wraparound was due to the overrun of a 16bit period
		}
		//Lookup table holds transit times for each layer in units of 0.1seconds as uint16_t. Assuming 1m/s descent at SLP. These are then scaled to 1km bins
		float transit_timef=(float)Transit_Times[Current_Bin]/(SLP_Velocity*10.0);//SLP_Velocity is in units of m/s
		transit_timef/=delta_t;//This is now a scaling factor for the horizontal drift
		float a=(float)delta[0]*transit_timef*0.001;//Latitude is easy to process, just divide by factor of 1000, to scale to 11m units
		Horizontal_Drift[0]+=(int16_t)a;//Add onto the integration bin, int16_t in 11m units can accomodate up to approx 350km of descent drift
		a=(float)delta[1]*transit_timef*0.001*cosf((float)GPS_pos->latitude*1e-7*M_PI/180.0);//The longitude also gets scaled by cos(lat) factor	
		Horizontal_Drift[1]+=(int16_t)a;
		Current_Bin=current_altitude_bin;
		if(Current_Bin>=50)	//Should not happen, but handle erranous values here
			Current_Bin=49;
		PWR_BackupAccessCmd(ENABLE);/* Allow access to BKP Domain, the rewrite all the backup state registers */
		BKP_WriteBackupRegister(BKP_DR4,Current_Bin);
		BKP_WriteBackupRegister(BKP_DR5,Horizontal_Drift[0]);
		BKP_WriteBackupRegister(BKP_DR6,Horizontal_Drift[1]);
		BKP_WriteBackupRegister(BKP_DR7,(uint16_t)((uint32_t)Bin_Entry[0])&0x0000FFFF);//Upper and lower 16 bit words of entry latitude and longitude
		BKP_WriteBackupRegister(BKP_DR8,((uint16_t*)&(Bin_Entry[0]))[1]);
		BKP_WriteBackupRegister(BKP_DR9,(uint16_t)((uint32_t)Bin_Entry[1])&0x0000FFFF);
		BKP_WriteBackupRegister(BKP_DR10,((uint16_t*)&(Bin_Entry[1]))[1]);
		BKP_WriteBackupRegister(BKP_DR11,Entry_Time);
		PWR_BackupAccessCmd(DISABLE);
	}

}

//This function takes the current GPS position and estimates the landing location
void correct_GPS_position(Ubx_Gps_Type* GPS_pos, int32_t landing[2]) {
	int32_t drift[2]={};		//Used to model the drift (within current layer and total)
	float indx=((float)(GPS_pos->mslaltitude)/1000000.0);//Floating point index
	indx-=(float)Current_Bin;	//Fractional index through the current bin
	if(indx>0.2) {			//Only try to do some further correction when we are more than 1/5th of the way into a layer
		drift[0]=GPS_pos->latitude-Bin_Entry[0];
		drift[1]=GPS_pos->longitude-Bin_Entry[1];	
		uint32_t transit_time=(uint32_t)(GPS_pos->time/1000);//Index into the GPS week in seconds
		uint16_t transit_time_=*(uint16_t*)&transit_time;//Lower 16 bits
		int32_t delta_t=(int32_t)transit_time_-(int32_t)Entry_Time;
		if(delta_t<0) {//Wraparound occured
			if(!(transit_time&0xFFFF0000))//The current 16bit GPS interval is at the start of a new GPS week
				delta_t+=(24UL*3600UL*7UL)%(1<<16);//remainer at end of GPS week
			else
				delta_t+=(1<<16);//Wraparound was due to the overrun of a 16bit period
		}
		float transit_timef=(float)Transit_Times[Current_Bin]/(SLP_Velocity*10.0);
		transit_timef=transit_timef*indx/delta_t;
		drift[0]=(int32_t)(transit_timef*(float)drift[0]);//Scale the drift
		drift[1]=(int32_t)(transit_timef*(float)drift[1]);
	}
	landing[0]=GPS_pos->latitude+(int32_t)Horizontal_Drift[0]*1000+drift[0];//Latitude is simple, no extra scaling
	landing[1]=GPS_pos->longitude+(int32_t)((float)Horizontal_Drift[1]*1000.0/cosf((float)GPS_pos->latitude*1e-7*M_PI/180.0))+drift[1];//Longitude is scaled
}

//This function is used for boot init of the landing drift predictor. It can be passed force_reset==true as argument to force reset
//The landing spot estimator uses BBRAM registers 4 through to 11
//4 contains the current integration bin, integration bins are 1km thick. Note that there is no DEM
//5 and 6 contain int16_t variables with North and East drift in int16_t units (scaled equatorial degree units, approx 11m units, or UBX scaled by 1/1000)
//7,8 hold entry latitude to the current drift measurement interval
//9,10 hold entry longitude
//11 holds entry time in seconds of week as uint16_t. This will roll around every few hours and so needs to be handled appropriatly
void initialise_landing_estimator(Ubx_Gps_Type* GPS_pos,uint8_t force_reset,float SLP_Vel) {
	SLP_Velocity=SLP_Vel;//The descent velocity at sea level pressure
	PWR_BackupAccessCmd(ENABLE);
	if(!force_reset) {
		Current_Bin=BKP_ReadBackupRegister(BKP_DR4);//This is the last _entered_ bin index
		Horizontal_Drift[0]=BKP_ReadBackupRegister(BKP_DR5);
		Horizontal_Drift[1]=BKP_ReadBackupRegister(BKP_DR6);
		Bin_Entry[0]=(int32_t)(((uint32_t)BKP_ReadBackupRegister(BKP_DR8)<<16)|(uint32_t)BKP_ReadBackupRegister(BKP_DR7));
		Bin_Entry[1]=(int32_t)(((uint32_t)BKP_ReadBackupRegister(BKP_DR10)<<16)|(uint32_t)BKP_ReadBackupRegister(BKP_DR9));
		Entry_Time=BKP_ReadBackupRegister(BKP_DR11);
	}
	else {
		Current_Bin=((GPS_pos->mslaltitude)/1000000);
		Horizontal_Drift[0]=0;
		Horizontal_Drift[1]=0;
		Bin_Entry[0]=GPS_pos->latitude;
		Bin_Entry[1]=GPS_pos->longitude;
		uint32_t transit_time=(uint32_t)(GPS_pos->time/1000);//Index into the GPS week in seconds
		Entry_Time=*(uint16_t*)&transit_time;
		BKP_WriteBackupRegister(BKP_DR4,Current_Bin);
		BKP_WriteBackupRegister(BKP_DR5,0);
		BKP_WriteBackupRegister(BKP_DR6,0);
		BKP_WriteBackupRegister(BKP_DR7,(uint16_t)((uint32_t)Bin_Entry[0])&0x0000FFFF);//Upper and lower 16 bit words of entry latitude and longitude
		BKP_WriteBackupRegister(BKP_DR8,((uint16_t*)&(Bin_Entry[0]))[1]);
		BKP_WriteBackupRegister(BKP_DR9,(uint16_t)((uint32_t)Bin_Entry[1])&0x0000FFFF);
		BKP_WriteBackupRegister(BKP_DR10,((uint16_t*)&(Bin_Entry[1]))[1]);
		BKP_WriteBackupRegister(BKP_DR11,Entry_Time);
	}
	PWR_BackupAccessCmd(DISABLE);
}

