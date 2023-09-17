#include ".\OLED/iic_oled.h"
#include ".\DATA/data.h"

//----------------------------------------------------------------------------
void powerOff(void)
{
	int cnt =0;

	eepromSaveParam( EEP_SHUNT , &R , sizeof(R));
	eepromSaveParam( EEP_PARAMS1 , &bitParams , sizeof(bitParams));

	AFIO->EXTICR[3] = AFIO_EXTICR4_EXTI13_PC;
	EXTI->FTSR = EXTI_FTSR_TR13;
	EXTI->IMR = 0;
	EXTI->EMR = EXTI_EMR_MR13;
	EXTI->PR = 0x0003FFFF;

	DAC->CR  =0;
	ADC1->CR2 =0;
	GPIOA->CRH &= ~(
			GPIO_CRH_CNF9 |GPIO_CRH_MODE9 // usart tx
			|GPIO_CRH_CNF10|GPIO_CRH_MODE10 // usart rx
	);

	lcd_write(COMMAND, 0xA5); // DAL
	lcd_write(COMMAND, 0xAE); // turn off display


	GPIOA->BSRR = GPIO_Pin_3; // ANALOG OFF
	GPIOA->BRR = GPIO_Pin_15; // BACKLIGHT OFF

	SCB->SCR |= SCB_SCR_SLEEPDEEP;
	PWR->CR &= ~(PWR_CR_PDDS|PWR_CR_LPDS);
	PWR->CR |= PWR_CR_LPDS;

	while( !(GPIOC->IDR & GPIO_Pin_13) ) ;

	apo = APO_MAX;
	while(cnt <1000000)
	{
		if(cnt > 400000) GPIOA->BSRR = GPIO_Pin_15; // BACKLIGHT ON
		if( !(GPIOC->IDR & GPIO_Pin_13) )cnt++;
		else
		{
			if(cnt > 400000){ apo = APO_4MIN;break;}//512 = 4min
			GPIOA->BRR = GPIO_Pin_15;
			if(cnt>0)cnt = 0; else cnt--;
			if(cnt<-100){	EXTI->PR = 0x0003FFFF;__NOP();	__WFE(); }
		}

	}
	NVIC_SystemReset();

	//GPIOA->BSRR = GPIO_Pin_15; // BACKLIGHT ON

}
//----------------------------------------------------------------------------
void waitz(float lim)
{
	float min = -lim , max = lim;
	float z;
	cplx Z;

	sign = -1;

	if(lim < 0 ) // minimum
	{
		z = min * 2;
		while( z > min )
		{
			measure(&Z,1);
			z = absolute(Z.Re);
			if( (buttons()>>PWR_BTN) & BTN_PUSH  ) powerOff();
		}

	}else{
		z = 0;
		while( z < max*max )
		{
			measure(&Z,1);
			z = (Z.Re*Z.Re+Z.Im*Z.Im);
			if( (buttons()>>PWR_BTN) & BTN_PUSH  ) powerOff();
		}
	}

	measure(&Z,CAL_SETTLE);
}
//----------------------------------------------------------------------------
void waitOpen()
{
	lcd_gotoxy(0, 2);lcd_putstr("Open leads!!",1);	waitz(1000);
}

