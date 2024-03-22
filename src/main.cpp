#include <WiFi.h>
#include <WebServer.h>

/* Put your SSID & Password */
const char *ssid = "ESP_Julight";  // Enter SSID here
const char *password = "12345678"; // Enter Password here

/* Put IP Address details */
IPAddress local_ip(192, 168, 0, 1);
IPAddress gateway(192, 168, 0, 1);
IPAddress subnet(255, 255, 255, 0);

WebServer server(80);

const int freq = 100000;
const int ledChannel1 = 0;
const int ledChannel2 = 1;
const int resolution = 6;
const int stepSize = 1;

const int maxValue = 1 << resolution;

uint8_t LED1pin = 25;
int led1vol = maxValue >> 1;

uint8_t LED2pin = 27;
int led2vol = maxValue >> 1;

String buildSlider(int led, int value)
{
  String ledId = "led" + String(led) + "slider";
  String formId = "form" + String(led);
  return R"(<form id=")" + formId + R"(" action="/ledchanged" method="post" onchange="this.submit();">
                    <input type="range" id=")" +
         ledId + R"(" name="value" min="0" max=")" + String(maxValue) + R"(" step=")" + String(stepSize) + R"(" value=")" + String(value) + R"(" />
                    <input type="hidden" name="led" value=")" +
         String(led) + R"(" />
            </form>)";
}

String buildStatusText(int value)
{
  return "<p class=\"led_status\">" + String(value) + "</p>";
}

String buildOffButton(int led, int value)
{
  return "<a class=\"button button-off\" href=\"/led" + String(led) + "off\">-</a>\n";
}

String buildOnButton(int led, int value)
{
  return "<a class=\"button button-on\" href=\"/led" + String(led) + "on\">+</a>\n";
}

String buildLedIdHeader(int led)
{
  return "<h3>LED " + String(led) + "</h3>";
}

void buildLedForm(String &ptr, int led, int value)
{
  ptr += R"(<div style='display: flex; flex-direction: column; align-items: center;'>)";
  ptr += buildLedIdHeader(led);
  ptr += buildOnButton(led, value);
  ptr += buildStatusText(value);
  ptr += buildSlider(led, value);
  ptr += buildOffButton(led, value);
  ptr += "</div>";
}

String SendHTML(int led1v, int led2v)
{
  String ptr = "<!DOCTYPE html> <html>\n";
  ptr += "<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">\n";
  ptr += "<title>LED Control</title>\n";
  ptr += "<style>html { font-family: Helvetica; display: inline-block; margin: 0px; text-align: center;}\n";
  ptr += "body{margin: 0; height: 100vh; } h1 {color: #444444;} h3 {color: #444444;}\n";
  ptr += ".button {display: block;width: 80px;background-color: #3498db;border: none;color: white;padding: 13px 30px;text-decoration: none;font-size: 25px;cursor: pointer;border-radius: 4px;}\n";
  ptr += ".button-on {background-color: #3498db;}\n";
  ptr += ".button-on:active {background-color: #2980b9;}\n";
  ptr += ".button-off {background-color: #34495e;}\n";
  ptr += ".button-off:active {background-color: #2c3e50;}\n";
  ptr += ".led_status {font-weight: bold; font-size: 50px; margin: 40px 0px}\n";
  ptr += "p {font-size: 14px;color: #888;}\n";
  ptr += "</style>\n";
  ptr += "</head>\n";
  ptr += "<body>\n";
  ptr += "<div style='height:100%; position: relative;'>";
  ptr += "<div style='position: absolute; top: 50; left: 0; right: 0;'>";
  ptr += "<h1>ESP32 Web Server</h1>\n";
  ptr += "<h3>Using Access Point(AP) Mode</h3>\n";
  ptr += "</div>";

  ptr += "<div style='display: flex; align-items: center; justify-content: center; height: 100%'>";
  buildLedForm(ptr, 1, led1v);
  ptr += "<div style='width: 100px;'></div>";
  buildLedForm(ptr, 2, led2v);
  ptr += "</div>";
  ptr += "</div>";

  
  ptr += "</body>\n";
  ptr += "</html>\n";
  
  return ptr;
}

void handle_OnConnect()
{
  auto html = SendHTML(led1vol, led2vol);
  Serial.println(html);
  server.send(200, "text/html", html);
}

void handle_led1inc()
{
  if (led1vol <= 255 - stepSize)
  {
    led1vol += stepSize;
  }
  server.send(200, "text/html", SendHTML(led1vol, led2vol));
}

void handle_led1dec()
{
  if (led1vol >= stepSize)
  {
    led1vol -= stepSize;
  }
  server.send(200, "text/html", SendHTML(led1vol, led2vol));
}

void handle_led2inc()
{
  if (led2vol <= 255 - stepSize)
  {
    led2vol += stepSize;
  }
  server.send(200, "text/html", SendHTML(led1vol, led2vol));
}

void handle_NotFound()
{
  server.send(404, "text/plain", "Not found");
}

void handle_led2dec()
{
  if (led2vol >= stepSize)
  {
    led2vol -= stepSize;
  }
  server.send(200, "text/html", SendHTML(led1vol, led2vol));
}

int fixValueInRange(int value)
{
  if (value < 0)
  {
    return 0;
  }
  if (value > maxValue)
  {
    return maxValue;
  }
  return value;
}

void interpolatedLedcWrite(int channel, int oldValue, int newValue, int steps)
{
  int stepSize = (newValue - oldValue) / steps;
  // Starting delay value
  float delayTime = 100; // You can adjust this initial delay to your preference
  // Exponential decay factor
  float decayFactor = 0.85; // Adjust this factor to control how quickly the delay shrinks (must be < 1)

  for (int i = 0; i < steps; i++)
  {
    ledcWrite(channel, oldValue + i * stepSize);
    delay((int)delayTime);

    // Apply exponential decay to the delay time
    delayTime *= decayFactor;
  }
  ledcWrite(channel, newValue);
}

void handleLedChanged()
{
  int rawValue = server.arg("value").toInt();
  int value = fixValueInRange(rawValue);
  int led = server.arg("led").toInt();
  if (led == 1)
  {
    interpolatedLedcWrite(ledChannel1, led1vol, value, abs(led1vol - value) / stepSize);
    led1vol = value;
  }
  else if (led == 2)
  {
    interpolatedLedcWrite(ledChannel2, led2vol, value, abs(led2vol - value) / stepSize);
    led2vol = value;
  }
  else
  {
    handle_NotFound();
    return;
  }
  server.send(200, "text/html", SendHTML(led1vol, led2vol));
}

void setup()
{
  ledcSetup(ledChannel1, freq, resolution);
  ledcAttachPin(LED1pin, ledChannel1);

  ledcSetup(ledChannel2, freq, resolution);
  ledcAttachPin(LED2pin, ledChannel2);

  Serial.begin(115200);

  WiFi.begin(ssid, password);

  WiFi.softAP(ssid, password);
  WiFi.softAPConfig(local_ip, gateway, subnet);
  delay(100);

  server.on("/", handle_OnConnect);
  server.on("/led1on", handle_led1inc);
  server.on("/led1off", handle_led1dec);
  server.on("/led2on", handle_led2inc);
  server.on("/led2off", handle_led2dec);
  server.on("/ledchanged", handleLedChanged);
  server.onNotFound(handle_NotFound);

  server.begin();
  Serial.println("HTTP server started");
}
void loop()
{
  server.handleClient();

  ledcWrite(ledChannel1, led1vol);
  ledcWrite(ledChannel2, led2vol);
}
