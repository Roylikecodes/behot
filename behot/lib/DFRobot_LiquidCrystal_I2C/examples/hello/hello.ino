#include <DFRobot_LiquidCrystal_I2C.h>

DFRobot_LiquidCrystal_I2C mylcd(0x20,16,2);

void setup()
{
  mylcd.init();
  mylcd.backlight();
}
void loop()
{
  mylcd.setCursor(0, 1-1);
  mylcd.print("hello");
}


