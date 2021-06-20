// #pragma mark - Depend ESP8266Audio and ESP8266_Spiram libraries

#include <M5Core2.h>
#include <Wire.h>
#include <driver/i2s.h>

#include <Update.h>

#include "t2k.h"

// static int gBlue=0;

const uint8_t buttonA_GPIO = 39;
const uint8_t buttonB_GPIO = 38;
const uint8_t buttonC_GPIO = 37;
uint32_t buttonA_count = 0;

static M5Touch gTouch;
static M5Buttons gButtons;
//static Button gBtnC=Button(230,240,80,40, true, "BtnC");

class T2K_TouchButton {
	const int mLeft,mRight,mTop,mBottom;
	bool mIsPressed=false;
	bool mNowPressed=false;
public:
	T2K_TouchButton(int inLeft,int inTop,int inWidth,int inHeight)
	:mLeft(inLeft),mRight(inLeft+inWidth),mTop(inTop),mBottom(inTop+inHeight) {
		// empty
	}
	void Update() {
		bool isPressed=gTouch.ispressed();
		bool nowPressed = mIsPressed==false && isPressed;

		TouchPoint_t p=gTouch.getPressPoint();
		int x=p.y;
		int y=240-p.x;
		if(nowPressed==false) {
			mNowPressed=false;
		} else {
			mNowPressed = mLeft<x && x<mRight && mTop<y && y<mBottom;
		}
		mIsPressed = isPressed && mLeft<x && x<mRight && mTop<y && y<mBottom;
	}
	bool IsNowPressed() { return mNowPressed; }
	bool IsPressed() { return mIsPressed; }
};

static T2K_TouchButton gBtnA( 10,240,100,40);
static T2K_TouchButton gBtnB(110,240,120,40);
static T2K_TouchButton gBtnC(230,240,120,40);

// @V40 -> @V4
const char *gXeviMML="@M150 @V4 L12 O6 @K-1 f4&frcfa<c>arf grggrd&drgfre f4&frcfa<c>arf g-rg-g-rf&f2"
					 "@V2 L16 $c<c>b<cec>b<c> c<c>b-<cec>b-<c> c<c>a<cec>a<c> c<c>a-<cec>a-<c>";
const char *gZapper="@M500 @V125 L32 O2F+ O6F+ O2G O6C+ O2G+ O5G+ O2A O5D+"; 
const char *gGalaxian="T450L32O5C>BAGFEDC>BAG";

static const char *gBallSpritePattern[]={
	"__RRRR__",
	"_RRRRRR_",
	"RRWRRRRR",
	"RRRRRRRR",
	"RRRRRRRr",
	"RRRRRRrr",
	"_RRRRrr_",
	"__rrrr__",
};
static T2K_Sprite16colors gBallSprite;
static uint8_t gBallSpritePalette[7][16];

static void topMenu();
static void blinkTopMenu();
static void drawTopMenu(int inSelectedItem,int inCounterForBlink);

static void mmlTest();
static void displaySceneIdError();

static void spriteTest();
static void initBall(int inTargetIndex);

static void fadeOut();

static void displayUsage();

enum SceneID {
	kTOP_SCENE_ID			=  0,

	kMML_TEST_SCENE_ID		= 10,
	kSPRITE_TEST_SCENE_ID	= 20,

	kBLINK_TOP_MENU_SCENE_ID=200,
	kFADE_OUT_SCENE_ID		=201,

	kINVALID_SCENE_ID		=255,
};
static T2K_Scene gSceneTable[]={
	{ kTOP_SCENE_ID,			topMenu 	 },
	{ kMML_TEST_SCENE_ID,		mmlTest 	 },
	{ kSPRITE_TEST_SCENE_ID,	spriteTest	},

	{ kBLINK_TOP_MENU_SCENE_ID,	blinkTopMenu },
	{ kFADE_OUT_SCENE_ID,		fadeOut 	 },
	
	{ kINVALID_SCENE_ID,		NULL		 },	// sentinel
};

