#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#include <FluxGarage_RoboEyes.h>
roboEyes roboEyes; // Create RoboEyes instance

void setup() {
  Serial.begin(115200);
  
  // Startup OLED Display
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C or 0x3D
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }
  
  setupRoboEyes();
}

void setupRoboEyes() {
  // First clear the display
  display.clearDisplay();
  display.display();
  
  // Startup robo eyes
  roboEyes.begin(SCREEN_WIDTH, SCREEN_HEIGHT - 16, 100); // Reserve bottom 16 pixels for text
  
  // Define some automated eyes behaviour
  roboEyes.setAutoblinker(ON, 3, 2); // Start auto blinker animation cycle
  roboEyes.setIdleMode(ON, 2, 2); // Start idle animation cycle (eyes looking in random directions)
  
  // Define eye shapes, all values in pixels
  roboEyes.setWidth(30, 30); // byte leftEye, byte rightEye
  roboEyes.setHeight(30, 30); // byte leftEye, byte rightEye
  roboEyes.setBorderradius(5, 5); // byte leftEye, byte rightEye
  roboEyes.setSpacebetween(10); // int space -> can also be negative

  // Define mood
  roboEyes.setMood(HAPPY); // Start with happy eyes
}

void loop() {
  // Update robot eyes
  roboEyes.update();
}