void waitClose()
{
	lcd_gotoxy(0, 2);lcd_putstr("Close leads!!",1);waitz(-10);
}
//----------------------------------------------------------------------------
void print_openshort(int t) // open - 0
{
	lcd_gotoxy(0, 2);

	switch(t)
	{
	case 0:
		lcd_putstr("Open cal    ",1);break;
	case 1:
		lcd_putstr("Short cal   ",1);break;
	case 2:
		lcd_putstr("Balance 1   ",1);break;
	case 3:
		lcd_putstr("Balance 2   ",1);break;
	default:
		lcd_putstr("Error!!!!!  ",1);break;
	}
}
//----------------------------------------------------------------------------
void calibrate( void ){
	waitOpen();

	print_openshort(0);

	measure(&(cal.Zo),CAL_ROUNDS);

	waitClose();

	print_openshort(1);

	measure(&(cal.Zs),CAL_ROUNDS);

	// save to eeprom
	eepromSaveParam( EEP_CAL_BASE  + findex , &cal , sizeof(cal) );

	cstatus |= (1<<(findex));
}
//----------------------------------------------------------------------------
void balance( void )
{
	cplx ch0,ch2;
	uint32_t ogain = gain;

	resetBalanceData();

	waitOpen();

	print_openshort(2); // ------------------

	swap_dacs = 0;
	sign = -1;

	measure(&Z,1);
	measure(&Z,CAL_ROUNDS);

	ch0.Re = mAcc[1].Re;
	ch0.Im = mAcc[1].Im;

	ch2.Re = mAcc[2].Re;
	ch2.Im = mAcc[2].Im;

	print_openshort(3); // ------------------

	sign = 1;

	measure(&Z,1);
	measure(&Z,CAL_ROUNDS);

	if(ch0.Re * mAcc[1].Re > 0)
	{
		ch0.Re -= mAcc[1].Re;
		ch0.Im -= mAcc[1].Im;
		ch2.Re -= mAcc[2].Re;
		ch2.Im -= mAcc[2].Im;

	}else{

		ch0.Re += mAcc[1].Re;
		ch0.Im += mAcc[1].Im;
		ch2.Re += mAcc[2].Re;
		ch2.Im += mAcc[2].Im;
	}

	cplxDiv(&ch0,&ch2);

	corr.shift0.Re = ch0.Re;
	corr.shift0.Im = ch0.Im;

	//
	sign = 1;
	gain = ogain;
	measure(&Z,8);

	print_openshort(0);

	measure(&Z,1);
	measure(&Z,CAL_ROUNDS);

	ch0.Re = mAcc[1].Re;
	ch0.Im = mAcc[1].Im;

	cplxDiv(&ch0,&mAcc[0]);// corr ch0 to match ch1

	waitClose();

	sign = 1;measure(&Z,8);
	print_openshort(1);

	measure(&Z,2);
	measure(&Z,CAL_ROUNDS);

	ch2.Re = mAcc[0].Re;
	ch2.Im = mAcc[0].Im;

	cplxDiv(&ch2,&mAcc[2]);// corr ch2 to match ch0
	cplxMul(&ch2,&ch0);    // full corr ch2

	corr.Corr0.Re = ch0.Re;
	corr.Corr0.Im = ch0.Im;
	corr.Corr2.Re = ch2.Re;
	corr.Corr2.Im = ch2.Im;

	// save Corr to eeprom

	eepromSaveParam( EEP_BALANCE_BASE  + findex , &corr ,sizeof(corr) );

	cstatus |= (1<<(findex+8));
	sign = -1;
	swap_dacs = 0;
}