void setup() {
	// pixels.begin();
	pinMode(0,INPUT_PULLDOWN);

	t2kInit();
//M5.begin(false,false,false,false);

	if(digitalRead(buttonA_GPIO)==0 && Update.canRollBack()) {
		Update.rollBack();
		ESP.restart();
	}

	Serial.begin(115200);
	delay(50);
	Serial.println("");
	Serial.printf("Arduino main program is running core %d\n",xPortGetCoreID());

	if( t2kGCoreInit() ) { Serial.println("t2kGCoreInit Done.\n"); }
	t2kGCoreStart();
	t2kSetBrightness(128);	// 0 to 255
	t2kFill(~0);

	t2kSCoreInit();
	t2kSCoreStart();

	Serial.println("=== SOUND INIT DONE");
	Serial.printf("internal Free Heap %d\n",ESP.getFreeHeap());

	t2kICoreInit();
	Serial.println("=== t2kICore INIT DONE");

	t2kFontInit();
	Serial.println("=== t2kFont INIT DONE");

	bool result=t2kMmlInit();
	Serial.printf("=== t2kMML INIT DONE %d\n",result);

	result=t2kSceneInit(displaySceneIdError);
	Serial.printf("=== t2kScene INIT DONE %d\n",result);

	t2kCollisionInit();

	Serial.println("===== START =====");
	Serial.printf("Internal Total heap %d, internal Free Heap %d\n", ESP.getHeapSize(), ESP.getFreeHeap());
	Serial.printf("SPIRam Total heap %d, SPIRam Free Heap %d\n", ESP.getPsramSize(), ESP.getFreePsram());
	Serial.printf("Flash Size %d, Flash Speed %d\n", ESP.getFlashChipSize(), ESP.getFlashChipSpeed());
	Serial.printf("ChipRevision %d, Cpu Freq %d, SDK Version %s\n", ESP.getChipRevision(), ESP.getCpuFreqMHz(), ESP.getSdkVersion());

	if(t2kCheckMML(gXeviMML)==false) { Serial.printf("MML ERROR BGM\n"); }
	if(t2kCheckMML(gZapper)==false) { Serial.printf("MML ERROR Zapper\n"); }
	if(t2kCheckMML(gGalaxian)==false) { Serial.printf("MML ERROR Paro\n"); }

	initBallSpritePalette();
	if(t2kInitSprite(&gBallSprite,8,8,gBallSpritePattern,
					 0,0,true,gBallSpritePalette[0])==false) {
		Serial.printf("BALL SPRITE ERROR\n");
	}

	t2kAddScenes(gSceneTable);
	initBall(0);

	t2kRegisterCutomCKB(T2K_CUSTOM_CKB_UP,CKB_VAL_UP,false);
	t2kRegisterCutomCKB(T2K_CUSTOM_CKB_DOWN,CKB_VAL_DOWN,false);
	t2kRegisterCutomCKB(T2K_CUSTOM_CKB_LEFT,CKB_VAL_LEFT,false);
	t2kRegisterCutomCKB(T2K_CUSTOM_CKB_RIGHT,CKB_VAL_RIGHT,false);
	t2kRegisterCutomCKB(T2K_CUSTOM_CKB_A,CKB_VAL_X_LOW,false);
	t2kRegisterCutomCKB(T2K_CUSTOM_CKB_B,CKB_VAL_Z_LOW,false);
	t2kRegisterCutomCKB(T2K_CUSTOM_CKB_START,CKB_VAL_RET,false);
	t2kRegisterCutomCKB(T2K_CUSTOM_CKB_SELECT,CKB_VAL_L_LOW,false);

// gAxp.ScreenBreath(15);

// t2kSetNextSceneID(kMML_TEST_SCENE_ID);
	t2kLed(false); // 緑色LEDを点ける
	t2kSetMasterVolume(kAllChannels,50);

	gTouch.begin();
}

uint8_t gVolume=32;	// 128;
bool gSoundON=true;
bool gPlayingBGM=false;

uint32_t gFrameCount=0;
int gTestCH=0;

int gCount=0;
bool led=true;

// void FillScreen(uint32_t inColor);

void loop() {
	gTouch.update();
	gButtons.update();

	gBtnA.Update();
	gBtnB.Update();
	gBtnC.Update();

	gFrameCount++;
	t2kUpdateMML();
	t2kInputUpdate();
	t2kSceneUpdate();
	t2kFlip();
}

