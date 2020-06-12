// DX7 sysex to XFM2 sysex file converter
// R Heslip June 2020
// based on
//      DX72Csound by Jeff Harrington  idealord@dorsai.org           
//       After Models by Russell Pinkston                                 
//====================================================================

#include "stdio.h"
#include "stdlib.h"
#include "XFM2_params.h"

struct dx7_operator {
unsigned char OP_EG_R1;   
unsigned char OP_EG_R2;     
unsigned char OP_EG_R3;      
unsigned char OP_EG_R4;      
unsigned char OP_EG_L1;     
unsigned char OP_EG_L2;     
unsigned char OP_EG_L3;     
unsigned char OP_EG_L4;     
unsigned char LEV_SCL_BRK_PT; 
unsigned char SCL_LEFT_DEPTH;
unsigned char SCL_RIGHT_DEPTH;
unsigned char SCL_LEFT_CURVE:2;
unsigned char SCL_RIGHT_CURVE:6;
unsigned char RATE_SCALE:3;
unsigned char OSC_DETUNE:5;
unsigned char AMP_MOD_SENS:2;
unsigned char KEY_VEL_SENS:6;
unsigned char OUTPUT_LEV; 
unsigned char OSC_MODE:1;
unsigned char FREQ_COARSE:7;
unsigned char FREQ_FINE;
} operator[6];

struct dx7_globals {
unsigned char PITCH_EG_R1;  
unsigned char PITCH_EG_R2;  
unsigned char PITCH_EG_R3;   
unsigned char PITCH_EG_R4;   
unsigned char PITCH_EG_L1;   
unsigned char PITCH_EG_L2;   
unsigned char PITCH_EG_L3;   
unsigned char PITCH_EG_L4;   
unsigned char ALGORITHM; 
unsigned char FEEDBACK:3;
unsigned char OSC_KEY_SYNC:5;   // **** what is this parameter? 
unsigned char LFO_SPEED;
unsigned char LFO_DELAY;     
unsigned char LF_PT_MOD_DEP; 
unsigned char LF_AM_MOD_DEP; 
unsigned char SYNC:1;
unsigned char WAVE:3;
unsigned char LF_PT_MOD_SNS:4;
unsigned char TRANSPOSE;
char NAME[10];
} global;

#define SCALE99 255/99  // scale factor for DX7 0-99 values to XFM2 0-255 values
#define LEVELSCALE 230/99  // scale factor for DX7 0-99 level values to XFM2 values - levels are a little high if mapped directly
#define FEEDBACKSCALE 255/7  // scale factor for DX7 0-7 feedback values to XFM2 values 
#define FINESCALE 150/50  // scale factor for DX7 0-99 fine pitch values to XFM2 values this ratio seems to sound good for the organ and clav patches
#define LFOSPEEDSCALE 115/30  // scale factor for DX7 0-99 lfo speed to XFM2 values  - this ratio seems to sound good for the koto patch
#define LFODEPTHSCALE 7/17  // scale factor for DX7 0-99 lfo depth to XFM2 values  - this ratio seems to sound good for the koto patch

