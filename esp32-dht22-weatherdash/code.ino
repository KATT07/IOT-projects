#include <WiFi.h>
#include <WebServer.h>
#include <DHT.h>
#include <time.h>
#include <math.h>

// ============================================================
// WIFI SETTINGS
// ============================================================

const char* WIFI_SSID = "";
const char* WIFI_PASSWORD = "";

// ============================================================
// DHT22 SETTINGS
// ============================================================

#define DHT_PIN 4
#define DHT_TYPE DHT22

DHT dht(DHT_PIN, DHT_TYPE);

// ============================================================
// WEB SERVER
// ============================================================

WebServer server(80);

// ============================================================
// SAMPLING
// ============================================================

const unsigned long SAMPLE_INTERVAL = 30000UL;

// 24 hours:
// 24 * 60 * 60 / 30 = 2880

const uint16_t MAX_SAMPLES = 2880;

// ============================================================
// SENSOR DATA
// ============================================================

struct SensorReading
{
  uint32_t timestamp;
  float temperature;
  float humidity;
  float heatIndex;
};

SensorReading history[MAX_SAMPLES];

uint16_t writeIndex = 0;
uint16_t sampleCount = 0;

unsigned long lastSampleMillis = 0;

// ============================================================
// CURRENT VALUES
// ============================================================

float currentTemperature = NAN;
float currentHumidity = NAN;
float currentHeatIndex = NAN;

// ============================================================
// HEAT INDEX
// ============================================================

float calculateHeatIndex(float temperatureC, float humidity)
{
  float tempF;
  float heatIndexF;
  float heatIndexC;

  // Celsius -> Fahrenheit
  tempF = (1.8f * temperatureC) + 32.0f;

  // Original formula supplied by user
  heatIndexF =
      -42.379f
      + 2.04901523f * tempF
      + 10.14333127f * humidity
      - 0.22475541f * tempF * humidity
      - 0.00683783f * tempF * tempF
      - 0.05481717f * humidity * humidity
      + 0.00122874f * tempF * tempF * humidity
      + 0.00085282f * tempF * humidity * humidity
      - 0.00000199f * tempF * tempF * humidity * humidity;

  // Low humidity correction

  if (
      humidity < 13.0f &&
      tempF > 80.0f &&
      tempF < 112.0f
     )
  {
    heatIndexF =
        heatIndexF
        - ((13.0f - humidity) / 4.0f)
        * sqrtf(
            17.0f
            - fabsf(tempF - 95.0f) / 17.0f
          );
  }

  // High humidity correction

  else if (
      humidity > 85.0f &&
      tempF > 80.0f &&
      tempF < 87.0f
     )
  {
    heatIndexF =
        heatIndexF
        + ((humidity - 85.0f) / 10.0f)
        * ((87.0f - tempF) / 5.0f);
  }

  // Fahrenheit -> Celsius

  heatIndexC =
      (heatIndexF - 32.0f) * 5.0f / 9.0f;

  return heatIndexC;
}

// ============================================================
// TIMESTAMP
// ============================================================

uint32_t getTimestamp()
{
  time_t now;

  time(&now);

  if (now < 100000)
  {
    return 0;
  }

  return (uint32_t)now;
}

// ============================================================
// GET ACTUAL ARRAY INDEX
//
// Returns data in chronological order:
// oldest -> newest
// ============================================================

uint16_t getHistoryIndex(uint16_t position)
{
  if (sampleCount < MAX_SAMPLES)
  {
    return position;
  }

  return (writeIndex + position) % MAX_SAMPLES;
}

// ============================================================
// READ DHT22
// ============================================================