static void mmlTest() {
	uint8_t *gram=t2kGetFramebuffer();
	for(int y=0; y<120; y++) {
		for(int x=0; x<160; x++) {
			gram[FBA(x,y)]=RGB(((y+gFrameCount)%8)/2,
							   (((y+gFrameCount)/10)%8)/2,
							   (((x+gFrameCount)/8)%4)/2);
		}
	}

	if( t2kIsPressedNativeButtonA() ) {
		t2kFill(RGB(7,0,0));
		if( gSoundON ) {
			t2kTone(1,T2K_TONE_G4,50,255);
			t2kAddTone(1,T2K_TONE_A4,50,255);
			t2kAddTone(1,T2K_TONE_B4,50,255);
			t2kAddTone(1,T2K_TONE_C5,50,255);
		}
	}
	if( t2kNowPressedLeft() ) {
		t2kPlayMML(gTestCH,"CDEFGAB<C");
	}
	if( t2kIsPressedRight()) {
		t2kFill(RGB(0,0,3));
		if( gSoundON ) { t2kTone(2,440,100,gVolume); }
	}
	if( t2kIsPressedUp() || gBtnA.IsPressed() ) {
		// t2kFill(RGB(0,7,0));
		if(gVolume<250) {
			gVolume+=5;
		} else {
			gVolume=255;
		}
		// t2kSetBrightness(gVolume);
		for(int ch=0; ch<kNumOfChannels; ch++) {
			t2kSetMasterVolume(ch,gVolume);
		}
	}
	if( t2kIsPressedDown() || gBtnB.IsPressed() ) {
		// t2kFill(kWhite);
		if(gVolume>5) {
			gVolume-=5;
		} else {
			gVolume=0;
		}
		// t2kSetBrightness(gVolume);
		for(int ch=0; ch<kNumOfChannels; ch++) {
			t2kSetMasterVolume(ch,gVolume);
		}
	}
	if( gPlayingBGM==false ||   t2kNowPressedNativeButtonB() || t2kNowPressedStart() ) {
		t2kPlayMML(0,gXeviMML);
		gPlayingBGM=true;
	}
	if( t2kNowPressedSelect() ) {
		gTestCH=(gTestCH+1)%4;
	}

	if( t2kNowPressedA() ) {
		t2kStopMML(0);
	}

	if( t2kNowPressedB() || t2kIsPressedNativeButtonC() ) {
		t2kFill(kRed);
		// t2kPlayMML(3,gZapper);
		t2kPlayMML(3,gGalaxian);
	}

	static int sx=0;
	sx=(sx+1)%160;
	uint8_t dotColor=RGB(0,7,0);
	const int dotSize=5;
	for(int i=0; i<dotSize*dotSize; i++) {
		int dx=i%dotSize,dy=i/dotSize;
		t2kPSet(sx+dx,14+dy,dotColor);
	}

	displayUsage();

	if(t2kIsPressedStart() && t2kIsPressedSelect() || gBtnC.IsNowPressed() ) {
		t2kSetNextSceneID(kFADE_OUT_SCENE_ID);
	}
}
static void displayUsage() {
	uint8_t color[8]={ kBlack,kBlue,kRed,kMagenta,kGreen,kCyan,kYellow,kWhite };

	t2kPutStr(24,4,kWhite,"HI SCORE 10000");
	t2kPrintf(4,20,kWhite,"YOUR SCORE: %07d",gFrameCount);

	if(gPlayingBGM==false) {
		int x=10;
		int y=32;
		uint8_t c=color[gFrameCount%8];
		t2kPrintf(x,y,    c,"Push STRT");
		t2kPrintf(x,y+8,  c,"  or Native-MIDDLE button");
 		t2kPrintf(x,y+8*2,c,"  to play BGM.");
	} else {
		// int x=10;
		int y=32;
		t2kPrintf(4,y+8,kWhite,
							"= NOW PLAYING BGM =");
		t2kPrintf(4,y+16,kRed,
							"SELECT+START = >TOP");
	}
	t2kPrintf(4,56,kGreen," Current Volume:%3d",gVolume);

	int y=65;
	uint8_t buttonColor=kYellow;
	t2kPrintf(4,y,  kWhite,"master volume up");
	t2kPrintf(4,y+8+3,buttonColor,"UP");
	t2kPutChar(4,y+8*3,kWhite,'+');
	t2kPrintf(4,y+8*4+4,buttonColor,"DOWN");
	t2kPrintf(4,y+8*5+4,kWhite,"master volume up");

	t2kPrintf(80,y+8,kWhite,"test ch=%d",gTestCH);
	t2kPrintf(80,y+8*2,buttonColor,"A - ");	
	t2kPrintf(80,y+8*3,buttonColor,"B - Zapper");	
	t2kPrintf(60,y+8*4,kCyan,"or Native-C");
}

