#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <DNSServer.h>
#include <WebServer.h>
#include <WiFiManager.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#include <FluxGarage_RoboEyes.h>
roboEyes roboEyes; // Create RoboEyes instance

// Access point name and password
const char* AP_NAME = "RoboEyes-WiFi";
const char* AP_PASSWORD = "robotface";

// Groq API Key
const char* GROQ_API_KEY = "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";
const char* GROQ_API_URL = "https://api.groq.com/openai/v1/chat/completions";

// Bot Character Configuration
const char* BOT_NAME = "Pixel";
const char* BOT_PERSONALITY = "friendly, quirky robot assistant with a curious nature";
const char* BOT_BACKSTORY = "I was created to help kids and children learn puzzel solving!";
const char* BOT_LIKES = "solving puzzles, blinking my eyes in fun patterns, and helping humans";
const char* BOT_DISLIKES = "power outages, strong magnets, and being disconnected from WiFi";

// WiFi connection state
bool wifiConfigured = false;

// Web server on port 80
WebServer server(80);

// Chat message buffer
String currentMessage = "";
String currentResponse = "";
unsigned long messageDisplayTime = 0;
const unsigned long MESSAGE_DISPLAY_DURATION = 10000; // 10 seconds to display message

// DNS Server for captive portal
DNSServer dnsServer;

void setup() {
  Serial.begin(115200);
  
  // Startup OLED Display
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C or 0x3D
    Serial.println(F("SSD1306 allocation failed"));
    for (;;); // Don't proceed, loop forever
  }
  
  // Initialize WiFi in AP mode and start station mode simultaneously
  setupWiFi();
  
  // Initialize robo eyes
  setupRoboEyes();
  
  // Initialize web server
  setupWebServer();
}

void setupWiFi() {
  WiFiManager wifiManager;
  
  // Display setup instructions
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("Connect to WiFi:");
  
  display.setTextSize(2);
  display.setCursor(0, 12);
  display.println(AP_NAME);
  
  display.setTextSize(1);
  display.setCursor(0, 32);
  display.println("Password:");
  
  display.setTextSize(2);
  display.setCursor(0, 44);
  display.println(AP_PASSWORD);
  
  display.display();
  
  // Keep both AP and STA mode active
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(AP_NAME, AP_PASSWORD);
  
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);
  
  // Configure callback for when WiFi config is complete
  wifiManager.setSaveConfigCallback([]() {
    Serial.println("WiFi config saved");
    wifiConfigured = true;
  });
  
  // Use autoConnect instead of startConfigPortal to maintain both AP and station connection
  // This will attempt to connect to previously saved credentials and fall back to config portal if needed
  wifiManager.setConfigPortalTimeout(300); // 5 minute timeout for portal if needed
  wifiManager.autoConnect(AP_NAME, AP_PASSWORD);
  
  // Even if STA connection fails, keep the AP running
  wifiConfigured = true;
  
  // Show connection information
  displayConnectionInfo();
}

void displayConnectionInfo() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  
  if (WiFi.status() == WL_CONNECTED) {
    display.println("WiFi Connected!");
    display.println("SSID: " + WiFi.SSID());
    display.println("IP: " + WiFi.localIP().toString());
  } else {
    display.println("AP Mode Active");
    display.println("SSID: " + String(AP_NAME));
    display.println("IP: " + WiFi.softAPIP().toString());
  }
  
  display.println("\nHi, I'm " + String(BOT_NAME) + "!");
  display.println("Chat with me now!");
  display.display();
  
  delay(5000); // Show connection info for 5 seconds
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

void setupWebServer() {
  // Set up routes
  server.on("/", HTTP_GET, handleRoot);
  server.on("/chat", HTTP_POST, handleChat);
  
  // Setup DNS for captive portal to redirect all requests
  dnsServer.start(53, "*", WiFi.softAPIP());
  
  // Start the web server
  server.begin();
  Serial.println("Web server started");
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Station IP: " + WiFi.localIP().toString());
  }
  Serial.println("AP IP: " + WiFi.softAPIP().toString());
}

