#include "math.h"
#include <stdio.h>
#include "data.h"
//----------------------------------------------------------------------------

volatile cplx mData[3];

cplx Z,base;
measure_t mdata;

cplx __attribute__ ((section (".noinit"))) mAcc[3];

balance_data_t  __attribute__ ((section (".noinit"))) corr;
cal_data_t  __attribute__ ((section (".noinit")))  cal;// calibration constants

int __attribute__  ((section (".noinit"))) apo;
cplx __attribute__ ((section (".noinit"))) R;

int freq;
int __attribute__ ((section (".noinit"))) findex,cstatus;
uint32_t __attribute__ ((section (".noinit"))) bitParams;

int16_t  sine[N+N/4];
uint32_t  dac_buf[DAC_N];

uint16_t __attribute__ ((section (".noinit")))  adc_dma[N];
char __attribute__ ((section (".noinit"))) tmpStr[20];// temp string

//----------------------------------------------------------------------------
// res = res / div
//----------------------------------------------------------------------------
void cplxDiv(cplx * res, cplx * div)
{
	cplx tmp;
	float mod2 = div->Re*div->Re + div->Im*div->Im;

	tmp.Re =  ( res->Re * div->Re + res->Im * div->Im ) / mod2;	//(a*c+b*d)/(c*c+d*d);
	tmp.Im =  ( res->Im * div->Re - res->Re * div->Im ) / mod2; //(b*c-a*d)/(c*c+d*d);
	res->Re = tmp.Re;
	res->Im = tmp.Im;

}
//----------------------------------------------------------------------------
// res = res * mul
//----------------------------------------------------------------------------
void cplxMul(cplx * res, cplx * mul)
{
	cplx tmp;
	tmp.Re =  ( res->Re * mul->Re - res->Im * mul->Im );// a*c - b*d
	tmp.Im =  ( res->Re * mul->Im + res->Im * mul->Re );// a*d + b*c
	res->Re = tmp.Re;
	res->Im = tmp.Im;
}
//----------------------------------------------------------------------------
int cordic(int theta)
{
	int k, tx, ty;
	int x=cordic_1K,y=0,z=theta;

	if( ( z >= half_pi ) || ( z < -half_pi) ) z = (half_pi<<1) - z;

	for ( k = 0 ; k < 32; ++k ) // 32bit
	{
		if(z >= 0 )
		{
			tx = x -  (y>>k) ;
			ty = y +  (x>>k) ;
			z = z -  cordic_ctab[k];
			x = tx; y = ty;
		}else{
			tx = x +  (y>>k) ;
			ty = y -  (x>>k) ;
			z = z +  cordic_ctab[k];
			x = tx; y = ty;
		}
	}
	return (y);
	//*c = x; *s = y;
}
//----------------------------------------------------------------------------
void resetBalanceData(void)
{

corr.Corr0.Re = 1.0;corr.Corr0.Im = 0;
corr.Corr2.Re = 1.0;corr.Corr2.Im = 0;

corr.shift0.Re = 0;
corr.shift0.Im = 0;
corr.shift1.Re = 0;
corr.shift1.Im = 0;
corr.shift2.Re = 0;
corr.shift2.Im = 0;

}
//----------------------------------------------------------------------------
void fillSine(int freq)
{
	int s,i;
	long long pp;
	for(i=0;i<N;i++)
	{
		pp = ( ((long long )freq * i << 32 ) / N );
		s = cordic((int)pp);

		if(s > 0 )
			s = ((s>>14)+1)>>1;
		else
			s = ((s>>14)-1)>>1;

		sine[i] = s;
	}

	for(i=0;i<N/4;i++)	sine[i+N] = sine[i];


	// load Corr[findex] from eeprom

resetBalanceData();

cstatus = 0;

if( eepromLoadParam( EEP_BALANCE_BASE  + findex , &corr ) > 0 ) cstatus |= (1<<(findex+8));
if( eepromLoadParam( EEP_CAL_BASE  + findex , &cal ) > 0 ) cstatus |= (1<<(findex));

}
//----------------------------------------------------------------------------
float absolute(float x)
{
	if (x < 0) return (-x);
	return (x);
}
//----------------------------------------------------------------------------
float square(float x) {
	float guess = 1;
	int lim = 40;

	while( (absolute(guess*guess - x) >= 1e-40 )&&(lim-- >0))
		guess = ((x/guess) + guess) * 0.5;

	return (guess);
}
//----------------------------------------------------------------------------
void puthex(int inp,short size)
{
	char buf[9];
	int j;

	for(j=size-1;j>=0;j--)
	{char dig = (inp&0xf);
	if(dig<10){buf[j]=0x30+dig;}else{buf[j]=0x41+dig-10;}
	inp >>=4;
	}

	buf[size]=0;
	uart_tx(buf,1);
}
//----------------------------------------------------------------------------
void sputdec(char * buf,int inp)
{
	int ptr = 0;
	int s,startflag = 1;
	int digit;


	for(ptr =0;ptr < 12; ptr++)	buf[ptr]=0;
	ptr = 0;

	if(inp<0 ){inp = -inp;buf[ptr++]='-';}

	for(s = 0;s<10;s++)
	{
		digit =0;
		while (inp >= dig[s]){inp-=dig[s];digit++;}

		if(digit != 0) startflag =0;
		if(s == 9 )startflag =0;
		if( startflag == 0 ) buf[ptr++]=0x30+digit;

	}

	return;
}
//----------------------------------------------------------------------------
void putdec(int inp)
{
	char buf[12];
	sputdec(buf,inp);
	uart_tx(buf,1);
}
//----------------------------------------------------------------------------
void sputFloat (float num,char * out, char * suffix)
{
	char nnn[20];
	int i,dot;

	out[0] = ' ';
	if(num < 0 ) { num = -num;out[0]= '-';}
	if(num > 1e10) num = 1e10;
	if(num < 1e-15) num = 1e-15;

	int exp = 19;

	if(num < 10000.0)
	{
		while(num < 10000.0){ num*= 10.0; exp--;	}
	}else{
		while(num > 99999.4){ num*= 0.1; exp++;	}
	}

	dot = (exp+30)%3;
	exp = (exp)/3;

	if(exp <0 || exp> 8 )
	{
		for(i = 0; i<6;i++)
		{
			out[i+1] = '-';
		}
	}else{

		sputdec(nnn,num+0.5);

		char * optr = &out[1];
		for(i = 0; i<6;i++)
		{
			*optr++ = nnn[i];
			if(i==dot){ *optr++  = '.'; }
		}
	}

	out[6] = 0;
	suffix[0] = dp[exp];
}
//----------------------------------------------------------------------------
void  printFloat(int x , int y,float num,char * suffix)
{
char out[20];
char sfx[20];

sfx[0] = ' ';
strcpy(sfx+1,suffix);
sputFloat(num,out,sfx);

	lcd_gotoxy(x,y+1);
	if(num >= 0) lcd_putstr(" ",0); else  lcd_putstr("-",0);

	lcd_putnum(x+6,y,out);
	lcd_gotoxy(15*4 + x,y+1);
	lcd_putstr(sfx,0);

	if(getBf(BIT_UART_MODE))
	{
	if(sfx[1] == '\\')strcpy(&sfx[1],"Ohm");
	uart_tx(out,1);
	uart_tx(sfx,1);
	uart_tx(" ",1);
	}

}
//----------------------------------------------------------------------------
void measure(cplx * Z , int rounds)
{
	cplx I, ch[3];

	mAcc[0].Re = 0;mAcc[0].Im = 0;
	mAcc[1].Re = 0;mAcc[1].Im = 0;
	mAcc[2].Re = 0;mAcc[2].Im = 0;

	int r = rounds;

	while(r-- >0 )
	{
		runRound();
		mAcc[0].Re += mData[0].Re;
		mAcc[0].Im += mData[0].Im;
		mAcc[1].Re += mData[1].Re;
		mAcc[1].Im += mData[1].Im;
		mAcc[2].Re += mData[2].Re;
		mAcc[2].Im += mData[2].Im;

		runRound();
		mAcc[0].Re -= mData[0].Re;
		mAcc[0].Im -= mData[0].Im;
		mAcc[1].Re -= mData[1].Re;
		mAcc[1].Im -= mData[1].Im;
		mAcc[2].Re -= mData[2].Re;
		mAcc[2].Im -= mData[2].Im;

	}

	ch[0].Re = mAcc[0].Re;
	ch[0].Im = mAcc[0].Im;
	ch[1].Re = mAcc[1].Re;
	ch[1].Im = mAcc[1].Im;
	ch[2].Re = mAcc[2].Re;
	ch[2].Im = mAcc[2].Im;

	cplxMul(&ch[0],&corr.shift0);
	cplxMul(&ch[1],&corr.shift0);
	cplxMul(&ch[2],&corr.shift0);

	mAcc[0].Re -= ch[1].Re + ch[2].Re;
	mAcc[0].Im -= ch[1].Im + ch[2].Im;
	mAcc[1].Re -= ch[0].Re + ch[2].Re;
	mAcc[1].Im -= ch[0].Im + ch[2].Im;
	mAcc[2].Re -= ch[0].Re + ch[1].Re;
	mAcc[2].Im -= ch[0].Im + ch[1].Im;

	mdata.polar[0].Re= 0.5*vcc*1.4901161e-12*square(mAcc[0].Re*mAcc[0].Re + mAcc[0].Im*mAcc[0].Im);// 1/512/65536/20000
	mdata.polar[1].Re= 0.5*vcc*1.4901161e-12*square(mAcc[1].Re*mAcc[1].Re + mAcc[1].Im*mAcc[1].Im);
	mdata.polar[2].Re= 0.5*vcc*1.4901161e-12*square(mAcc[2].Re*mAcc[2].Re + mAcc[2].Im*mAcc[2].Im);

	mdata.polar[0].Im= 180.0*atan2f(mAcc[0].Im,mAcc[0].Re)/M_PI;
	mdata.polar[1].Im= 180.0*atan2f(mAcc[1].Im,mAcc[1].Re)/M_PI;
	mdata.polar[2].Im= 180.0*atan2f(mAcc[2].Im,mAcc[2].Re)/M_PI;

	cplxMul(&mAcc[0], &(corr.Corr0) );
	cplxMul(&mAcc[2], &(corr.Corr2) );

	Z->Re = (mAcc[2].Re - mAcc[0].Re); // V
	Z->Im = (mAcc[2].Im - mAcc[0].Im);
	cplxMul(Z,&R);
	I.Re = (mAcc[0].Re - mAcc[1].Re); // I
	I.Im = (mAcc[0].Im - mAcc[1].Im);
	cplxDiv(Z,&I);

}
//----------------------------------------------------------------------------
float filter(int sidx,float new)
{
	static float state[4];
	static int cnt[4];

	float t = (state[sidx]-new);
		state[sidx] = state[sidx]*(1-ALPHA)+ new*ALPHA;
		if(t < 0 )cnt[sidx]++; else  cnt[sidx]--;

	if( (cnt[sidx] > 2*FLT_LEN) || (cnt[sidx] < -2*FLT_LEN) )
	{
		state[sidx] = new;
		cnt[sidx] = 0;
	}

	return state[sidx];
}

