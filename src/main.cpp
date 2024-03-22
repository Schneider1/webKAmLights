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

String buildSlider(int led, int value) {
  String ledId = "led" + String(led) + "slider";
  return R"(<div><input type="range" id=")" + ledId + "\""
   + R"( name=")" + ledId + "\""
   + R"( min="0" max=")" + String(maxValue) + "\""
   + R"( step=")" + String(stepSize) + "\"" 
   + R"( value=")" + String(value) + "\""
    + R"( oninput="ledSliderChange(this.value, )" + String(led) + ")\""
   R"(/><label for=")" + ledId + "\">LED" + String(led) + R"(</label></div>)";
}

String SendHTML(int led1v, int led2v)
{
  String ptr = "<!DOCTYPE html> <html>\n";
  ptr += "<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">\n";
  ptr += "<title>LED Control</title>\n";
  ptr += "<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}\n";
  ptr += "body{margin-top: 50px;} h1 {color: #444444;margin: 50px auto 30px;} h3 {color: #444444;margin-bottom: 50px;}\n";
  ptr += ".button {display: block;width: 80px;background-color: #3498db;border: none;color: white;padding: 13px 30px;text-decoration: none;font-size: 25px;margin: 0px auto 35px;cursor: pointer;border-radius: 4px;}\n";
  ptr += ".button-on {background-color: #3498db;}\n";
  ptr += ".button-on:active {background-color: #2980b9;}\n";
  ptr += ".button-off {background-color: #34495e;}\n";
  ptr += ".button-off:active {background-color: #2c3e50;}\n";
  ptr += "p {font-size: 14px;color: #888;margin-bottom: 10px;}\n";
  ptr += "</style>\n";
  ptr += "</head>\n";
  ptr += "<body>\n";
  ptr += "<h1>ESP32 Web Server</h1>\n";
  ptr += "<h3>Using Access Point(AP) Mode</h3>\n";

  // ptr += "<div style='width:100%'><input type='range' min='0' max='"+String(maxValue)+"' value='"+String(led1v)+"'>";

  ptr += "<a class=\"button button-off\" href=\"/led1off\">-</a>\n";
  ptr += "<p>LED1 Status:" + String(led1v) + "</p>";
  ptr += buildSlider(1, led1v);
  ptr += "<a class=\"button button-on\" href=\"/led1on\">+</a>\n";

  ptr += "<a class=\"button button-off\" href=\"/led2off\">-</a>\n";
  ptr += "<p>LED2 Status:" + String(led2v) + "</p>";
  ptr += buildSlider(2, led2v);
  ptr += "<a class=\"button button-on\" href=\"/led2on\">+</a>\n";

  ptr += R"(<script>
  function ledSliderChange(val, led) {
    fetch(`/ledchanged?value=${val}&led=${led}`)
        .then(response => {
            if (!response.ok) {
                throw new Error('Network response was not ok');
            }
            return response.text();
        })
        .then(data => {
            console.log(data);
            document.body.innerHTML = data;
        })
        .catch(error => {
            console.error('There has been a problem with your fetch operation:', error);
        });
}
  </script>
  )";
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

void handleLedChanged()
{
  int rawValue = server.arg("value").toInt();
  int value = fixValueInRange(rawValue) ;
  int led = server.arg("led").toInt();
  if (led == 1)
  {
    led1vol = value;
  }
  else if (led == 2)
  {
    led2vol = value;
  }
  else {
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