static int gSelectedTopMenuItem=0;
static int gBlinkCount=0;
static int gNextSceneIdFromTopMenu=-1;
static bool gInitBrightness=false;
static void topMenu() {
	if(gInitBrightness==false) {
		t2kSetBrightness(255);
		gInitBrightness=true;
	}
	const int numOfMenuItems=3;
	drawTopMenu(gSelectedTopMenuItem,-1);

	if( t2kNowPressedUp() || gBtnA.IsNowPressed() ) {
		gSelectedTopMenuItem=(gSelectedTopMenuItem+numOfMenuItems-1)%numOfMenuItems;
		t2kPlayMML(0,"@V4 L32 N880");
	}
	if(t2kNowPressedSelect() || t2kNowPressedDown() || gBtnB.IsNowPressed() ) {
		gSelectedTopMenuItem=(gSelectedTopMenuItem+1)%numOfMenuItems;
		t2kPlayMML(0,"@V4 L32 N800");
	}

	if(t2kNowPressedStart() || t2kNowPressedB() || gBtnC.IsNowPressed() ) {
		switch(gSelectedTopMenuItem) {
			case 0:	gNextSceneIdFromTopMenu=kMML_TEST_SCENE_ID;		break;
			case 1: gNextSceneIdFromTopMenu=kSPRITE_TEST_SCENE_ID;	break;
			case 2: gNextSceneIdFromTopMenu=kINVALID_SCENE_ID;		break;
			default:
				gInitBrightness=false;
				gNextSceneIdFromTopMenu=kINVALID_SCENE_ID;
		}
		gBlinkCount=0;
		t2kSetNextSceneID(kBLINK_TOP_MENU_SCENE_ID);
	}
}
static void drawTopMenu(int inSelectedItem,int inCounterForBlink) {
	t2kFill(kBlack);
	t2kPrintf(50,kWhite,"DEMO TESTER");

	const char *item[3]={
		"PLAY MML",
		"DRAW SPRITES",
		"ERROR SCREEN",
	};
	for(int i=0; i<3; i++) {
		if(inCounterForBlink>0 && i==inSelectedItem && inCounterForBlink%4>=2) {
			continue;
		} else {
			t2kPrintf(47,70+i*10,kWhite,item[i]);
		}
		if(inCounterForBlink>0 && inCounterForBlink%4==0) {
			t2kPlayMML(0,"L96 O6 CEG");
		}
	}
	t2kDrawFontPattern(35,70+inSelectedItem*10,kWhite,T2K_TrianglePatternRight);
}
static void blinkTopMenu() {
	if(++gBlinkCount>21) {
		t2kSetNextSceneID(gNextSceneIdFromTopMenu);
		if(gNextSceneIdFromTopMenu==kSPRITE_TEST_SCENE_ID) {
			t2kPlayMML(0,gXeviMML);
		}
		return;
	}
	drawTopMenu(gSelectedTopMenuItem,gBlinkCount);
}

enum {
	kDarkBlue=RGB(0,0,1),
	kDarkRed =RGB(3,0,0),
	kDarkMagenta=RGB(3,0,1),
	kDarkGreen  =RGB(0,3,0),
	kDarkCyan	=RGB(0,3,1),
	kDarkYellow =RGB(3,3,0),
	kGray    =RGB(3,3,2),
};
static uint8_t kDarkBallColor[7]={
	kDarkBlue,kDarkRed,kDarkMagenta,kDarkGreen,kDarkCyan,kGray,
};
static uint8_t kBrightBallColor[7]={
	kBlue,kRed,kMagenta,kGreen,kCyan,kYellow,kWhite,
};
static void initBallSpritePalette() {
	for(int i=0; i<7; i++) {
		t2kSetDefault16colorPalette(gBallSpritePalette[i]);
		gBallSpritePalette[i][ 2]=kDarkBallColor[i];
		gBallSpritePalette[i][10]=kBrightBallColor[i];
	}
	gBallSpritePalette[6][15]=kYellow;
}