// maps DX7 algorithms to XFM2 ALGO bits
struct algmap {
	unsigned char algo0;
	unsigned char algo1;
	unsigned char algo2;
	unsigned char algo3;
	unsigned char algo4;
	unsigned char algo5;
} DX7toXFM2alg[32] = // Rene's mapping corrected by RH - was missing feedback bits, wrong ops in some cases
{
	// op1      op2       op3       op4       op5      op6
	0b0000101,0b0000000,0b0010001,0b0100000,0b1000000,0b0000000, // algo 1
	0b0000101,0b0000100,0b0010001,0b0100000,0b1000000,0b0000000, // algo 2
	0b0000101,0b0001000,0b0010000,0b0100000,0b1000000,0b1000000, // algo 3
	0b0000101,0b0001000,0b0000000,0b0100001,0b1000000,0b0010000, // algo 4
	0b0000101,0b0000000,0b0010001,0b0000000,0b1000001,0b1000000, // algo 5
	0b0000101,0b0000000,0b0010001,0b0000000,0b1000001,0b0100000, // algo 6
	0b0000101,0b0000000,0b0110001,0b0000000,0b1000000,0b1000000, // algo 7
	0b0000101,0b0000000,0b0110001,0b0010000,0b1000000,0b0000000, // algo 8
	0b0000101,0b0000100,0b0110001,0b0000000,0b1000000,0b0000000, // algo 9
	0b0000101,0b0001000,0b0001000,0b1100001,0b0000000,0b0000000, // algo 10
	0b0000101,0b0001000,0b0000000,0b1100001,0b0000000,0b1000000, // algo 11
	0b0000101,0b0000100,0b1110001,0b0000000,0b0000000,0b0000000, // algo 12	
	0b0000101,0b0000000,0b1110001,0b0000000,0b0000000,0b1000000, // algo 13	
	0b0000101,0b0000000,0b0010001,0b1100000,0b0000000,0b1000000, // algo 14
	0b0000101,0b0000100,0b0010001,0b1100000,0b0000000,0b0000000, // algo 15
	0b0101101,0b0000000,0b0010000,0b0000000,0b1000000,0b1000000, // algo 16
	0b0101101,0b0000100,0b0010000,0b0000000,0b1000000,0b0000000, // algo 17	
	0b0011101,0b0000000,0b0001000,0b0100000,0b1000000,0b0000000, // algo 18	
	0b0000101,0b0001000,0b0000000,0b1000001,0b1000001,0b1000000, // algo 19	
	0b0001001,0b0001001,0b0001000,0b1100001,0b0000000,0b0000000, // algo 20	
	0b0001001,0b0001001,0b0001000,0b1000001,0b1000001,0b0000000, // algo 21	
	0b0000101,0b0000000,0b1000001,0b1000001,0b1000001,0b1000000, // algo 22	
	0b0000001,0b0001001,0b0000000,0b1000001,0b1000001,0b1000000, // algo 23	
	0b0000001,0b0000001,0b1000001,0b1000001,0b1000001,0b1000000, // algo 24	
	0b0000001,0b0000001,0b0000001,0b1000001,0b1000001,0b1000000, // algo 25	
	0b0000001,0b0001001,0b0000000,0b1100001,0b0000000,0b1000000, // algo 26		
	0b0000001,0b0001001,0b0001000,0b1100001,0b0000000,0b0000000, // algo 27	
	0b0000101,0b0000000,0b0010001,0b0100000,0b0100000,0b0000001, // algo 28	
	0b0000001,0b0000001,0b0010001,0b0000000,0b1000001,0b1000000, // algo 29	
	0b0000001,0b0000001,0b0010001,0b0100000,0b0100000,0b0000001, // algo 30	
	0b0000001,0b0000001,0b0000001,0b0000001,0b1000001,0b1000000, // algo 31
	0b0000001,0b0000001,0b0000001,0b0000001,0b0000001,0b1000001, // algo 32	
};  

// maps feedback to the correct xfm2 op by algorithm
unsigned char feedback[32] = {
	6,  // algo 1
	2,
	6,
	6,
	6,
	6, // algo 6
	6,
	4,
	2, // algo 9
	3,
	6,
	2, // algo 12
	6,
	6,
	2,	// algo 15
	6,
	2,
	3, // algo 18
	6,
	3,
	3,
	6, // algo 22
	6,
	6,
	6, // algo 25
	6,
	3,
	5, // algo 28
	6,
	5,
	6,
	6  // algo 32
};
	
	
// this is the "init" state of the xfm2 parameters as captured by the xfm2 editor
// we overwrite these with the DX7 values with appropriate scaling and conversion
// note that array element 0 was added because the header file has parameter numbers, not zero based array indices
// 
//
unsigned char xfm2parms[513] = 
{0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,0,0,0,0,0,0,
128,128,128,128,128,128,255,255,255,255,255,255,0,0,
0,0,0,0,60,60,60,60,60,60,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,255,255,255,255,255,255,0,255,
255,255,255,255,255,0,255,255,255,255,255,255,0,0,0,
0,0,0,0,0,230,230,230,230,230,230,0,255,255,255,255,
255,255,0,255,255,255,255,255,160,0,160,160,160,160,
160,160,128,128,128,128,255,255,255,255,0,0,0,0,0,0,
0,0,0,0,0,0,0,100,0,0,255,30,0,30,255,255,255,255,
255,255,255,0,0,0,0,0,0,0,2,2,24,0,0,0,0,0,200,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,255,255,255,255,255,
255,128,0,128,255,0,0,0,0,0,0,0,0,128,128,128,0,127,
255,255,255,255,255,255,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,255,0,0,0,0,0,0,0,0,0,255,255,255,255,255,255,
255,255,255,255,255,255,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,
1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,255,0,0,128,0,255,0,
100,1,0,255,0,0,128,10,0,128,2,128,0,255,0,0,0,0,0,0,0,
0,0,10,0,0,120,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,255,0,0,
0,0,0,0,0,0,0,255,0,0,50,0,0,128,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,255,0,0,100,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,200,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,5,5,0,100,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0
};