//----------------------------------------------------------------------------
unsigned int processButton(unsigned int state)
{
int res = 0;

if( ((state&PUSH_MSK) == PUSH_MAP) && ((state&LPUSH_MSK)!= (LPUSH_MSK-1))  ) {res|= BTN_PUSH;} // push debounce
if( (state&LPUSH_MSK) == LPUSH_MAP ) res|= BTN_LPUSH;
if( (state&RPT_MSK) == RPT_MAP ) res|= BTN_RPT;

return res;
}
//---------------------------------------------------------------------------
int buttons()
{
	int out = 0;
	static unsigned int state[BTN_CNT];

	for(int i = 0;i<BTN_CNT;i++)
	{
		state[i]<<=1;
		if( !(*btn[i].port & btn[i].msk) ) { state[i] |= 1;}
		out |= processButton(state[i])<<i;
	}

	return (out);
}
//----------------------------------------------------------------------------
int menuProcess(int btns){

static int selected = 0;
static int editable = 0;

if( (selected < menu[menuCurrent].first )||( selected > menu[menuCurrent].last) ){ selected = menu[menuCurrent].first;editable =0;}

if(editable == 0)
{
if( (btns) & ((BTN_PUSH|BTN_LPUSH|BTN_RPT)<<SP_BTN)) {selected--;} // down
if( (btns) & ((BTN_PUSH|BTN_LPUSH|BTN_RPT)<<REL_BTN)) {selected++;} // up
}else{
 menu[selected].d(selected,tmpStr,btns);
}

if( selected > menu[menuCurrent].last) selected = menu[menuCurrent].last;

int min = selected - 1;
int max = menu[menuCurrent].last;

if( (min + MAX_MENU_ITEMS - 1) > max ) min = max - MAX_MENU_ITEMS + 1;
if( min < menu[menuCurrent].first) min = menu[menuCurrent].first;

if( max > min + MAX_MENU_ITEMS - 1 ) max = min + MAX_MENU_ITEMS - 1;
if( max > menu[menuCurrent].last ) max = menu[menuCurrent].last;

if ( menu[menuCurrent].parent>= 0  ){ lcd_gotoxy(0,0);lcd_putstr(menu[menuCurrent].name,1); }

lcd_gotoxy(6*14,0);
if(min != menu[menuCurrent].first) lcd_putstr("<",0);else lcd_putstr(" ",0);
if(max != menu[menuCurrent].last) lcd_putstr(">",0);
lcd_putstr("",1);

for(int i = 0 ; i <= (max-min); i++)// display items
{
	mask = 0x00;
	//if( (min  + i + 1) == selected) mask = 0x80;

	lcd_gotoxy(0,i+2);
	if(selected == (min+i) )	mask = 0xFF;

	lcd_putstr(menu[min + i].name,1);

	if(menu[min + i].d != NULL)
		{
		menu[min + i].d(min + i,tmpStr,0);
		if( (selected == (min+i)) && editable )mask = 0x00;
		lcd_gotoxy(6*(16 - strlen(tmpStr)),i+2);
		lcd_putstr(tmpStr,1);
		}
}

mask = 0x00;

if((btns) & BTN_PUSH) // menu select
{

if(  editable == 0  ) {
	if(menu[selected].d != NULL)
	{
	if(menu[selected].d(selected,tmpStr,0)){return selected;}else{editable = 1;}
	}
	else
	return selected;
}else 	editable = 0;

}

if( (editable ==0) && ( ( (btns) & BTN_LPUSH) | (menu[menuCurrent].timeout < (hundredMsTick - startTime))) ){	return menu[menuCurrent].parent ;} // menu select

return menuCurrent;
}
//----------------------------------------------------------------------------
void printSecondary(int x, int y,int idx)
{
lcd_gotoxy(x,y);

switch(idx) //{"OFF","Z","Q","D","VDD","VBAT"};
		{
case 1:
	lcd_putstr("Z  ",0);
	printSmallFloat(mdata.Zpolar.Re,"\\");
	break;
case 2:
	lcd_putstr("Q  ",0);
	printSmallFloat(mdata.Q," ");
	break;
case 3:
	lcd_putstr("D  ",0);
	printSmallFloat(mdata.D," ");
	break;
case 4:
	lcd_putstr("VD ",0);
	printSmallFloat(vcc*0.001,"V");
	break;
case 5:
	lcd_putstr("VB ",0);
	printSmallFloat(vbat*0.001,"V");
	break;
case 6:
	lcd_putstr("AN ",0);
	printSmallFloat(mdata.Zpolar.Im,"`");
	break;
default:
		lcd_putstr("          ",0);
		}
return;
}
//----------------------------------------------------------------------------
int leafProcess(int btns) {
static cplx sum;
static int round = 0;
int averages = getBf(BIT_AVERAGES);
float ls, cs,d,d2,q,zlimit;

sum.Re += Z.Re;
sum.Im += Z.Im;
round++;
if(round >= averages ){

	Z.Re = sum.Re/averages;
	Z.Im = sum.Im/averages;

	if(getBf(BIT_REL_MODE))	{
			Z.Re -= base.Re;
			Z.Im -= base.Im;
		}else{
			base.Re= Z.Re ;
			base.Im = Z.Im;
		}

	ls = Z.Im/6.283185307/(freq*1000.0);
	cs = -1/6.283185307/(freq*1000.0)/Z.Im;

	d = Z.Re/Z.Im; if(d < 0 ) d = -d;
	d2 = d*d;
	q = 1/d;

	lcd_gotoxy(13*6,2);//13*6
	lcd_putstr( sLCMode[getBf(BIT_LC_MODE)] ,0);
	if( getBf(BIT_LC_MODE) == 1) // par
	{
		ls = ls*(1+d2);
		cs = cs/(1+d2);
		Z.Re = Z.Re*(1.0+d2)/d2;
	}


	zlimit = rlim[getBf(BIT_LIM_RANGE)];

	mdata.Zpolar.Re= square(Z.Re*Z.Re + Z.Im*Z.Im);
	mdata.Zpolar.Im= 180.0*atan2f(Z.Im,Z.Re)/M_PI;

	mdata.Q = q;
	mdata.D = d;
//	mod = square(Z.Re*Z.Re + Z.Im*Z.Im);


	printSecondary(0,0,getBf(BIT_1STPARAM));
	printSecondary(0,1,getBf(BIT_2NDPARAM));
	printSecondary(0,2,getBf(BIT_3RDPARAM));


	if(mdata.Zpolar.Re < zlimit)
	{
	printFloat(0,3,Z.Re,"\\");

	if(Z.Im > 0)
	{
		printFloat(0,6,ls,"H");
	}else{
		printFloat(0,6,cs,"F");
	}
	}else{
		for(int i =3;i<8;i++)
		{
		lcd_gotoxy(0,i);
		lcd_putstr("      \\",1);
		}
		lcd_gotoxy(0,8);	lcd_putstr(" ",1);
		lcd_gotoxy(0,4);
		lcd_putstr("OUT OF RANGE",1);
	}

	if(getBf(BIT_UART_MODE))	{	uart_tx("\r\n",1);	}

	sum.Re = 0;
	sum.Im = 0;
	round = 0;

}

printBat(6*13-2,0,(((int)vbat - BAT_MIN)*100)/(BAT_MAX-BAT_MIN));

lcd_gotoxy(13*6,3);
lcd_putstr(sFreqs[findex] ,0);

lcd_gotoxy(13*6,4);
if( (cstatus & (1<<(findex+8))))	lcd_putstr("BAL",0); else	lcd_putstr("---",0);

lcd_gotoxy(13*6,5);
if( (cstatus & (1<<findex))) lcd_putstr("CAL",0); else	lcd_putstr("---",0);

lcd_gotoxy(13*6,6);
if(  ( apo - hundredMsTick + startTime )  < APO_4MIN ) lcd_putstr("APO",0);
	else lcd_putstr("   ",0);

lcd_gotoxy(13*6,7);
if( getBf(BIT_REL_MODE) ) lcd_putstr(">-<",0); else	lcd_putstr("   ",0);

if(vbat < iLowBat[getBf(BIT_LOWBAT)] ) return  MENU_LOWBAT;

if(btns & (BTN_PUSH<<SP_BTN)){
	findex++;
	if(findex>= FCNT ) findex = 0;
	freq = flist[findex];
	gain = fgain[findex];
	fillSine(freq);
	setBf(BIT_REL_MODE,0); //reset rel mode on freq switch
}
if(btns & (BTN_LPUSH<<SP_BTN)){ return MENU_CAL;}

if(btns & (BTN_PUSH<<REL_BTN)){ setBf(BIT_REL_MODE,getBf(BIT_REL_MODE)-1); }
if(btns & (BTN_LPUSH<<REL_BTN)){ setBacklight((getBf(BIT_BL_MODE)+1)&1); }

if(btns & (BTN_PUSH<<PWR_BTN)){ return menu[menuCurrent].parent ;} // menu select
if(btns & (BTN_LPUSH<<PWR_BTN)) powerOff();


return menuCurrent;
}
//----------------------------------------------------------------------------
int leafBatLow(int btns)
{
lcd_gotoxy(2*6,4);lcd_putstr("Battery low!",0);

if( (menu[menuCurrent].timeout < (hundredMsTick - startTime)) ){ powerOff() ;} // menu select

if(btns & (BTN_PUSH<<REL_BTN)){setBf(BIT_LOWBAT,0); return (MENU_DISP) ;}

return menuCurrent;
}
//----------------------------------------------------------------------------
int leafInfo(int btns) {

lcd_gotoxy(0,0);lcd_putstr( VER ,1);
lcd_gotoxy(0,2);lcd_putstr("   Neekeetos    ",0);
lcd_gotoxy(0,3);lcd_putstr("   @yahoo.com   ",0);
lcd_gotoxy(0,4);lcd_putstr(" big thanks to  ",0);
lcd_gotoxy(0,5);lcd_putstr(" TESLight, Link ",0);
lcd_gotoxy(0,6);lcd_putstr("Ozzy & radiocats",0);
lcd_gotoxy(0,7);lcd_putstr(" @ radiokot.ru  ",0);

if( (menu[menuCurrent].timeout < (hundredMsTick - startTime)) ){ return menu[menuCurrent].parent ;} // menu select

if(btns & (BTN_PUSH<<PWR_BTN) ){ return menu[menuCurrent].parent ;}
if(btns & (BTN_PUSH<<REL_BTN)){ return (MENU_SCR) ;}
if(btns & (BTN_PUSH<<SP_BTN)){ return (MENU_DIAG) ;}

return menuCurrent;
}
//----------------------------------------------------------------------------
int leafScr(int btns) {

lcd_gotoxy(0,0);lcd_putstr("TOP LEFT",1);
lcd_gotoxy(0,4);lcd_putstr("screen mode set ",0);
lcd_gotoxy(0,7);lcd_putstr("    BOTTOM RIGHT",0);

editScr(0,tmpStr,btns);

if(btns & (BTN_PUSH<<PWR_BTN)){
	eepromSaveParam( EEP_PARAMS1 , &bitParams , sizeof(bitParams));
	return menu[menuCurrent].parent ;
	} // menu select

return menuCurrent;
}
//----------------------------------------------------------------------------
float getMean(float * data, int n)
{
float sum = 0.0 ;

	for(int i =0;i<n;i++)
	{
	sum += data[i];
	}
return (sum/(float)n);
}
//----------------------------------------------------------------------------
float getSigma(float * data, int n)
{
float mean = getMean(data,n);
float sum = 0.0;

		for(int i =0;i<n;i++)
		{
		sum+= (data[i]-mean)*(data[i]-mean);
		}

return square(sum/(float)n);
}
//----------------------------------------------------------------------------