// ------------------------------------------------------------------
//	sprite test
// ------------------------------------------------------------------
struct Ball {
	float x,y;
	float dx,dy;
	int color;
};
static int gNumOfSprite=1;
const int MAX_BALLS=256;
static Ball gBall[MAX_BALLS];
static void initBall(int inTargetIndex) {
	gBall[inTargetIndex].x=random(0,159);
	gBall[inTargetIndex].y=random(0,119);

	float dx,dy;
	do {
		dx=random(-20,+20)/10.0f*(4+inTargetIndex/20.0f);
		dy=random(-20,+20)/10.0f*(4+inTargetIndex/20.0f);;
	} while(abs(dx)<1|| abs(dy)<1);
	gBall[inTargetIndex].dx=dx;
	gBall[inTargetIndex].dy=dy;
}
static void spriteTest() {
	t2kSetMasterVolume(kAllChannels,50);
	t2kFill(kBlack);
	if(t2kIsPressedUp() && gNumOfSprite<MAX_BALLS) {
		initBall(gNumOfSprite++);				
		t2kPlayMML(1,gGalaxian);
	}
	if(t2kIsPressedDown() && gNumOfSprite>1) {
		gNumOfSprite--;
		t2kPlayMML(1,gGalaxian);
	}
	for(int i=0; i<gNumOfSprite; i++) {
		gBall[i].x+=gBall[i].dx;
		gBall[i].y+=gBall[i].dy;
		if(gBall[i].x<0) { gBall[i].x=0; gBall[i].dx*=-1; }
		if(gBall[i].x+8>=kGRamWidth) { gBall[i].x=kGRamWidth-8-1; gBall[i].dx*=-1; }
		if(gBall[i].y<0) { gBall[i].y=0; gBall[i].dy*=-1; }
		if(gBall[i].y+8>=kGRamHeight) { gBall[i].y=kGRamHeight-8-1; gBall[i].dy*=-1; }
		t2kPutSprite(&gBallSprite,gBall[i].x,gBall[i].y,gBallSpritePalette[i%7]);
	}
	uint8_t color = gNumOfSprite<MAX_BALLS ? kWhite : kCyan;
	t2kPrintf(60,color,"Num of Sprites %03d",gNumOfSprite);

	if(t2kIsPressedStart() && t2kIsPressedSelect()) {
		t2kSetNextSceneID(kFADE_OUT_SCENE_ID);
	}
}


// ------------------------------------------------------------------
//	fade out
// ------------------------------------------------------------------
static uint8_t gFadeOutCount=250;
static void fadeOut() {
	t2kCopyFromFrontBuffer();
	t2kSetBrightness(gFadeOutCount);
	gFadeOutCount-=10;
	if(gFadeOutCount==0) {
		t2kSetNextSceneID(kTOP_SCENE_ID);
		gFadeOutCount=250;
		gInitBrightness=false;
		t2kStopMMLs();
		t2kQuiet();
	}
}

static void displaySceneIdError() {
	t2kFill(kRed);
	t2kPrintf(10,40,kWhite,"SCENE ID ERROR");
	t2kPrintf(10,50,kWhite,"scene id=%d",t2kGetCurrentSceneID());

	t2kPrintf(10,70,kWhite,"PUSH ANY BUTTON");
	t2kPrintf(10,80,kWhite,"TO RETURN TOP MENU");

	int n=gFrameCount%11;
	for(int i=0; i<n; i++) {
		t2kPutChar(10+i*8,100,kWhite,'>');
	}
	t2kPrintf(100,100,kWhite,"%06d",gFrameCount);

	if( t2kNowPressedAnyButton() ) {
		t2kSetNextSceneID(kTOP_SCENE_ID);
	}
}