void handleRoot() {
  String html = "<!DOCTYPE html>"
                "<html>"
                "<head>"
                "<title>" + String(BOT_NAME) + " Chat</title>"
                "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
                "<style>"
                "body { font-family: Arial, sans-serif; margin: 0; padding: 20px; max-width: 800px; margin: 0 auto; }"
                "h1 { color: #333; }"
                "#chatbox { border: 1px solid #ccc; height: 300px; padding: 10px; overflow-y: auto; margin-bottom: 10px; }"
                "#message { width: 100%; padding: 8px; box-sizing: border-box; margin-bottom: 10px; }"
                "#send { background-color: #4CAF50; color: white; padding: 10px 15px; border: none; cursor: pointer; }"
                ".user-msg { color: blue; }"
                ".bot-msg { color: green; }"
                ".bot-info { background-color: #f0f0f0; padding: 10px; border-radius: 5px; margin-bottom: 10px; }"
                "</style>"
                "</head>"
                "<body>"
                "<h1>Chat with " + String(BOT_NAME) + "</h1>"
                "<div class=\"bot-info\">"
                "<p><strong>About me:</strong> I'm " + String(BOT_NAME) + ", a " + String(BOT_PERSONALITY) + ".</p>"
                "</div>"
                "<div id=\"chatbox\"></div>"
                "<div>"
                "<input type=\"text\" id=\"message\" placeholder=\"Type your message here...\">"
                "<button id=\"send\" onclick=\"sendMessage()\">Send</button>"
                "</div>"
                "<script>"
                "function sendMessage() {"
                "  var message = document.getElementById('message').value;"
                "  if(message.trim() == '') return;"
                "  var chatbox = document.getElementById('chatbox');"
                "  chatbox.innerHTML += '<p class=\"user-msg\"><strong>You:</strong> ' + message + '</p>';"
                "  chatbox.scrollTop = chatbox.scrollHeight;"
                "  document.getElementById('message').value = '';"
                "  fetch('/chat', {"
                "    method: 'POST',"
                "    headers: {'Content-Type': 'application/x-www-form-urlencoded'},"
                "    body: 'message=' + encodeURIComponent(message)"
                "  })"
                "  .then(response => response.text())"
                "  .then(data => {"
                "    chatbox.innerHTML += '<p class=\"bot-msg\"><strong>" + String(BOT_NAME) + ":</strong> ' + data + '</p>';"
                "    chatbox.scrollTop = chatbox.scrollHeight;"
                "  })"
                "  .catch(error => {"
                "    console.error('Error:', error);"
                "    chatbox.innerHTML += '<p style=\"color:red\">Error sending message!</p>';"
                "  });"
                "}"
                "document.getElementById('message').addEventListener('keypress', function(e) {"
                "  if (e.key === 'Enter') {"
                "    sendMessage();"
                "  }"
                "});"
                "</script>"
                "</body>"
                "</html>";
  
  server.send(200, "text/html", html);
}

void handleChat() {
  if (server.hasArg("message")) {
    String message = server.arg("message");
    Serial.println("Received chat message: " + message);
    
    // Display user message on OLED
    currentMessage = "User: " + message;
    messageDisplayTime = millis();
    
    // Update eyes to show thinking status
    roboEyes.setMood(TIRED);
    
    // Check if we should use local character response
    String response;
    if (isIdentityQuestion(message)) {
      response = generateCharacterResponse(message);
    } else {
      // Use Groq API for other questions
      response = getGroqResponse(message);
    }
    
    // Display bot response on OLED
    currentResponse = response;
    messageDisplayTime = millis(); // Reset timer for new message
    
    // Update eyes to show happy status
    roboEyes.setMood(HAPPY);
    
    // Send response back to client
    server.send(200, "text/plain", response);
  } else {
    server.send(400, "text/plain", "Missing message parameter");
  }
}

bool isIdentityQuestion(String message) {
  message.toLowerCase();
  
  // Check for identity-related questions
  if (message.indexOf("your name") >= 0 || 
      message.indexOf("who are you") >= 0 ||
      message.indexOf("what are you") >= 0 ||
      (message.indexOf("name") >= 0 && message.indexOf("?") >= 0) ||
      message.indexOf("about you") >= 0 ||
      message.indexOf("tell me about yourself") >= 0) {
    return true;
  }
  return false;
}

String generateCharacterResponse(String message) {
  message.toLowerCase();
  
  if (message.indexOf("your name") >= 0 || 
      (message.indexOf("name") >= 0 && message.indexOf("?") >= 0)) {
    return "I'm " + String(BOT_NAME) + "! Nice to meet you!";
  }
  else if (message.indexOf("who are you") >= 0) {
    return "I'm " + String(BOT_NAME) + ", a " + String(BOT_PERSONALITY) + ". " + String(BOT_BACKSTORY);
  }
  else if (message.indexOf("what are you") >= 0) {
    return "I'm a robot assistant with a simple display and WiFi capabilities. My creators call me " + String(BOT_NAME) + "!";
  }
  else if (message.indexOf("about you") >= 0 || message.indexOf("tell me about yourself") >= 0) {
    return "I'm " + String(BOT_NAME) + ", " + String(BOT_BACKSTORY) + " I like " + String(BOT_LIKES) + " and dislike " + String(BOT_DISLIKES) + ".";
  }
  else {
    return "I'm " + String(BOT_NAME) + "! I'm here to chat and assist you. What would you like to talk about?";
  }
}