#define MS 16

int diagScr(int btns) {

static int round = 0;
int averages = getBf(BIT_AVERAGES);
static int ptr = 0;
static int sigma = 0,rel = 1;
static float V1[MS],V2[MS],V7[MS],A1[MS],A2[MS],A7[MS];
float angle1,angle2,angle7;

lcd_gotoxy(0,0);lcd_putstr("DIAG ",0);
lcd_putstr(sFreqs[findex] ,0);
if(sigma)lcd_putstr("   SIGMA",1);else lcd_putstr("    MEAN",1);
lcd_gotoxy(6*13,1);
if(rel)lcd_putstr("REL",1);else lcd_putstr("    ",1);

angle1 = mdata.polar[0].Im;
angle2 = mdata.polar[1].Im;
angle7 = mdata.polar[2].Im - 180;

if(angle1 < 0)  { angle1 += 180;angle2 += 180;	 angle7 += 180;}
if(angle7 < 0 ) { angle7+=360; }

if(rel)
{
	angle2 = angle2 - angle1;
	angle7 = angle7 - angle1;

	V1[ptr] = 1e-3*mdata.polar[0].Re;
	V2[ptr] = 1e-3*mdata.polar[1].Re - V1[ptr];
	V7[ptr] = 1e-3*mdata.polar[2].Re - V1[ptr];
}else{
	V1[ptr] = 1e-3*mdata.polar[0].Re;
	V2[ptr] = 1e-3*mdata.polar[1].Re;
	V7[ptr] = 1e-3*mdata.polar[2].Re;
}

A1[ptr] = angle1;
A2[ptr] = angle2;
A7[ptr] = angle7;

ptr++;if(ptr >= MS) ptr = 0;

round++;
if(round >= averages ){
	if(sigma)
	{
		lcd_gotoxy(0,2);lcd_putstr("Ch1(PA1)",0);printSmallFloat(getSigma(V1,MS),"V");
		lcd_gotoxy(0,3);lcd_putstr("        ",0);printSmallFloat(getSigma(A1,MS),"`");
		lcd_gotoxy(0,4);lcd_putstr("Ch2(PA2)",0);printSmallFloat(getSigma(V2,MS),"V");
		lcd_gotoxy(0,5);lcd_putstr("        ",0);printSmallFloat(getSigma(A2,MS),"`");
		lcd_gotoxy(0,6);lcd_putstr("Ch7(PA7)",0);printSmallFloat(getSigma(V7,MS),"V");
		lcd_gotoxy(0,7);lcd_putstr("        ",0);printSmallFloat(getSigma(A7,MS),"`");

	}else{
		lcd_gotoxy(0,2);lcd_putstr("Ch1(PA1)",0);printSmallFloat(getMean(V1,MS),"V");
		lcd_gotoxy(0,3);lcd_putstr("        ",0);printSmallFloat(getMean(A1,MS),"`");
		lcd_gotoxy(0,4);lcd_putstr("Ch2(PA2)",0);printSmallFloat(getMean(V2,MS),"V");
		lcd_gotoxy(0,5);lcd_putstr("        ",0);printSmallFloat(getMean(A2,MS),"`");
		lcd_gotoxy(0,6);lcd_putstr("Ch7(PA7)",0);printSmallFloat(getMean(V7,MS),"V");
		lcd_gotoxy(0,7);lcd_putstr("        ",0);printSmallFloat(getMean(A7,MS),"`");
	}

round = 0;
}

if(btns & (BTN_PUSH<<REL_BTN)){ 	if(sigma) sigma = 0; else sigma = 1; }
if(btns & (BTN_LPUSH<<REL_BTN)){ 	if(rel) rel = 0; else rel = 1; }
if(btns & (BTN_PUSH<<SP_BTN)){
	findex++;
	if(findex>= FCNT ) findex = 0;
	freq = flist[findex];
	gain = fgain[findex];
	fillSine(freq);
}

if(btns & (BTN_PUSH<<PWR_BTN)){	gain = fgain[findex];return menu[menuCurrent].parent ;	} // menu select

return menuCurrent;
}

