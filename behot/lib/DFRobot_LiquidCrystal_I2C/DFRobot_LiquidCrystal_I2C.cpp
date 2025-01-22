// Based on the work by DFRobot

#include "DFRobot_LiquidCrystal_I2C.h"
#include <inttypes.h>
#include "Arduino.h"


#define printIIC(args)    Wire.write(args)
void DFRobot_LiquidCrystal_I2C::write(uint8_t value) {
    if(rgbFlag){
        uint8_t data[3] = {0x40, value};
        send1(data, 2);
    }else{
        send(value, Rs);
    }
}



// When the display powers up, it is configured as follows:
//
// 1. Display clear
// 2. Function set: 
//    DL = 1; 8-bit interface data 
//    N = 0; 1-line display 
//    F = 0; 5x8 dot character font 
// 3. Display on/off control: 
//    D = 0; Display off 
//    C = 0; Cursor off 
//    B = 0; Blinking off 
// 4. Entry mode set: 
//    I/D = 1; Increment by 1
//    S = 0; No shift 
//
// Note, however, that resetting the Arduino doesn't reset the LCD, so we
// can't assume that its in that state when a sketch starts (and the
// LiquidCrystal constructor is called).

DFRobot_LiquidCrystal_I2C::DFRobot_LiquidCrystal_I2C()
{
  _Addr = 0x20;
  _cols = 16;
  _rows = 2;
  _backlightval = LCD_NOBACKLIGHT;
  enable = false;
}

void DFRobot_LiquidCrystal_I2C::begin(uint32_t lcd_Addr)
{
  enable = true;
  _Addr = lcd_Addr;
  _backlightval = LCD_NOBACKLIGHT;
  if(lcd_Addr == 0x3e) rgbFlag = true;
  else rgbFlag = false;
  init();
  backlight();
}

void DFRobot_LiquidCrystal_I2C::checkEnable()
{
  if (!enable) {
    enable = true;
    init();
    backlight();
  }
}

void DFRobot_LiquidCrystal_I2C::init()
{
    checkEnable();
    init_priv();
}

void DFRobot_LiquidCrystal_I2C::init_priv()
{
    checkEnable();
    Wire.begin();
    _displayfunction = LCD_4BITMODE | LCD_1LINE | LCD_5x8DOTS;
    begin(_cols, _rows);  
}

void DFRobot_LiquidCrystal_I2C::begin(uint8_t cols, uint8_t lines, uint8_t dotsize)
{
    checkEnable();
    if (lines > 1) {
        _displayfunction |= LCD_2LINE;
    }
    _numlines = lines;

    // for some 1 line displays you can select a 10 pixel high font
    if ((dotsize != 0) && (lines == 1)) {
        _displayfunction |= LCD_5x10DOTS;
    }

    // SEE PAGE 45/46 FOR INITIALIZATION SPECIFICATION!
    // according to datasheet, we need at least 40ms after power rises above 2.7V
    // before sending commands. Arduino can turn on way befer 4.5V so we'll wait 50
    delay(50); 
    if(rgbFlag){
        command(LCD_FUNCTIONSET | _displayfunction);
        delay(5);  // wait more than 4.1ms
        
        ///< second try
        command(LCD_FUNCTIONSET | _displayfunction);
        delay(5);
    }else{
        // Now we pull both RS and R/W low to begin commands
        expanderWrite(_backlightval);    // reset expanderand turn backlight off (Bit 8 =1)
        delay(1000);

        //put the LCD into 4 bit mode
        // this is according to the hitachi HD44780 datasheet
        // figure 24, pg 46
        
          // we start in 8bit mode, try to set 4 bit mode
        write4bits(0x03 << 4);
        delayMicroseconds(4500); // wait min 4.1ms
       
        // second try
        write4bits(0x03 << 4);
        delayMicroseconds(4500); // wait min 4.1ms
       
        // third go!
        write4bits(0x03 << 4); 
        delayMicroseconds(150);
       
        // finally, set to 4-bit interface
        write4bits(0x02 << 4); 
    }
    


    // set # lines, font size, etc.
    command(LCD_FUNCTIONSET | _displayfunction);  
    
    // turn the display on with no cursor or blinking default
    _displaycontrol = LCD_DISPLAYON | LCD_CURSOROFF | LCD_BLINKOFF;
    display();
    
    // clear it off
    clear();
    
    // Initialize to default text direction (for roman languages)
    _displaymode = LCD_ENTRYLEFT | LCD_ENTRYSHIFTDECREMENT;
    
    // set the entry mode
    command(LCD_ENTRYMODESET | _displaymode);
    
    if(rgbFlag){
        setReg(REG_MODE1, 0);
    ///< set LEDs controllable by both PWM and GRPPWM registers
        setReg(REG_OUTPUT, 0xFF);
    ///< set MODE2 values
    ///< 0010 0000 -> 0x20  (DMBLNK to 1, ie blinky mode)
        setReg(REG_MODE2, 0x20);
    
        //setColorWhite();
        setRGB(255, 255, 255);
    }

    home();
  
}

