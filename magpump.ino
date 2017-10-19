#include <Picaso_Const4D.h>
#include <Picaso_Serial_4DLib.h>
// Define Communication for Data Output
#define DataSerial Serial
// Define Communication for Amplifier
#define AmpSerial Serial3
// Define Display Communication
#define DisplaySerial Serial1
Picaso_Serial_4DLib Display(&DisplaySerial);
// Define temperature pin and variables
const int temperature_pin = A0;
unsigned long temperature_time = 0;
float kty_adc, kty_resistance, kty_temperature;
float kty_temperature_wma = 23.0;
float kty_temperature_sum = 0.0;
int kty_temperature_counter = 0;
float kty_temperature_mean = 23.0;
// T vs. R approximated as line b/w 20 and 100 C
float temperature_m = 0.112;
float temperature_b = -88.864;
// Define menu states
String menu = "main";
bool run_traj = false;
// Define selected trajectory
int selected_trajectory = 1;
String trajectory_commands[] = {"i r0 32781",
                                "i r0 32782",
                                "i r0 32783",
                                "i r0 32784",
                                "i r0 32785",
                                "i r0 32786",
                                "i r0 32787",
                                "i r0 32788",
                                "i r0 32789"};
// Define other stuff
unsigned long check_time = 0;
unsigned long output_time = 0;
float x, y;
void setup() {
  // Initialize Communication for Data Output
  DataSerial.begin(115200);
  // Initialize Communication for Amplifier
  AmpSerial.begin(9600);
  // Initialize Display Communication
  Display.Callback4D = mycallback;
  Display.TimeLimit4D   = 5000;
  DisplaySerial.begin(9600);
  ///* Do we need this?
  //--------------------------------Optional reset routine-----------------------------------
  // Reset the Display using D4 of the Arduino
  // If using jumper wires, reverse the logic states below. (logic states reversed!)
  pinMode(4, OUTPUT);
  digitalWrite(4, 0);
  delay(100);
  digitalWrite(4, 1);
  delay(5000);
  //-----------------------------------------END---------------------------------------------
  //*/
  // Initialize Display
  Display.gfx_ScreenMode(LANDSCAPE);
  Display.gfx_BGcolour(WHITE);
  Display.gfx_Cls();
  Display.touch_Set(0);
  delay(200);
  // Initialize temperature pin and variables
  analogReference(INTERNAL1V1);
  pinMode(temperature_pin,INPUT);
  // Draw main menu
  draw_main_menu();
  
}
void loop() {
  // Check for action, maybe do stuff
  if (millis() - check_time > 100) {
    check_time = millis();
    
    if (Display.touch_Get(0) == 1) {
      // Retreive touch position
      x = Display.touch_Get(1);
      y = Display.touch_Get(2);
      // Check main_menu buttons
      if (menu=="main") {
        check_main_buttons();
      // Check trajectory menu buttons
      } else if (menu=="trajectories") {
        check_trajectory_buttons();
      }
      
    }
  }
    
  // Retrieve instantaneous temperature, always do this
  //  Get mean(wma(kty_temperature)) display update as bonus
  //   Triggers CoilTemp emergency if necessary
  if (millis() - temperature_time > 100) {
    temperature_time = millis();
    kty_temperature = read_temperature();
  }
  // Output data, always do this
  //  This is very dependent on DAQ system
  
  if (millis() - output_time > 100) {
    output_time = millis();
    sendInt("17582d45-6afa-11e7-971b-6c0b843e9461", obtain_data_ram(0x0c));   // coil current
    sendInt("256cb785-6afa-11e7-971b-6c0b843e9461", obtain_data_ram(0x32));   // coil position
    sendInt("294ad913-6afa-11e7-971b-6c0b843e9461", obtain_data_ram(0x18));   // coil velocity
    sendInt("a0fb2bc7-a85c-11e7-971b-6c0b843e9461", kty_temperature);       // coil temp
  }
  
  
}
bool in_rectangle(int x, int y, int x1, int y1, int x2, int y2) {
  if (x >= x1 && x <= x2) {
    if (y >= y1 && y <= y2) {
      return true;
    }
    return false;
  }
  return false;
}
// Draw Full Main Menu
void draw_main_menu() {
  // Clear display
  Display.gfx_Cls();
  // Run trajectory button
  Display.gfx_RectangleFilled(1,1,318,119,BLUE);
  Display.txt_BGcolour(BLUE);
  Display.txt_Width(2);
  Display.txt_Height(2);
  Display.txt_MoveCursor(2,2);
  Display.print("Run Trajectory ");
  Display.print(selected_trajectory);
  // Button for trajectories menu
  Display.gfx_RectangleFilled(1,121,318,179,BLUE);
  Display.txt_MoveCursor(6,4);
  Display.txt_BGcolour(BLUE);
  Display.print("Trajectories");
  // Box for temperature display
  Display.gfx_RectangleFilled(1,181,318,238,BLACK);
  Display.txt_BGcolour(BLACK);
  Display.txt_MoveCursor(8,4);
  Display.print("T = ");
  Display.print(kty_temperature_mean);
  Display.print(" C");
}
// Draw stop button
void draw_main_stop() {
  Display.gfx_RectangleFilled(1,1,318,119,RED);
  Display.txt_BGcolour(RED);
  Display.txt_MoveCursor(2,1);
  Display.txt_Width(2);
  Display.txt_Height(2);
  Display.println("Trajectory Running");
  Display.txt_MoveCursor(3,2);
  Display.println("Press to Stop");
}
// Draw run button
void draw_main_run() {
  Display.gfx_RectangleFilled(1,1,318,119,BLUE);
  Display.txt_BGcolour(BLUE);
  Display.txt_MoveCursor(2,2);
  Display.txt_Width(2);
  Display.txt_Height(2);
  Display.print("Run Trajectory ");
  Display.print(selected_trajectory);
}
// Check main menu buttons
void check_main_buttons() {
  // Check run button
  if (in_rectangle(x, y , 1,1,318,119)) {
    // if running then stop
    if (run_traj) {
      AmpSerial.println("i r1 0");    // Need new stop command!
      draw_main_run();
      run_traj = false;
    // if not running then go
    } else {
      AmpSerial.println("i r1 1");
      delay(10);
      AmpSerial.println(trajectory_commands[selected_trajectory]);
      draw_main_stop();
      run_traj = true;
    }
  // Check trajectories menu button
  } else if (in_rectangle(x,y, 1,121,318,179) && !run_traj) {
    draw_trajectory_menu();
    menu = "trajectories";
  }
}
// Draw full trajectory menu
void draw_trajectory_menu() {
  // Clear display
  Display.gfx_Cls();
  // Box for trajectory display
  Display.gfx_RectangleFilled(1,1,159,79,BLACK);
  Display.txt_MoveCursor(2,2);
  Display.txt_Width(2);
  Display.txt_Height(2);
  Display.txt_BGcolour(BLACK);
  Display.print("Traj: ");
  Display.print(selected_trajectory);
  // Button for main menu
  Display.gfx_RectangleFilled(161,1,318,79,BLUE);
  Display.txt_BGcolour(BLUE);
  Display.txt_MoveCursor(1,12);
  Display.print("Return");
  // Button for traj 1
  Display.gfx_RectangleFilled(1,81,79,159, BLUE);
  Display.txt_BGcolour(BLUE);
  Display.txt_MoveCursor(4,2);
  Display.print("1");
  // Button for traj 2
  Display.gfx_RectangleFilled(81,81,159,159, BLUE);
  Display.txt_BGcolour(BLUE);
  Display.txt_MoveCursor(4,7);
  Display.print("2");
  // Button for traj 3
  Display.gfx_RectangleFilled(161,81,239,159, BLUE);
  Display.txt_BGcolour(BLUE);
  Display.txt_MoveCursor(4,12);
  Display.print("3");
  // Button for traj 4
  Display.gfx_RectangleFilled(241,81,318,159, BLUE);
  Display.txt_BGcolour(BLUE);
  Display.txt_MoveCursor(4,17);
  Display.print("4");
  // Button for traj 5
  Display.gfx_RectangleFilled(1,161,79,238, BLUE);
  Display.txt_BGcolour(BLUE);
  Display.txt_MoveCursor(8,2);
  Display.print("5");
  // Button for traj 6
  Display.gfx_RectangleFilled(81,161,159,238, BLUE);
  Display.txt_BGcolour(BLUE);
  Display.txt_MoveCursor(8,7);
  Display.print("6");
  // Button for traj 7
  Display.gfx_RectangleFilled(161,161,239,238, BLUE);
  Display.txt_BGcolour(BLUE);
  Display.txt_MoveCursor(8,12);
  Display.print("7");
  // Button for traj 8
  Display.gfx_RectangleFilled(241,161,318,238, BLUE);
  Display.txt_BGcolour(BLUE);
  Display.txt_MoveCursor(8,17);
  Display.print("8");
}
// Draw selected trajectory
void draw_trajectory_selected() {
  Display.gfx_RectangleFilled(1,1,159,79,BLACK);
  Display.txt_MoveCursor(1,1);
  Display.txt_Width(2);
  Display.txt_Height(2);
  Display.txt_BGcolour(BLACK);
  Display.print("Traj: ");
  Display.print(selected_trajectory);
}
// Check trajectory buttons
void check_trajectory_buttons() {
  
  // Check main menu button
  if (in_rectangle(x,y,161,1,319,79)) {
    draw_main_menu();
    menu = "main";
  // Check trajectory buttons
  } else if (in_rectangle(x,y,1,81,319,239)) {
    // Find new trajectory
    //  Quick search for 2 rows of 4 buttons each
    if (y > 160) {
      selected_trajectory = 5;
    } else {
      selected_trajectory = 1;
    }
    selected_trajectory = selected_trajectory + int(x/80.0);
    draw_trajectory_selected();
          
  }
}
// Read KTY temperature
//  Update Display with mean(wma(kty_temperature))
//   Return instantaneous temperature
float read_temperature() {
  kty_adc = analogRead(temperature_pin);
  kty_resistance = ((kty_adc/1024) * 1.1 * 10000) / (12 - (kty_adc/1024) * 1.1);
  kty_resistance = (2200 * kty_resistance) / (2200 - kty_resistance);
  kty_temperature = temperature_m * kty_resistance + temperature_b;
  kty_temperature_wma = 0.9 * kty_temperature_wma + 0.1 * kty_temperature;
  kty_temperature_sum = kty_temperature_sum + kty_temperature_wma;
  kty_temperature_counter++;
  if (kty_temperature_counter == 9) {
    kty_temperature_mean = kty_temperature_sum / 10.0;
    kty_temperature_sum = 0.0;
    kty_temperature_counter = 0;
    if (menu=="main") {
      Display.gfx_RectangleFilled(1,181,318,238,BLACK);
      Display.txt_Width(2);
      Display.txt_Height(2);
      Display.txt_BGcolour(BLACK);
      Display.txt_MoveCursor(8,4);
      Display.print("T = ");
      Display.print(kty_temperature_mean);
      //Display.print(kty_adc);
      Display.print(" C");
    }
  }
  // Trigger CoilTemp emergency?
  if (kty_temperature_mean > 80) {
    emergency("CoilTemp");
  }
  
  return kty_temperature;
}
// Emergency Amplifier Stop
void emergency(String emergency_str){
  AmpSerial.println("i r1 0");    // Need new emergency stop
  
  Display.gfx_BGcolour(RED);
  Display.gfx_Cls();
  Display.gfx_RectangleFilled(0,0,160,120,BLACK);
  Display.txt_MoveCursor(3,2);
  Display.txt_Width(2);
  Display.txt_Height(2);
  Display.txt_BGcolour(BLACK);
  Display.print("Emergency: ");
  Display.txt_MoveCursor(5,0);
  Display.txt_Width(3);
  Display.txt_Height(3);
  Display.print(emergency_str);
  
  if(emergency_str=="CoilTemp"){
    Display.print(kty_temperature_mean);
    Display.print("C");
  }
  // kill loop(), requires hard reset
  exit(0);
}
// Display connection error
void mycallback(int ErrCode, unsigned char Errorbyte) {
  // Pin 13 has an LED connected on most Arduino boards. Just give it a name
  int led = 13;
  pinMode(led, OUTPUT);
  while (1) {
    digitalWrite(led, HIGH);  // turn the LED on (HIGH is the voltage level)
    delay(200);                // wait for 200 ms
    digitalWrite(led, LOW);    // turn the LED off by making the voltage LOW
    delay(200);                // wait for 200 ms
  }
}
// Sends data to the computer
void sendInt(const String& guid, int data) {
  
  DataSerial.print(guid);
  DataSerial.print(",");
  DataSerial.println(data);
  
  //DataSerial.println(millis());
}
// Gets data from the RAM bank.
long obtain_data_ram(int param_id) {
  AmpSerial.print("g r");
  AmpSerial.println(param_id);
  // Probably long enough for a whole message to get buffered
  // 10 was small, 20 worked, 40 is guaranteed
  delay(20);
  kill_char();  // v
  kill_char();  // space
  bool negative = false;
  long result = 0;
  while (AmpSerial.available()) {
    char val = AmpSerial.read();
    if (val == '-') {
      negative = true;
    } else if (val >= '0' && val <= '9') {
      result *= 10;
      result += (val - '0');
    }
  }
  return result * (negative ? -1 : 1);
}
// Consumes a single character from AmpSerial
void kill_char() {
  while (!AmpSerial.available()) {}
  char val = AmpSerial.read();
}