//----------------------------------------------------------------------------
int leafBalance(int btns) {

lcd_gotoxy(0,0);lcd_putstr("Balance",1);

balance();

return menu[menuCurrent].parent ;
}
//----------------------------------------------------------------------------
int leafCal(int btns) {

lcd_gotoxy(0,0);lcd_putstr("Calibration",1);

calibrate();

return menu[menuCurrent].parent ;
}
//----------------------------------------------------------------------------
int leafCalReset(int btns) {

cstatus &= ~((1<<(findex))|(1<<(findex+8)));

eepromVoidParam(EEP_BALANCE_BASE  + findex);
eepromVoidParam(EEP_CAL_BASE + findex);

resetBalanceData();

return menu[menuCurrent].parent ;
}
//----------------------------------------------------------------------------
int editScr(int sel, char* str, int btns) {

uint32_t m = getBf(BIT_SCR_MODE);

strcpy(str,configBits[BIT_SCR_MODE].list[m]);

if(btns & (BTN_PUSH<<SP_BTN)) {m += 1;
	if(m > configBits[BIT_SCR_MODE].max) m = 0;
	lcd_init(m);
	}
if(btns & (BTN_PUSH<<REL_BTN)){m -= 1;lcd_init(m);}
setBf(BIT_SCR_MODE,m);

return 0;
}