void DFRobot_LiquidCrystal_I2C::setRGB(uint8_t r, uint8_t g, uint8_t b)
{
    checkEnable();
    setReg(REG_RED, r);
    setReg(REG_GREEN, g);
    setReg(REG_BLUE, b);
}

void DFRobot_LiquidCrystal_I2C::setReg(uint8_t addr, uint8_t data)
{
    checkEnable();
    Wire.beginTransmission(0xc0>>1); // transmit to device #4
    Wire.write(addr);
    Wire.write(data);
    Wire.endTransmission();    // stop transmitting
}
/********** high level commands, for the user! */
void DFRobot_LiquidCrystal_I2C::clear()
{
    checkEnable();
    command(LCD_CLEARDISPLAY);// clear display, set cursor position to zero
    delayMicroseconds(2000);  // this command takes a long time!
}

void DFRobot_LiquidCrystal_I2C::home()
{
    checkEnable();
    command(LCD_RETURNHOME);  // set cursor position to zero
    delayMicroseconds(2000);  // this command takes a long time!
}

void DFRobot_LiquidCrystal_I2C::setCursor(uint8_t col, uint8_t row)
{
    checkEnable();
    if(rgbFlag){
        col = (row == 0 ? col|0x80 : col|0xc0);
        uint8_t data[3] = {0x80, col};

        send1(data, 2);
    }else{
        if(row<0) row=0;
        if(row>1) row=1;
        if(col<0) col=0;
        if(col>15) col=15;
        int row_offsets[] = { 0x00, 0x40, 0x14, 0x54 };
        if ( row > _numlines ) {
            row = _numlines-1;    // we count rows starting w/0
        }
        command(LCD_SETDDRAMADDR | (col + row_offsets[row]));
    }
    
}

// Turn the display on/off (quickly)
void DFRobot_LiquidCrystal_I2C::noDisplay() {
    checkEnable();
    _displaycontrol &= ~LCD_DISPLAYON;
    command(LCD_DISPLAYCONTROL | _displaycontrol);
}
void DFRobot_LiquidCrystal_I2C::display() {
    checkEnable();
    _displaycontrol |= LCD_DISPLAYON;
    command(LCD_DISPLAYCONTROL | _displaycontrol);
}

// Turns the underline cursor on/off
void DFRobot_LiquidCrystal_I2C::noCursor() {
    checkEnable();
    _displaycontrol &= ~LCD_CURSORON;
    command(LCD_DISPLAYCONTROL | _displaycontrol);
}
void DFRobot_LiquidCrystal_I2C::cursor() {
    checkEnable();
    _displaycontrol |= LCD_CURSORON;
    command(LCD_DISPLAYCONTROL | _displaycontrol);
}

// Turn on and off the blinking cursor
void DFRobot_LiquidCrystal_I2C::noBlink() {
    checkEnable();
    _displaycontrol &= ~LCD_BLINKON;
    command(LCD_DISPLAYCONTROL | _displaycontrol);
}
void DFRobot_LiquidCrystal_I2C::blink() {
    checkEnable();
    _displaycontrol |= LCD_BLINKON;
    command(LCD_DISPLAYCONTROL | _displaycontrol);
}

// These commands scroll the display without changing the RAM
void DFRobot_LiquidCrystal_I2C::scrollDisplayLeft(void) {
    checkEnable();
    command(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVELEFT);
}
void DFRobot_LiquidCrystal_I2C::scrollDisplayRight(void) {
    checkEnable();
    command(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVERIGHT);
}

