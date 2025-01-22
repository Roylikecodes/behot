#include <Arduino.h>
#include "DFRobot_LiquidCrystal_I2C.h"
#define BUTTON_PIN1 2
#define BUTTON_PIN2 3
#define HEAT_PIN 4
#define HEAT_50 50.0
#define HEAT_70 70.0
#define HEAT_80 80.0
#define TIME_30S 30
#define TIME_1M 60
#define TIME_5M 300

// 创建对象
DFRobot_LiquidCrystal_I2C lcd1602;
int minute = 0;
int second = 0;
bool isStart = false;
float heat = 0.0;
float rheat = 0.0;

// 在全局变量区域添加
unsigned long lastDisplayUpdate = 0;
const unsigned long DISPLAY_INTERVAL = 1000; // 显示更新间隔(1秒)
unsigned long lastButtonPress = 0;  // 移到全局变量
const unsigned long DEBOUNCE_DELAY = 300; // 消抖延时
unsigned long lastMenuUpdate = 0;
const unsigned long MENU_UPDATE_INTERVAL = 200; // 菜单显示更新间隔

enum MainMenu {HEAT, TIME, OK};
enum SubMenu {OPT1, OPT2, OPT3};

MainMenu currentMain = HEAT;
SubMenu currentSub = OPT1;
bool inSubmenu = false;
int selectedTime = TIME_30S;
float selectedHeat = HEAT_50;

String formatTime(int minute, int second) {
  String timeStr = "time:";
  timeStr += (minute < 10) ? "0" + String(minute) : String(minute);
  timeStr += ":";
  timeStr += (second < 10) ? "0" + String(second) : String(second);
  return timeStr;
}

void updata(int min,int sec,float h){
  unsigned long currentTime = millis();
  if(currentTime - lastDisplayUpdate >= DISPLAY_INTERVAL) {
    lcd1602.clear();
    lcd1602.printLine(uint32_t(1), "heat:"+String(h));
    lcd1602.printLine(uint32_t(2), formatTime(min, sec));
    lastDisplayUpdate = currentTime;
  }
}

void changetime(int &loopnum){
    if(loopnum >= 1000){
      if(second == 0){
        if(minute != 0){
          minute--;
          second = 59;
        }
        else{
          isStart = false;
          lcd1602.clear();
          lcd1602.printLine(uint32_t(1),"time out");
          lcd1602.printLine(uint32_t(2),"Press B1 to back");
          digitalWrite(HEAT_PIN, LOW);
          return;
        }
      }
      else{
        second--;
      }
      loopnum = 0;
    }
    updata(minute,second,heat);
}

void starttime(int um,int us,float uheat){
  minute = um;
  second = us;
  heat = uheat;
  isStart = true;
  digitalWrite(HEAT_PIN, HIGH);
  updata(minute,second,heat);
}

void displayMainMenu() {
  lcd1602.clear();
  String line1 = "";
  String line2 = "";
  
  switch(currentMain) {
    case HEAT: 
      line1 = ">Heat:" + String(selectedHeat);
      line2 = " Time:" + String(selectedTime) + "s";
      break;
    case TIME: 
      line1 = " Heat:" + String(selectedHeat);
      line2 = ">Time:" + String(selectedTime) + "s";
      break;
    case OK: 
      line1 = " Heat:" + String(selectedHeat);
      line2 = ">OK";
      break;
  }
  lcd1602.printLine(uint32_t(1), line1);
  lcd1602.printLine(uint32_t(2), line2);
}