void takeReading()
{
  float temperature;
  float humidity;
  float heatIndex;

  temperature = dht.readTemperature();
  humidity = dht.readHumidity();

  if (
      isnan(temperature) ||
      isnan(humidity)
     )
  {
    Serial.println(
        "ERROR: DHT22 reading failed."
    );

    return;
  }

  heatIndex =
      calculateHeatIndex(
          temperature,
          humidity
      );

  currentTemperature = temperature;
  currentHumidity = humidity;
  currentHeatIndex = heatIndex;

  // Store new reading

  history[writeIndex].timestamp =
      getTimestamp();

  history[writeIndex].temperature =
      temperature;

  history[writeIndex].humidity =
      humidity;

  history[writeIndex].heatIndex =
      heatIndex;

  // Advance circular buffer

  writeIndex++;

  if (writeIndex >= MAX_SAMPLES)
  {
    writeIndex = 0;
  }

  if (sampleCount < MAX_SAMPLES)
  {
    sampleCount++;
  }

  // Serial output

  Serial.println();
  Serial.println(
      "=============================="
  );

  Serial.print("Temperature: ");
  Serial.print(temperature, 2);
  Serial.println(" C");

  Serial.print("Humidity:    ");
  Serial.print(humidity, 2);
  Serial.println(" %");

  Serial.print("Feels Like:  ");
  Serial.print(heatIndex, 2);
  Serial.println(" C");

  Serial.print("Samples:     ");
  Serial.print(sampleCount);
  Serial.print(" / ");
  Serial.println(MAX_SAMPLES);

  Serial.println(
      "=============================="
  );
}

// ============================================================
// SEND A FLOAT ARRAY IN SMALL CHUNKS
//
// This prevents creation of one huge JSON String.
// ============================================================

void sendFloatArray(
    uint8_t dataType
)
{
  String chunk;

  chunk.reserve(4096);

  for (
      uint16_t i = 0;
      i < sampleCount;
      i++
      )
  {
    uint16_t index =
        getHistoryIndex(i);

    float value;

    if (dataType == 0)
    {
      value =
          history[index].temperature;
    }
    else if (dataType == 1)
    {
      value =
          history[index].humidity;
    }
    else
    {
      value =
          history[index].heatIndex;
    }

    if (i > 0)
    {
      chunk += ",";
    }

    char number[20];

    snprintf(
        number,
        sizeof(number),
        "%.2f",
        value
    );

    chunk += number;

    // Flush periodically

    if (chunk.length() >= 3500)
    {
      server.sendContent(chunk);

      chunk = "";
    }
  }

  if (chunk.length() > 0)
  {
    server.sendContent(chunk);
  }
}

// ============================================================
// SEND TIMESTAMP ARRAY IN SMALL CHUNKS
// ============================================================

void sendTimestampArray()
{
  String chunk;

  chunk.reserve(4096);

  for (
      uint16_t i = 0;
      i < sampleCount;
      i++
      )
  {
    uint16_t index =
        getHistoryIndex(i);

    if (i > 0)
    {
      chunk += ",";
    }

    chunk += String(
        history[index].timestamp
    );

    if (chunk.length() >= 3500)
    {
      server.sendContent(chunk);

      chunk = "";
    }
  }

  if (chunk.length() > 0)
  {
    server.sendContent(chunk);
  }
}

// ============================================================
// DATA ENDPOINT
//
// IMPORTANT:
// This streams the response instead of creating a giant String.
// ============================================================

void handleData()
{
  server.setContentLength(
      CONTENT_LENGTH_UNKNOWN
  );

  server.send(
      200,
      "application/json",
      ""
  );

  // ==========================================================
  // TEMPERATURE
  // ==========================================================

  server.sendContent(
      "{\"temperature\":["
  );

  sendFloatArray(0);

  server.sendContent(
      "],"
  );

  // ==========================================================
  // HUMIDITY
  // ==========================================================

  server.sendContent(
      "\"humidity\":["
  );

  sendFloatArray(1);

  server.sendContent(
      "],"
  );

  // ==========================================================
  // HEAT INDEX
  // ==========================================================

  server.sendContent(
      "\"heatIndex\":["
  );

  sendFloatArray(2);

  server.sendContent(
      "],"
  );

  // ==========================================================
  // TIMESTAMPS
  // ==========================================================

  server.sendContent(
      "\"timestamps\":["
  );

  sendTimestampArray();

  server.sendContent(
      "],"
  );

  // ==========================================================
  // SAMPLE COUNT
  // ==========================================================

  String countJson;

  countJson.reserve(32);

  countJson +=
      "\"count\":";

  countJson +=
      String(sampleCount);

  countJson +=
      "}";

  server.sendContent(
      countJson
  );

  server.sendContent("");
}

// ============================================================
// WEB PAGE
// ============================================================