// This is for text that flows Left to Right
void DFRobot_LiquidCrystal_I2C::leftToRight(void) {
    checkEnable();
    _displaymode |= LCD_ENTRYLEFT;
    command(LCD_ENTRYMODESET | _displaymode);
}

// This is for text that flows Right to Left
void DFRobot_LiquidCrystal_I2C::rightToLeft(void) {
    checkEnable();
    _displaymode &= ~LCD_ENTRYLEFT;
    command(LCD_ENTRYMODESET | _displaymode);
}

// This will 'right justify' text from the cursor
void DFRobot_LiquidCrystal_I2C::autoscroll(void) {
    checkEnable();
    _displaymode |= LCD_ENTRYSHIFTINCREMENT;
    command(LCD_ENTRYMODESET | _displaymode);
}

// This will 'left justify' text from the cursor
void DFRobot_LiquidCrystal_I2C::noAutoscroll(void) {
    checkEnable();
    _displaymode &= ~LCD_ENTRYSHIFTINCREMENT;
    command(LCD_ENTRYMODESET | _displaymode);
}

// Allows us to fill the first 8 CGRAM locations
// with custom characters
void DFRobot_LiquidCrystal_I2C::createChar(uint8_t location, uint8_t charmap[]) {
    checkEnable();
    location &= 0x7; // we only have 8 locations 0-7
    command(LCD_SETCGRAMADDR | (location << 3));
    for (int i=0; i<8; i++) {
        write(charmap[i]);
    }
}

//createChar with PROGMEM input
void DFRobot_LiquidCrystal_I2C::createChar(uint8_t location, const char *charmap) {
    checkEnable();
    location &= 0x7; // we only have 8 locations 0-7
    command(LCD_SETCGRAMADDR | (location << 3));
    for (int i=0; i<8; i++) {
            write(pgm_read_byte_near(charmap++));
    }
}

// Turn the (optional) backlight off/on
void DFRobot_LiquidCrystal_I2C::noBacklight(void) {
    checkEnable();
    _backlightval=LCD_NOBACKLIGHT;
    expanderWrite(0);
}

void DFRobot_LiquidCrystal_I2C::backlight(void) {
    checkEnable();
    _backlightval=LCD_BACKLIGHT;
    expanderWrite(0);
}



/*********** mid level commands, for sending data/cmds */

inline void DFRobot_LiquidCrystal_I2C::command(uint8_t value) {
    checkEnable();
    if(rgbFlag){
        uint8_t data[3] = {0x80, value};
        send1(data, 2);
    }else{
        send(value, 0);
    }
    
}


/************ low level data pushing commands **********/

void DFRobot_LiquidCrystal_I2C::send1(uint8_t *data, uint8_t len)
{
    checkEnable();
    Wire.beginTransmission(_Addr);        // transmit to device #4
    for(int i=0; i<len; i++) {
        Wire.write(data[i]);
        delay(5);
    }
    Wire.endTransmission();                     // stop transmitting
}

// write either command or data
void DFRobot_LiquidCrystal_I2C::send(uint8_t value, uint8_t mode) {
    checkEnable();
    uint8_t highnib=value&0xf0;
    uint8_t lownib=(value<<4)&0xf0;
    write4bits((highnib)|mode);
    write4bits((lownib)|mode); 
}

void DFRobot_LiquidCrystal_I2C::write4bits(uint8_t value) {
    checkEnable();
    expanderWrite(value);
    pulseEnable(value);
}

void DFRobot_LiquidCrystal_I2C::expanderWrite(uint8_t _data){
    checkEnable();
    Wire.beginTransmission(_Addr);
    printIIC((int)(_data) | _backlightval);
    Wire.endTransmission();   
}

void DFRobot_LiquidCrystal_I2C::pulseEnable(uint8_t _data){
    checkEnable();
    expanderWrite(_data | En);    // En high
    delayMicroseconds(1);        // enable pulse must be >450ns
    
    expanderWrite(_data & ~En);    // En low
    delayMicroseconds(50);        // commands need > 37us to settle
} 


// Alias functions

void DFRobot_LiquidCrystal_I2C::cursor_on(){
    checkEnable();
    cursor();
}