unsigned char xfm2header[5] = {0xf0,0x43,0x0,0x0,0x0};  // xfm2 sysex header
unsigned char xfm2midibytes[2];  // parameters are saved in XFM2 sysex file as high byte low byte midi format
unsigned char xfm2trailer[1]= {0xf7};  // end of sysex

main(argc,argv)
int argc;
char *argv[];
{
FILE *fpin,*fpout;
unsigned short x,y,z,SCL_LEFT_CURVE,SCL_RIGHT_CURVE,OSC_DETUNE,OSC_RATE_SCALE;
unsigned short KEY_VEL_SENS,AMP_MOD_SENS,FREQ_COARSE,OSC_MODE;
char header[6];
char out[20];
int total, count=0,op=0,xop=0,p;
float frequency;
char name[11];
if((fpin=fopen(argv[1],"rb"))==0)  {
	printf("Cannot open file %s\n",argv[1]);
	exit(1);
	}

fread(&header, 6L, 1, fpin);

for (count=0;count<32;count++) {
  op=0;
  fread((&(operator[op])), 17L, 6, fpin);
  fread(&global, 16L, 1, fpin);
  fread(&name, 10, 1, fpin);
  name[10] = '\0';
  printf("\nPatch %d %s ALG %02d\n",count+1,name,global.ALGORITHM+1);
 
// note that we don't have to set all of the XFM2 parameters - parameter array is preset to "init" state 
// DX7 parameters are scaled and mapped to the corresponding XFM2 parameter

// map DX7 algorithm to XFM2 OP values - XFM2 can do any algorithm, DX7 does only 32

	xfm2parms[PRM_ALGO0]=DX7toXFM2alg[global.ALGORITHM].algo0;
	xfm2parms[PRM_ALGO1]=DX7toXFM2alg[global.ALGORITHM].algo1;
	xfm2parms[PRM_ALGO2]=DX7toXFM2alg[global.ALGORITHM].algo2;
	xfm2parms[PRM_ALGO3]=DX7toXFM2alg[global.ALGORITHM].algo3;
	xfm2parms[PRM_ALGO4]=DX7toXFM2alg[global.ALGORITHM].algo4;
	xfm2parms[PRM_ALGO5]=DX7toXFM2alg[global.ALGORITHM].algo5;	

// pitch env gen dx7 50 = no pitch shift, XFM2 128 = no pitch shift
	xfm2parms[PRM_L0_P]=(unsigned char)((unsigned)global.PITCH_EG_L4*SCALE99);  // xfm2 L0 L4 L5 = dx7 L4 - start and end level
	xfm2parms[PRM_L4_P]=(unsigned char)((unsigned)global.PITCH_EG_L4*SCALE99); 
//	xfm2parms[PRM_L5_0]=(unsigned char)((unsigned)global.PITCH_EG_L4*SCALE99);  // leave this as zero so envelope ends at 128
	xfm2parms[PRM_L1_P]=(unsigned char)((unsigned)global.PITCH_EG_L1*SCALE99);  // L1 - attack level
	xfm2parms[PRM_L2_P]=(unsigned char)((unsigned)global.PITCH_EG_L2*SCALE99);  // L2 - end of decay1
	xfm2parms[PRM_L3_P]=(unsigned char)((unsigned)global.PITCH_EG_L3*SCALE99);  // L3 sustain	
	xfm2parms[PRM_R1_P]=(unsigned char)((unsigned)global.PITCH_EG_R1*SCALE99);  // R1 - attack rate
	xfm2parms[PRM_R2_P]=(unsigned char)((unsigned)global.PITCH_EG_R2*SCALE99);  // R2 - decay 1 rate
	xfm2parms[PRM_R3_P]=(unsigned char)((unsigned)global.PITCH_EG_R3*SCALE99);  // R3 - decay 2 rate
	xfm2parms[PRM_R4_P]=(unsigned char)((unsigned)global.PITCH_EG_R4*SCALE99);  // R4 - release rate

// lfo parameters
	xfm2parms[PRM_LFO_SPEED ]=(unsigned char)((unsigned)global.LFO_SPEED*LFOSPEEDSCALE);  // lfo speed
	xfm2parms[PRM_LFO_FADE ]=(unsigned char)((unsigned)global.LFO_DELAY*SCALE99);  // lfo delay
	xfm2parms[PRM_LFO_PITCH_DEPTH ]=(unsigned char)((unsigned)global.LF_PT_MOD_DEP*LFODEPTHSCALE);  // lfo pitch mod depth
	xfm2parms[PRM_LFO_AMP_DEPTH]=(unsigned char)((unsigned)global.LF_AM_MOD_DEP*SCALE99);  // lfo amp mod depth
	xfm2parms[PRM_LFO_WAVE]=global.WAVE;  // lfo waveform ***** does DX7 map directly to XFM2 ?
	xfm2parms[PRM_LFO_SYNC]=global.SYNC;  // lfo sync

	
	xfm2parms[PRM_TRANSPOSE]=global.TRANSPOSE;  // transpose
	
	
// convert operator levels, rates etc
// note that ops are read from DX7 sysex in the order 6,5,4,3,2,1 but we convert them 1,2,3,4,5,6

	for (op=0;op<6;op++) {
		xop=5-op;  // change DX7 sysex order to incrementing order
		xfm2parms[PRM_LEVEL0 +xop]=(unsigned char)((unsigned)operator[op].OUTPUT_LEV*LEVELSCALE); // op level 
		xfm2parms[PRM_L0_0 +xop]=(unsigned char)((unsigned)operator[op].OP_EG_L4*SCALE99);  // xfm2 L0 L4 L5 = dx7 L4 - start and end level
		xfm2parms[PRM_L4_0 +xop]=(unsigned char)((unsigned)operator[op].OP_EG_L4*SCALE99); 
//		xfm2parms[PRM_L5_0 +xop]=(unsigned char)((unsigned)operator[op].OP_EG_L4*SCALE99);  // leave this as zero so envelope ends at zero
		xfm2parms[PRM_L1_0 +xop]=(unsigned char)((unsigned)operator[op].OP_EG_L1*SCALE99);  // L1 - attack level
		xfm2parms[PRM_L2_0 +xop]=(unsigned char)((unsigned)operator[op].OP_EG_L2*SCALE99);  // L2 - end of decay1
		xfm2parms[PRM_L3_0 +xop]=(unsigned char)((unsigned)operator[op].OP_EG_L3*SCALE99);  // L3 sustain	
		xfm2parms[PRM_R1_0 +xop]=(unsigned char)((unsigned)operator[op].OP_EG_R1*SCALE99);  // R1 - attack rate
		xfm2parms[PRM_R2_0 +xop]=(unsigned char)((unsigned)operator[op].OP_EG_R2*SCALE99);  // R2 - decay 1 rate
		xfm2parms[PRM_R3_0 +xop]=(unsigned char)((unsigned)operator[op].OP_EG_R3*SCALE99);  // R3 - decay 2 rate
		xfm2parms[PRM_R4_0 +xop]=(unsigned char)((unsigned)operator[op].OP_EG_R4*SCALE99);  // R4 - release rate
		xfm2parms[PRM_RATIO0 +xop]=(unsigned char)((unsigned)operator[op].FREQ_COARSE);  // coarse frequency	
		xfm2parms[PRM_RATIO_FINE0 +xop]=(unsigned char)((unsigned)operator[op].FREQ_FINE*FINESCALE);  // fine frequency			
		xfm2parms[PRM_FINE0 +xop]=(unsigned char)(128+(int)(operator[op].OSC_DETUNE-7));  // detune dx7 value 7= no detune XFM2 value 128= no detune **** this seems to work about right with no scaling
		if ((xop+1)==feedback[global.ALGORITHM]) xfm2parms[PRM_FEEDBACK0 +xop]=(unsigned char)((unsigned)global.FEEDBACK*FEEDBACKSCALE);  // set op feedback level for this algorithm
		else xfm2parms[PRM_FEEDBACK0 +xop]=0;
		xfm2parms[PRM_KEY_BP0 +xop]=operator[op].LEV_SCL_BRK_PT;  // key breakpoint	
		xfm2parms[PRM_KEY_LDEPTH0 +xop]=(unsigned char)((unsigned)operator[op].SCL_LEFT_DEPTH*SCALE99);  // left scale depth	
		xfm2parms[PRM_KEY_RDEPTH0 +xop]=(unsigned char)((unsigned)operator[op].SCL_RIGHT_DEPTH*SCALE99);  // right scale depth
		xfm2parms[PRM_KEY_LCURVE0 +xop]=operator[op].SCL_LEFT_CURVE;  // key curve	left **** not sure if this is 1:1 map between dx7 and XFM2
		xfm2parms[PRM_KEY_RCURVE0 +xop]=operator[op].SCL_RIGHT_CURVE;  // key curve	right
		xfm2parms[PRM_RATE_KEY0 +xop]=operator[op].RATE_SCALE;  // DX7 key rate scale 0-7 **** what is the range of this parameter on the XFM2
		xfm2parms[PRM_LFO_AMS0 +xop]=operator[op].AMP_MOD_SENS;  // DX7 amplitude mod sensitivity 0-7 **** what is the range of this parameter on the XFM2
		xfm2parms[PRM_VEL_SENS0 +xop]=operator[op].KEY_VEL_SENS;  // DX7 key velocity sensitivity 0-7 **** what is the range of this parameter on the XFM2
		// **** still have to implement OSC_MODE - its bitmapped in XFM2
	}
	
  for (op=5;op>=0;op--) {
    printf("OP%d:    \n",5-op+1);
    printf("Level %d    ",operator[op].OUTPUT_LEV);
    printf("KEY_VEL_SENS %d   \n",operator[op].KEY_VEL_SENS);
    printf("R1 %d   ",operator[op].OP_EG_R1);   
    printf("R2 %d   ",operator[op].OP_EG_R2);     
    printf("R3 %d   ",operator[op].OP_EG_R3);      
    printf("R4 %d   \n",operator[op].OP_EG_R4);
    printf("L1 %d   ",operator[op].OP_EG_L1);         
    printf("L2 %d   ",operator[op].OP_EG_L2);      
    printf("L3 %d   ",operator[op].OP_EG_L3);    
    printf("L4 %d   \n",operator[op].OP_EG_L4); 
    printf("BRK PT %d    ",operator[op].LEV_SCL_BRK_PT);
    printf("SCL L %d    ",operator[op].SCL_LEFT_DEPTH); 
	printf("SCL R %d    ",operator[op].SCL_RIGHT_DEPTH); 
	printf("SCL L CRV %d    ",operator[op].SCL_LEFT_CURVE);
	printf("SCL R CRV %d    \n",operator[op].SCL_RIGHT_CURVE);
    printf("AMP_MOD_SENS %d   ",operator[op].AMP_MOD_SENS);
    printf("OSC_MODE %d   ",operator[op].OSC_MODE);
//    frequency = FreqTableCoarse[operator[op].OSC_MODE][operator[op].FREQ_COARSE] * FreqTableFine[operator[op].OSC_MODE][operator[op].FREQ_FINE];
    printf("FREQ_COARSE %d   ",operator[op].FREQ_COARSE);
    printf("FREQ_FINE   %d   \n",operator[op].FREQ_FINE);
	printf("OSC_DETUNE  %d   ",operator[op].OSC_DETUNE);
    printf("OSC_RATE_SCALE  %d   \n",operator[op].RATE_SCALE);
  }
  
  
  printf("Globals:\n");
  printf("LFO_SPEED %d \n",global.LFO_SPEED);
  printf("LFO_DELAY %d ",global.LFO_DELAY);
  printf("LF_PT_MOD_DEP %d \n",global.LF_PT_MOD_DEP);
  printf("PITCH L1 %d L2 %d L3 %d L4 %d R1 %d R2 %d R3 %d R4 %d\n",global.PITCH_EG_L1,global.PITCH_EG_L2,global.PITCH_EG_L3,global.PITCH_EG_L4,
	global.PITCH_EG_R1,global.PITCH_EG_R2,global.PITCH_EG_R3,global.PITCH_EG_R4);
  printf("FEEDBACK %d\n",global.FEEDBACK);
//  printf("Algorithm = %d\n",global.ALGORITHM+1);
//  printf("Patch name is %s\n\n",name);
//  printbottom(stdout);
//  fclose(stdout);


// use patch name for xfm2 patch filename
	for(p=0; p<10;p++) if (name[p]==' ') name[p]='_'; // remove spaces from name
	
	sprintf(out,"%s.syx",name);

	if ((fpout=fopen(out,"wb")) == 0) {
		printf("Couldn't open %s\n",out);
		exit(1);
    }
	
	fwrite(xfm2header,1,5,fpout);  // write xfm2 sysex header
	for(p=1; p<513;p++) {  // write 512 parameters
		if (xfm2parms[p] > 127) xfm2midibytes[0]=1; // set hi byte
		else xfm2midibytes[0]=0;
		xfm2midibytes[1]=xfm2parms[p] & 0x7f; // set low byte
		fwrite(xfm2midibytes,1,2,fpout);	
	}
	fwrite(xfm2trailer,1,1,fpout); 
	fclose(fpout);
  }

fclose(fpin);

}