void sendWebPage()
{
  server.setContentLength(
      CONTENT_LENGTH_UNKNOWN
  );

  server.send(
      200,
      "text/html",
      ""
  );

  // ==========================================================
  // HTML + CSS
  // ==========================================================

  server.sendContent(
      "<!DOCTYPE html>"
      "<html>"
      "<head>"
      "<meta charset=\"UTF-8\">"
      "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1.0\">"
      "<title>ESP32 Climate Monitor</title>"

      "<script src=\"https://cdn.jsdelivr.net/npm/chart.js\"></script>"

      "<style>"

      "*{"
      "box-sizing:border-box;"
      "}"

      "body{"
      "margin:0;"
      "background:#111827;"
      "color:#e5e7eb;"
      "font-family:Arial,Helvetica,sans-serif;"
      "}"

      ".header{"
      "background:#0f172a;"
      "padding:22px 25px;"
      "border-bottom:1px solid #374151;"
      "}"

      ".header h1{"
      "margin:0;"
      "font-size:25px;"
      "}"

      ".header p{"
      "margin:7px 0 0 0;"
      "color:#9ca3af;"
      "}"

      ".container{"
      "width:100%;"
      "max-width:1600px;"
      "margin:auto;"
      "padding:20px;"
      "}"

      ".stats{"
      "display:grid;"
      "grid-template-columns:repeat(auto-fit,minmax(210px,1fr));"
      "gap:15px;"
      "margin-bottom:20px;"
      "}"

      ".stat{"
      "background:#1f2937;"
      "border:1px solid #374151;"
      "border-radius:12px;"
      "padding:18px;"
      "}"

      ".stat-title{"
      "color:#9ca3af;"
      "font-size:14px;"
      "margin-bottom:8px;"
      "}"

      ".stat-value{"
      "font-size:30px;"
      "font-weight:600;"
      "}"

      ".graph{"
      "background:#1f2937;"
      "border:1px solid #374151;"
      "border-radius:12px;"
      "padding:20px;"
      "margin-bottom:20px;"
      "}"

      ".graph h2{"
      "margin-top:0;"
      "font-size:19px;"
      "}"

      ".chart-container{"
      "position:relative;"
      "height:330px;"
      "}"

      ".footer{"
      "text-align:center;"
      "color:#6b7280;"
      "font-size:13px;"
      "padding-bottom:20px;"
      "}"

      "</style>"

      "</head>"
      "<body>"

      "<div class=\"header\">"
      "<h1>ESP32 Climate Monitor</h1>"
      "<p>24-hour temperature, humidity and feels-like history</p>"
      "</div>"

      "<div class=\"container\">"

      "<div class=\"stats\">"

      "<div class=\"stat\">"
      "<div class=\"stat-title\">Temperature</div>"
      "<div class=\"stat-value\">"
      "<span id=\"temperature\">--</span> &deg;C"
      "</div>"
      "</div>"

      "<div class=\"stat\">"
      "<div class=\"stat-title\">Humidity</div>"
      "<div class=\"stat-value\">"
      "<span id=\"humidity\">--</span> %"
      "</div>"
      "</div>"

      "<div class=\"stat\">"
      "<div class=\"stat-title\">Feels Like</div>"
      "<div class=\"stat-value\">"
      "<span id=\"heatIndex\">--</span> &deg;C"
      "</div>"
      "</div>"

      "<div class=\"stat\">"
      "<div class=\"stat-title\">Stored Samples</div>"
      "<div class=\"stat-value\">"
      "<span id=\"sampleCount\">--</span>"
      "</div>"
      "</div>"

      "</div>"

      "<div class=\"graph\">"
      "<h2>Temperature</h2>"
      "<div class=\"chart-container\">"
      "<canvas id=\"temperatureChart\"></canvas>"
      "</div>"
      "</div>"

      "<div class=\"graph\">"
      "<h2>Humidity</h2>"
      "<div class=\"chart-container\">"
      "<canvas id=\"humidityChart\"></canvas>"
      "</div>"
      "</div>"

      "<div class=\"graph\">"
      "<h2>Feels Like / Heat Index</h2>"
      "<div class=\"chart-container\">"
      "<canvas id=\"heatIndexChart\"></canvas>"
      "</div>"
      "</div>"

      "<div class=\"footer\">"
      "Sampling every 30 seconds &bull; Maximum history: 24 hours"
      "</div>"

      "</div>"
  );

  // ==========================================================
  // JAVASCRIPT
  // ==========================================================

  server.sendContent(
      "<script>"

      "var temperatureChart=null;"
      "var humidityChart=null;"
      "var heatIndexChart=null;"

      // ========================================================
      // CHART OPTIONS
      // ========================================================

      "function makeOptions(yTitle){"

      "return{"

      "responsive:true,"
      "maintainAspectRatio:false,"
      "animation:false,"

      "interaction:{"
      "mode:'index',"
      "intersect:false"
      "},"

      "scales:{"

      "x:{"

      "ticks:{"
      "color:'#9ca3af',"
      "maxTicksLimit:12,"
      "maxRotation:0"
      "},"

      "grid:{"
      "color:'rgba(255,255,255,0.05)'"
      "}"

      "},"

      "y:{"

      "title:{"
      "display:true,"
      "text:yTitle,"
      "color:'#9ca3af'"
      "},"

      "ticks:{"
      "color:'#9ca3af'"
      "},"

      "grid:{"
      "color:'rgba(255,255,255,0.08)'"
      "}"

      "}"

      "},"

      "plugins:{"

      "legend:{"

      "labels:{"
      "color:'#e5e7eb'"
      "}"

      "}"

      "}"

      "};"

      "}"

      // ========================================================
      // FORMAT TIMESTAMP
      // ========================================================

      "function formatTime(timestamp){"

      "if(!timestamp){"
      "return '';"
      "}"

      "var d=new Date(timestamp*1000);"

      "var day=String(d.getDate()).padStart(2,'0');"

      "var month=String(d.getMonth()+1).padStart(2,'0');"

      "var hour=String(d.getHours()).padStart(2,'0');"

      "var minute=String(d.getMinutes()).padStart(2,'0');"

      "return day+'/'+month+' '+hour+':'+minute;"

      "}"

      // ========================================================
      // CREATE CHARTS
      // ========================================================

      "function createCharts(data){"

      "var labels=data.timestamps.map(formatTime);"

      // Temperature chart

      "temperatureChart=new Chart("
      "document.getElementById('temperatureChart'),"
      "{"

      "type:'line',"

      "data:{"

      "labels:labels,"

      "datasets:[{"

      "label:'Temperature',"
      "data:data.temperature,"
      "borderWidth:2,"
      "pointRadius:0,"
      "pointHitRadius:10,"
      "tension:0.25,"
      "fill:false"

      "}]"

      "},"

      "options:makeOptions('Temperature (C)')"

      "}"

      ");"

      // Humidity chart

      "humidityChart=new Chart("
      "document.getElementById('humidityChart'),"
      "{"

      "type:'line',"

      "data:{"

      "labels:labels,"

      "datasets:[{"

      "label:'Humidity',"
      "data:data.humidity,"
      "borderWidth:2,"
      "pointRadius:0,"
      "pointHitRadius:10,"
      "tension:0.25,"
      "fill:false"

      "}]"

      "},"

      "options:makeOptions('Humidity (%)')"

      "}"

      ");"

      // Heat index chart

      "heatIndexChart=new Chart("
      "document.getElementById('heatIndexChart'),"
      "{"

      "type:'line',"

      "data:{"

      "labels:labels,"

      "datasets:[{"

      "label:'Feels Like',"
      "data:data.heatIndex,"
      "borderWidth:2,"
      "pointRadius:0,"
      "pointHitRadius:10,"
      "tension:0.25,"
      "fill:false"

      "}]"

      "},"

      "options:makeOptions('Feels Like (C)')"

      "}"

      ");"

      "}"

      // ========================================================
      // UPDATE EXISTING CHARTS
      // ========================================================

      "function updateCharts(data){"

      "var labels=data.timestamps.map(formatTime);"

      "temperatureChart.data.labels=labels;"
      "temperatureChart.data.datasets[0].data=data.temperature;"
      "temperatureChart.update('none');"

      "humidityChart.data.labels=labels;"
      "humidityChart.data.datasets[0].data=data.humidity;"
      "humidityChart.update('none');"

      "heatIndexChart.data.labels=labels;"
      "heatIndexChart.data.datasets[0].data=data.heatIndex;"
      "heatIndexChart.update('none');"

      "}"

      // ========================================================
      // LOAD DATA
      // ========================================================

      "async function loadData(){"

      "try{"

      "var response=await fetch('/data',{cache:'no-store'});"

      "if(!response.ok){"
      "throw new Error('HTTP '+response.status);"
      "}"

      "var data=await response.json();"

      "if(data.count>0){"

      "var last=data.count-1;"

      "document.getElementById('temperature').textContent="
      "Number(data.temperature[last]).toFixed(2);"

      "document.getElementById('humidity').textContent="
      "Number(data.humidity[last]).toFixed(2);"

      "document.getElementById('heatIndex').textContent="
      "Number(data.heatIndex[last]).toFixed(2);"

      "}"

      "document.getElementById('sampleCount').textContent="
      "data.count;"

      "if(temperatureChart===null){"

      "createCharts(data);"

      "}else{"

      "updateCharts(data);"

      "}"

      "}catch(error){"

      "console.error('Unable to load ESP32 data:',error);"

      "}"

      "}"

      // ========================================================
      // FIRST LOAD
      // ========================================================

      "loadData();"

      // ========================================================
      // REFRESH EVERY 30 SECONDS
      // ========================================================

      "setInterval(loadData,30000);"

      "</script>"

      "</body>"
      "</html>"
  );

  server.sendContent("");
}