//----------------------------------------------------------------------------
int editBalance(int sel, char* str, int btns) {

if(cstatus & (1<<(findex+8)) ) strcpy(str,"OK");else strcpy(str,"--");

return (-1); // jump to child
}
//----------------------------------------------------------------------------
int editCal(int sel, char* str, int btns) {

if(cstatus & (1<<(findex)) ) strcpy(str,"OK");else strcpy(str,"--");

return (-1); // jump to child
}
//----------------------------------------------------------------------------
int editContrast(int sel, char* str, int btns) {
uint32_t bf = getBf(BIT_SCR_CONTRAST);

sputdec(str,bf);

if(btns & (BTN_LPUSH<<PWR_BTN)) bf = DEF_CONTRAST;

if(btns & (BTN_PUSH<<SP_BTN)) {bf += 1;}
if(btns & (BTN_PUSH<<REL_BTN)){bf -= 1;}

lcd_setcontrast(setBf(BIT_SCR_CONTRAST,bf));

return 0;
}
//----------------------------------------------------------------------------
int editR(int sel, char* str, int btns) {

sputdec(str,R.Re*10);
int k = strlen(str);
str[k]=str[k-1];
str[k-1]= '.';
str[k+1] = '\\'; // ohm
str[k+2] = 0;

if(btns & (BTN_LPUSH<<PWR_BTN)) R.Re = SHUNT;

if(btns & (BTN_PUSH<<SP_BTN)) {R.Re+= 0.1;}
if(btns & (BTN_PUSH<<REL_BTN)){R.Re-= 0.1;}
if(btns & ((BTN_LPUSH|BTN_RPT)<<SP_BTN)) {R.Re+= 1;}
if(btns & ((BTN_LPUSH|BTN_RPT)<<REL_BTN)){R.Re-= 1;}

if(R.Re > 2000 || R.Re < 10 ) R.Re = SHUNT;

return 0;
}
//----------------------------------------------------------------------------
int editAverages(int sel, char* str, int btns)
{
	uint32_t bf = getBf(BIT_AVERAGES);

	sputdec(str,bf);

	if(btns & (BTN_LPUSH<<PWR_BTN)) bf = DEF_AVERAGES;

	if(btns & (BTN_PUSH<<SP_BTN)) {bf += 1;}
	if(btns & (BTN_PUSH<<REL_BTN)){bf -= 1;}

	if(bf<1) bf = 1;
	if(bf>20) bf = 20;

	setBf(BIT_AVERAGES,bf);

	return 0;
}
//----------------------------------------------------------------------------
int editBF(int sel, char* str, int btns) {

int bfi = menu[sel].first;

str[0] = 0;

if(bfi >= CONFIG_BITS_COUNT) return 0;

uint32_t bf = getBf(bfi);

strcpy(str,configBits[bfi].list[bf]);

if(btns & (BTN_PUSH<<SP_BTN)) {bf += 1;if(bf > configBits[bfi].max) bf = 0;}
if(btns & (BTN_PUSH<<REL_BTN)){bf -= 1;}

setBf(bfi,bf);

return 0;
}
//----------------------------------------------------------------------------
void printBat(int x,int y,int percent)
{

lcd_gotoxy(x,y);

if(percent < 0) percent = 0;
int active = 1 + (BAT_N_SEG* percent) / 100 ;

for(int i =0;i<BAT_N_SEG;i++)
{
	if(i < active) {
	lcd_write(DATA, 0x7F);
	lcd_write(DATA, 0x7F);
	lcd_write(DATA, 0x00);
	}else{
	lcd_write(DATA, 0x41);
	lcd_write(DATA, 0x41);
	lcd_write(DATA, 0x41);
	}
}
lcd_write(DATA, 0x7F); // tail
lcd_write(DATA, 0x1C);

}
//----------------------------------------------------------------------------
void setBacklight(int mode)
{
	if(mode){
		GPIOA->BSRR = GPIO_Pin_15; // BACKLIGHT ON
		setBf(BIT_BL_MODE,1);
	}else{
		GPIOA->BSRR = GPIO_Pin_15<<16; // BACKLIGHT ON
		setBf(BIT_BL_MODE,0);
	}
}