void chose(){
  unsigned long currentTime = millis();
  static bool button1Pressed = false;
  static bool button2Pressed = false;
  
  // 按钮1检测
  if(digitalRead(BUTTON_PIN1) == HIGH) {
    if(!button1Pressed && (currentTime - lastButtonPress >= DEBOUNCE_DELAY)) {
      lastButtonPress = currentTime;
      button1Pressed = true;
      
      if(!inSubmenu) {
        // 修改主菜单循环逻辑
        if(currentMain == OK) {
          currentMain = HEAT;
        } else if(currentMain == HEAT) {
          currentMain = TIME;
        } else if(currentMain == TIME) {
          currentMain = OK;
        }
        displayMainMenu();
      } else {
        // 子菜单循环逻辑
        if(currentSub == OPT3) {
          currentSub = OPT1;
        } else {
          currentSub = static_cast<SubMenu>(currentSub + 1);
        }
        
        // 显示子菜单
        lcd1602.clear();
        String line1 = "";
        String line2 = "";
        if(currentMain == HEAT) {
          line1 = "Heat:";
          switch(currentSub) {
            case OPT1: line2 = ">50.0"; break;
            case OPT2: line2 = ">70.0"; break;
            case OPT3: line2 = ">80.0"; break;
          }
        } else if(currentMain == TIME) {
          line1 = "Time:";
          switch(currentSub) {
            case OPT1: line2 = ">30s"; break;
            case OPT2: line2 = ">1m"; break;
            case OPT3: line2 = ">5m"; break;
          }
        }
        lcd1602.printLine(uint32_t(1), line1);
        lcd1602.printLine(uint32_t(2), line2);
      }
    }
  } else {
    button1Pressed = false;
  }
  
  // 按钮2检测
  if(digitalRead(BUTTON_PIN2) == HIGH) {
    if(!button2Pressed && (currentTime - lastButtonPress >= DEBOUNCE_DELAY)) {
      lastButtonPress = currentTime;
      button2Pressed = true;
      
      if(!inSubmenu) {
        if(currentMain == OK) {
          return;
        }
        inSubmenu = true;
        currentSub = OPT1;  // 重置子菜单选项
        // 立即显示子菜单的第一个选项
        lcd1602.clear();
        String line1 = "";
        String line2 = "";
        if(currentMain == HEAT) {
          line1 = "Heat:";
          line2 = ">50.0";
        } else if(currentMain == TIME) {
          line1 = "Time:";
          line2 = ">30s";
        }
        lcd1602.printLine(uint32_t(1), line1);
        lcd1602.printLine(uint32_t(2), line2);
        return;  // 直接返回，避免后续代码覆盖显示
      } else {
        if(currentMain == HEAT) {
          switch(currentSub) {
            case OPT1: selectedHeat = HEAT_50; break;
            case OPT2: selectedHeat = HEAT_70; break;
            case OPT3: selectedHeat = HEAT_80; break;
          }
        } else if(currentMain == TIME) {
          switch(currentSub) {
            case OPT1: selectedTime = TIME_30S; break;
            case OPT2: selectedTime = TIME_1M; break;
            case OPT3: selectedTime = TIME_5M; break;
          }
        }
        inSubmenu = false;
        currentSub = OPT1;
      }
      
      // 立即更新显示
      lcd1602.clear();
      String line1 = "";
      String line2 = "";
      if(!inSubmenu) {
        switch(currentMain) {
          case HEAT: 
            line1 = ">Heat:" + String(selectedHeat);
            line2 = " Time:" + String(selectedTime) + "s";
            break;
          case TIME: 
            line1 = " Heat:" + String(selectedHeat);
            line2 = ">Time:" + String(selectedTime) + "s";
            break;
          case OK: 
            line1 = " Heat:" + String(selectedHeat);
            line2 = ">OK";
            break;
        }
      }
      lcd1602.printLine(uint32_t(1), line1);
      lcd1602.printLine(uint32_t(2), line2);
    }
  } else {
    button2Pressed = false;
  }
}

void setup() {
  Serial.begin(115200);
  lcd1602.begin(0x3e);
  pinMode(2, INPUT);
  pinMode(4, OUTPUT);
  lcd1602.printLine(uint32_t(1), "Hello");
  lcd1602.printLine(uint32_t(2), "Wellcome");
}

void loop() {
  static int lop = 0;
  static bool inMenu = false;
  static bool timeOutScreen = false;  // 添加超时屏幕标志

  if(!isStart) {
    if(timeOutScreen && digitalRead(BUTTON_PIN1) == HIGH) {
      // 从超时屏幕返回主界面
      timeOutScreen = false;
      lcd1602.clear();
      lcd1602.printLine(uint32_t(1), "Hello");
      lcd1602.printLine(uint32_t(2), "Wellcome");
      delay(300);  // 防止按键重复触发
      return;
    }
    
    if(digitalRead(BUTTON_PIN1) == HIGH && !inMenu && !timeOutScreen){
      inMenu = true;  // 进入菜单模式
      currentMain = HEAT;  // 重置为第一个选项
      currentSub = OPT1;   // 重置子菜单选项
      inSubmenu = false;   // 重置子菜单状态
      // 显示初始菜单
      lcd1602.clear();
      lcd1602.printLine(uint32_t(1), ">Heat:" + String(selectedHeat));
      lcd1602.printLine(uint32_t(2), " Time:" + String(selectedTime) + "s");
    }
    
    if(inMenu) {
      chose();  // 持续处理菜单
      if(currentMain == OK && digitalRead(BUTTON_PIN2) == HIGH) {  // 确认选择
        inMenu = false;  // 退出菜单模式
        starttime(selectedTime/60, selectedTime%60, selectedHeat);
      }
    }
  }

  if(isStart){
    changetime(lop);
    delay(1);
    lop++;
  } else if(!inMenu) {
    timeOutScreen = true;  // 设置超时屏幕标志
  }
}