void DFRobot_LiquidCrystal_I2C::cursor_off(){
    checkEnable();
    noCursor();
}

void DFRobot_LiquidCrystal_I2C::blink_on(){
    checkEnable();
    blink();
}

void DFRobot_LiquidCrystal_I2C::blink_off(){
    checkEnable();
    noBlink();
}

void DFRobot_LiquidCrystal_I2C::load_custom_character(uint8_t char_num, uint8_t *rows){
    checkEnable();
    createChar(char_num, rows);
}

void DFRobot_LiquidCrystal_I2C::setBacklight(uint8_t new_val){
    checkEnable();
    if(new_val){
        backlight();        // turn backlight on
    }else{
        noBacklight();        // turn backlight off
    }
}

void DFRobot_LiquidCrystal_I2C::print(uint32_t x, uint32_t y, const char *pCh)
{
    checkEnable();
    setCursor(x, y);
    while(*pCh)
    {
        write(*pCh);
        pCh++;;
    }
}

void DFRobot_LiquidCrystal_I2C::print(uint32_t x, uint32_t y, double f, int precision)
{
    String str = String(f,5);

    char *p = (char *)(str.c_str() + str.length());
    while(*p == '\0'|| *p == '0'){
        *p = '\0';
        p--;
    }
    if(*p == '.')
        *p = '\0';
    if(str == "-0")
        str = "0";
    print(x, y, str);
}

void DFRobot_LiquidCrystal_I2C::print(uint32_t x, uint32_t y, uint32_t i)
{
    char buffer[34];
    memset(buffer, 0, 34);
    itoa(i, buffer, 10);
    print(x, y, buffer);
}

void DFRobot_LiquidCrystal_I2C::print(uint32_t x, uint32_t y, int32_t i)
{
    char buffer[34];
    memset(buffer, 0, 34);
    itoa(i, buffer, 10);
    print(x, y, buffer);
}

void DFRobot_LiquidCrystal_I2C::printLine(uint32_t line, const char *pCh)
{
    checkEnable();
    setCursor(0, line-1);
    uint8_t writeBuffer[16];
    memset(writeBuffer, ' ', 16);
    int len = strlen(pCh);
    if(len < 16)
        int _len = 16-len;
    if(len > 16) len = 16;
    for(int i = 0; i < len; i++)
    {
        writeBuffer[i] = pCh[i];
    }
    for(int i = 0; i < 16; i++)
    {
        write(writeBuffer[i]);
    }

}

void DFRobot_LiquidCrystal_I2C::printLine(uint32_t line, double f, int precision)
{
    String str = String(f,5);
    char *p = (char *)(str.c_str() + str.length());
    while(*p == '\0'|| *p == '0'){
        *p = '\0';
        p--;
    }
    if(*p == '.')
        *p = '\0';
    if(str == "-0")
        str = "0";
    printLine(line, str);
}

void DFRobot_LiquidCrystal_I2C::printLine(uint32_t line, int32_t i)
{
    char buffer[34];
    memset(buffer, 0, 34);
    itoa(i, buffer, 10);
    printLine(line, buffer);
}

void DFRobot_LiquidCrystal_I2C::printLine(uint32_t line, uint32_t i)
{
    char buffer[34];
    memset(buffer, 0, 34);
    itoa(i, buffer, 10);
    printLine(line, buffer);
}


// unsupported API functions
void DFRobot_LiquidCrystal_I2C::off(){}
void DFRobot_LiquidCrystal_I2C::on(){}
void DFRobot_LiquidCrystal_I2C::setDelay (int cmdDelay,int charDelay) {}
uint8_t DFRobot_LiquidCrystal_I2C::status(){return 0;}
uint8_t DFRobot_LiquidCrystal_I2C::keypad (){return 0;}
uint8_t DFRobot_LiquidCrystal_I2C::init_bargraph(uint8_t graphtype){return 0;}
void DFRobot_LiquidCrystal_I2C::draw_horizontal_graph(uint8_t row, uint8_t column, uint8_t len,  uint8_t pixel_col_end){}
void DFRobot_LiquidCrystal_I2C::draw_vertical_graph(uint8_t row, uint8_t column, uint8_t len,  uint8_t pixel_row_end){}
void DFRobot_LiquidCrystal_I2C::setContrast(uint8_t new_val){}

    