String getGroqResponse(String message) {
  if (WiFi.status() != WL_CONNECTED) {
    return "I'm " + String(BOT_NAME) + ". I need a WiFi connection to use my full capabilities. Currently in AP mode only.";
  }
  
  HTTPClient http;
  http.begin(GROQ_API_URL);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Authorization", String("Bearer ") + GROQ_API_KEY);
  
  // Create JSON payload with character context
  StaticJsonDocument<1536> doc;
  doc["model"] = "llama3-70b-8192";
  JsonArray messages = doc.createNestedArray("messages");
  
  // Add system message to inform about character
  JsonObject systemMessage = messages.createNestedObject();
  systemMessage["role"] = "system";
  systemMessage["content"] = "You are " + String(BOT_NAME) + ", a " + String(BOT_PERSONALITY) + 
                            ". Your backstory: " + String(BOT_BACKSTORY) + 
                            " Keep your responses brief and friendly, under 100 words. Always stay in character.";
  
  JsonObject userMessage = messages.createNestedObject();
  userMessage["role"] = "user";
  userMessage["content"] = message;
  
  String payload;
  serializeJson(doc, payload);
  
  int httpCode = http.POST(payload);
  String response = "Error contacting API";
  
  if (httpCode == 200) {
    String result = http.getString();
    StaticJsonDocument<2048> responseDoc;
    DeserializationError error = deserializeJson(responseDoc, result);
    
    if (!error) {
      response = responseDoc["choices"][0]["message"]["content"].as<String>();
    }
  }
  
  http.end();
  
  // If the response is empty or there was an error, provide a fallback
  if (response == "Error contacting API") {
    if (message.indexOf("hello") >= 0 || message.indexOf("hi") >= 0) {
      return "Hello there! I'm " + String(BOT_NAME) + ". How can I help you today?";
    } else if (message.indexOf("weather") >= 0) {
      return "I can't check the weather yet, but I hope it's nice outside!";
    } else {
      return "I received your message: \"" + message + "\". I'm still learning! - " + String(BOT_NAME);
    }
  }
  
  return response;
}

void displayMessage() {
  // Reserve bottom section of display for messages
  int displayTextY = SCREEN_HEIGHT - 16;
  
  // Clear the message area
  display.fillRect(0, displayTextY, SCREEN_WIDTH, 16, SSD1306_BLACK);
  
  // Show message if available and not expired
  if (currentMessage.length() > 0 && millis() - messageDisplayTime < MESSAGE_DISPLAY_DURATION) {
    display.setCursor(0, displayTextY);
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    
    // If showing user message and enough time has passed, switch to response
    if (currentMessage.startsWith("User:") && 
        currentResponse.length() > 0 && 
        millis() - messageDisplayTime > 3000) {
      // Display the bot response instead
      String displayText = String(BOT_NAME) + ": " + currentResponse;
      
      // Scroll long messages
      static int scrollPos = 0;
      static unsigned long lastScroll = 0;
      
      if (displayText.length() > 21) { // ~21 chars fit on 128px wide display with font size 1
        if (millis() - lastScroll > 300) { // Scroll faster (300ms)
          scrollPos++;
          if (scrollPos > displayText.length()) {
            scrollPos = 0;
          }
          lastScroll = millis();
        }
        
        display.println(displayText.substring(scrollPos, scrollPos + 21));
      } else {
        display.println(displayText);
      }
    } else {
      // Display the user message
      if (currentMessage.length() > 21) { // ~21 chars fit on 128px wide display with font size 1
        static int scrollPos = 0;
        static unsigned long lastScroll = 0;
        
        if (millis() - lastScroll > 300) { // Scroll faster (300ms)
          scrollPos++;
          if (scrollPos > currentMessage.length()) {
            scrollPos = 0;
          }
          lastScroll = millis();
        }
        
        display.println(currentMessage.substring(scrollPos, scrollPos + 21));
      } else {
        display.println(currentMessage);
      }
    }
  }
}

void checkWiFiStatus() {
  static unsigned long lastCheck = 0;
  
  // Check WiFi status every 30 seconds
  if (millis() - lastCheck > 30000) {
    lastCheck = millis();
    
    if (WiFi.status() != WL_CONNECTED) {
      // Try to reconnect
      Serial.println("WiFi disconnected, attempting to reconnect");
      WiFi.reconnect();
    }
  }
}

void loop() {
  // Handle DNS for captive portal
  dnsServer.processNextRequest();
  
  // Handle web server
  server.handleClient();
  
  // Keep checking and maintaining WiFi connection
  checkWiFiStatus();
  
  // Update robot eyes
  roboEyes.update();
  
  // Draw chat message at bottom of screen
  displayMessage();
  
  // Always display part of screen with messages
  display.display();
}