#define ESP8266
#include "BaseConfig.h"
#include "SoftwareSerial.h"
#include "DFRobotDFPlayerMini.h"

int adc;
String response;
int timeInterval = 25;
unsigned long elapsedTime;

SoftwareSerial mySoftwareSerial(5, 13); // RX, TX
DFRobotDFPlayerMini myDFPlayer;
#define INIT_SKIP -1
#define INIT_START 0
#define INIT_DELAY 1
#define INIT_OK1 2
#define INIT_OK2 3
#define INIT_RUN 4

int dfPlayerInit = INIT_START;
int dfPlayerType;
int dfPlayerParameter;

int muteState;
//delay initalising dfPlayer module after power on (mSec)
#define DFPLAYER_STARTUP 4000
#define VOLUP 0
#define VOLDOWN 1
#define SELECT1 2
#define SELECT2 3
#define MUTE 16
#define LONG_PRESS 1000

int folderSelect = 1;
int fileSelect = 1;
int volume = 15;
String cmd;
unsigned long resetPeriod;

float bat_volts;
int   bat_percent;

void init_dfPlayer() {
	Serial.println();
	Serial.println(F("Initializing DFPlayer ... (May take 2 seconds)"));
  
	//Use softwareSerial to communicate with mp3.
	//Extend timeout for getting reset message back (typically takes 1 second)	
	myDFPlayer.setTimeOut(DFMSG_TIMEOUT);
	resetPeriod = millis();
	if (myDFPlayer.begin(mySoftwareSerial)) {
		dfPlayerInit = INIT_OK1;
		Serial.print(F("DFPlayer Mini online."));
   
	} else {
		dfPlayerInit = INIT_OK2;
		Serial.print(F("DFPlayer Mini offline."));
	}
	resetPeriod = millis() - resetPeriod;
	Serial.println(" Reset period:" + String(resetPeriod));
}

void dfPlayerSetVolume(int vol) {
	if(vol > 30) vol = 30;
	if(vol < 0) vol = 0;
	if(vol != volume) {
		volume = vol;
		myDFPlayer.volume(volume);
	}
}

void dfPlayerCmd() {
	cmd = server.arg("cmd");
	int p1 = server.arg("p1").toInt();
	int p2 = server.arg("p2").toInt();
	int p3 = server.arg("p3").toInt();
	server.send(200, "text/html", F("dfPlayer cmd being processed"));
	Serial.print(cmd);Serial.printf(" p1=%d p2=%d\r\n",p1,p2);
	
	if(dfPlayerInit != INIT_RUN  && dfPlayerInit >= INIT_OK1) {
		myDFPlayer.volume(volume);
		dfPlayerInit = INIT_RUN;
		delaymSec(500);
	}

	if(cmd.equalsIgnoreCase("play")) {
			myDFPlayer.playFolder(folderSelect, p2);
	} else if(cmd.equalsIgnoreCase("playmp3")) {
		myDFPlayer.playMp3Folder(p1);
	} else if(cmd.equalsIgnoreCase("volume")) {
		dfPlayerSetVolume(p1);
	} else if(cmd.equalsIgnoreCase("stop")) {
		myDFPlayer.stop();
	} else if(cmd.equalsIgnoreCase("volumeup")) {
		dfPlayerSetVolume(volume+1);
	} else if(cmd.equalsIgnoreCase("volumedown")) {
		dfPlayerSetVolume(volume-1);
	} else if(cmd.equalsIgnoreCase("speaker")) {
		p1 = p1 ? 0 : 1;
		digitalWrite(MUTE, p1);
		muteState = p1;
	} else if(cmd.equalsIgnoreCase("pause")) {
		myDFPlayer.pause();
	} else if(cmd.equalsIgnoreCase("start")) {
		myDFPlayer.start();
   Serial.println("here");
	} else if(cmd.equalsIgnoreCase("next")) {
		myDFPlayer.next();
	} else if(cmd.equalsIgnoreCase("previous")) {
		myDFPlayer.previous();
	} else if(cmd.equalsIgnoreCase("mode")) {
		myDFPlayer.loop(p1);
	} else if(cmd.equalsIgnoreCase("loopFolder")) {
			myDFPlayer.loopFolder(folderSelect);
	} else if(cmd.equalsIgnoreCase("random")) {
		myDFPlayer.randomAll();
	} else if(cmd.equalsIgnoreCase("eq")) {
		myDFPlayer.EQ(p1);
	} else if(cmd.equalsIgnoreCase("device")) {
		myDFPlayer.outputDevice(p1);
	} else if(cmd.equalsIgnoreCase("setting")) {
		myDFPlayer.outputSetting(p1,p2);
	} else if(cmd.equalsIgnoreCase("sleep")) {
		myDFPlayer.sleep();
	} else if(cmd.equalsIgnoreCase("reset")) {
		myDFPlayer.reset();
	} else if(cmd.equalsIgnoreCase("init")) {
		init_dfPlayer();
	}
}

void getBattery() {
  adc = analogRead(A0);
	bat_volts   = ((adc*3.3)/1024)*2*1.125;
  bat_percent = (bat_volts/6)*100;
	response = "{\"voltage\":" + ((String) bat_volts) + ", \"battery\":"+ bat_percent +"}";
	server.send(200, "application/json", response);
}

bool dfPlayerFinished() {
	myDFPlayer.available();
	return (myDFPlayer._handleCommand == 61);
}

void setupStart() {
  Serial.begin(115200);
  pinMode(13,OUTPUT);
  pinMode(5,INPUT);
}

void extraHandlers() {
	server.on("/dfPlayer", dfPlayerCmd);
	server.on("/battery", getBattery);
}

void setupEnd() {
	delaymSec(500);
	mySoftwareSerial.begin(9600);
}

void loop() {
	server.handleClient();
	wifiConnect(1);
	delaymSec(timeInterval);
	elapsedTime++;
	if(dfPlayerInit == INIT_START && elapsedTime * timeInterval > DFPLAYER_STARTUP) {
		Serial.println(F("Delayed init of dfPlayer"));
		dfPlayerInit = INIT_DELAY;
		init_dfPlayer();
		digitalWrite(MUTE, 0);
	}
}