// ============================================================
// WIFI
// ============================================================

void connectWiFi()
{
  Serial.println();

  Serial.print(
      "Connecting to WiFi: "
  );

  Serial.println(
      WIFI_SSID
  );

  WiFi.mode(
      WIFI_STA
  );

  WiFi.begin(
      WIFI_SSID,
      WIFI_PASSWORD
  );

  while (
      WiFi.status() != WL_CONNECTED
        )
  {
    delay(500);

    Serial.print(".");
  }

  Serial.println();

  Serial.println(
      "WiFi connected."
  );

  Serial.print(
      "IP address: "
  );

  Serial.println(
      WiFi.localIP()
  );
}

// ============================================================
// NTP
// ============================================================

void setupTime()
{
  Serial.println();

  Serial.println(
      "Synchronizing time..."
  );

  // India = UTC + 5:30

  configTime(
      19800,
      0,
      "pool.ntp.org",
      "time.nist.gov",
      "time.google.com"
  );

  struct tm timeInfo;

  for (
      int i = 0;
      i < 20;
      i++
      )
  {
    if (
        getLocalTime(
            &timeInfo
        )
       )
    {
      Serial.println(
          "Time synchronized."
      );

      Serial.printf(
          "Time: %02d/%02d/%04d %02d:%02d:%02d\n",

          timeInfo.tm_mday,

          timeInfo.tm_mon + 1,

          timeInfo.tm_year + 1900,

          timeInfo.tm_hour,

          timeInfo.tm_min,

          timeInfo.tm_sec
      );

      return;
    }

    delay(500);
  }

  Serial.println(
      "WARNING: NTP synchronization failed."
  );
}

// ============================================================
// SETUP
// ============================================================

void setup()
{
  Serial.begin(
      115200
  );

  delay(1000);

  Serial.println();

  Serial.println(
      "================================"
  );

  Serial.println(
      "ESP32 DHT22 CLIMATE MONITOR"
  );

  Serial.println(
      "================================"
  );

  dht.begin();

  delay(2000);

  connectWiFi();

  setupTime();

  // ==========================================================
  // WEB SERVER
  // ==========================================================

  server.on(
      "/",
      HTTP_GET,
      sendWebPage
  );

  server.on(
      "/data",
      HTTP_GET,
      handleData
  );

  server.begin();

  Serial.println();

  Serial.println(
      "Web server started."
  );

  Serial.print(
      "Open: http://"
  );

  Serial.print(
      WiFi.localIP()
  );

  Serial.println(
      "/"
  );

  // First reading

  takeReading();

  lastSampleMillis =
      millis();
}

// ============================================================
// LOOP
// ============================================================

void loop()
{
  server.handleClient();

  unsigned long now =
      millis();

  if (
      now - lastSampleMillis >=
      SAMPLE_INTERVAL
     )
  {
    lastSampleMillis =
        now;

    takeReading();
  }
}
