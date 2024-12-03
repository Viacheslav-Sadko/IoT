#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DHT.h>
#include <vector>
#include <Adafruit_Sensor.h>
#include <EEPROM.h>

const char* ssid = "Decode";
const char* password = "1z2x3c4v";

ESP8266WebServer server(80);  // Створення веб-серверу на порту 80

const int heaterPin = 4;      // GPIO2 на ESP8266 (D4 на платі NodeMCU)
#define DHTPIN 5              // Пін для підключення DHT11 (D1)
#define DHTTYPE DHT11         // Вказуємо тип сенсора DHT11
DHT dht(DHTPIN, DHTTYPE);

const int MAX_NOTES = 5;  // Максимальна кількість заміток
const int NOTE_SIZE = 50; // Розмір пам’яті для кожної замітки

// Зчитує замітку з EEPROM
String getNoteFromEEPROM(int index) {
  int addr = index * NOTE_SIZE;
  String note = "";
  for (int i = 0; i < NOTE_SIZE; i++) {
    char ch = EEPROM.read(addr + i);
    if (ch == 0) break;
    note += ch;
  }
  return note;
}

// Записує замітку в EEPROM
void saveNoteToEEPROM(int index, const String& note) {
  int addr = index * NOTE_SIZE;
  for (int i = 0; i < NOTE_SIZE; i++) {
    if (i < note.length()) {
      EEPROM.write(addr + i, note[i]);
    } else {
      EEPROM.write(addr + i, 0); // Очищення залишку
    }
  }
  EEPROM.commit();
}

// Очищує замітку в EEPROM
void clearNoteInEEPROM(int index) {
  int addr = index * NOTE_SIZE;
  for (int i = 0; i < NOTE_SIZE; i++) {
    EEPROM.write(addr + i, 0);
  }
  EEPROM.commit();
}

void handleRoot() {
  String html = "<html><head>";
  html += "<meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<style>"
          "body { font-family: Arial, sans-serif; background-color: #1c1c1c; color: #f5f5f5; text-align: center; }"
          "h1 { color: #2196f3; }"
          "button, input[type='text'] { font-size: 1em; padding: 10px; margin: 10px; color: #1c1c1c; background-color: #2196f3; border: none; border-radius: 5px; }"
          "input[type='text'] { width: 60%; max-width: 300px; display: inline-block; color: #000000; }"
          "button.add-task { width: auto; display: inline-block; }"
          "ul { list-style-type: none; padding: 0; }"
          "li { display: flex; justify-content: space-between; align-items: center; padding: 10px; border-bottom: 1px solid #424242; }"
          "li button { margin-left: 10px; }"
          "</style>";

  html += "</head><body><h1>Heater Control and Notes</h1>";
  html += "<button onclick='turnHeaterOn()'>Heater On</button>";
  html += "<button onclick='turnHeaterOff()'>Heater Off</button><br><br>";
  
  html += "<h2>Temperature: <span id='temperature'></span> °C</h2>";
  html += "<h2>Humidity: <span id='humidity'></span> %</h2>";
  
  html += "<input type='text' id='newNote' placeholder='Enter task here'>";
  html += "<button class='add-task' onclick='addNote()'>Add Task</button><br><br>";

  html += "<ul id='notesList'>";
  for (int i = 0; i < MAX_NOTES; i++) {
    String note = getNoteFromEEPROM(i);
    if (note.length() > 0) {
      html += "<li id='note" + String(i) + "'><span><strong>" + String(i+1) + ". " + note + "</strong></span>";
      html += "<div><button onclick='editNote(" + String(i) + ")'>Edit</button>";
      html += "<button onclick='deleteNote(" + String(i) + ")'>Delete</button></div></li>";
    }
  }
  html += "</ul>";

  html += "<script>"
          "function addNote() {"
          "  var note = document.getElementById('newNote').value;"
          "  if (note) {"
          "    var xhttp = new XMLHttpRequest();"
          "    xhttp.open('POST', '/addNote', true);"
          "    xhttp.setRequestHeader('Content-type', 'application/x-www-form-urlencoded');"
          "    xhttp.send('note=' + encodeURIComponent(note));"
          "    xhttp.onload = function() { if (xhttp.status === 200) location.reload(); };"
          "    document.getElementById('newNote').value = '';"
          "  }"
          "}"
          "function deleteNote(index) {"
          "  var xhttp = new XMLHttpRequest();"
          "  xhttp.open('POST', '/deleteNote', true);"
          "  xhttp.setRequestHeader('Content-type', 'application/x-www-form-urlencoded');"
          "  xhttp.send('index=' + index);"
          "  xhttp.onload = function() { if (xhttp.status === 200) location.reload(); };"
          "}"
          "function editNote(index) {"
          "  var newText = prompt('Edit your task:');"
          "  if (newText !== null) {"
          "    var xhttp = new XMLHttpRequest();"
          "    xhttp.open('POST', '/editNote', true);"
          "    xhttp.setRequestHeader('Content-type', 'application/x-www-form-urlencoded');"
          "    xhttp.send('index=' + index + '&note=' + encodeURIComponent(newText));"
          "    xhttp.onload = function() { if (xhttp.status === 200) location.reload(); };"
          "  }"
          "}"
          "function turnHeaterOn() {"
          "  var xhttp = new XMLHttpRequest();"
          "  xhttp.open('GET', '/heaterOn', true);"
          "  xhttp.send();"
          "}"
          "function turnHeaterOff() {"
          "  var xhttp = new XMLHttpRequest();"
          "  xhttp.open('GET', '/heaterOff', true);"
          "  xhttp.send();"
          "}"
          "function updateSensorData() {"
          "  var xhttp = new XMLHttpRequest();"
          "  xhttp.onreadystatechange = function() {"
          "    if (this.readyState == 4 && this.status == 200) {"
          "      var data = JSON.parse(this.responseText);"
          "      document.getElementById('temperature').innerHTML = data.temperature;"
          "      document.getElementById('humidity').innerHTML = data.humidity;"
          "    }"
          "  };"
          "  xhttp.open('GET', '/sensor', true);"
          "  xhttp.send();"
          "}"
          "setInterval(updateSensorData, 1000);"
          "</script>";

  html += "</body></html>";
  server.send(200, "text/html; charset=utf-8", html);
}

void handleAddNote() {
  if (server.hasArg("note")) {
    String note = server.arg("note");
    for (int i = 0; i < MAX_NOTES; i++) {
      if (getNoteFromEEPROM(i).length() == 0) {
        saveNoteToEEPROM(i, note);
        break;
      }
    }
    server.send(200, "text/plain; charset=utf-8", "Note added");
  } else {
    server.send(400, "text/plain; charset=utf-8", "Note is missing");
  }
}

void handleDeleteNote() {
  if (server.hasArg("index")) {
    int index = server.arg("index").toInt();
    if (index >= 0 && index < MAX_NOTES) {
      clearNoteInEEPROM(index);
      server.send(200, "text/plain; charset=utf-8", "Note deleted");
    } else {
      server.send(400, "text/plain; charset=utf-8", "Invalid index");
    }
  } else {
    server.send(400, "text/plain; charset=utf-8", "Index is missing");
  }
}

void handleEditNote() {
  if (server.hasArg("index") && server.hasArg("note")) {
    int index = server.arg("index").toInt();
    String note = server.arg("note");
    if (index >= 0 && index < MAX_NOTES) {
      saveNoteToEEPROM(index, note);
      server.send(200, "text/plain; charset=utf-8", "Note edited");
    } else {
      server.send(400, "text/plain; charset=utf-8", "Invalid index");
    }
  } else {
    server.send(400, "text/plain; charset=utf-8", "Index or note is missing");
  }
}

void handleHeaterOn() {
  digitalWrite(heaterPin, HIGH);
  server.send(200, "text/plain", "Heater is ON");
}

void handleHeaterOff() {
  digitalWrite(heaterPin, LOW);
  server.send(200, "text/plain", "Heater is OFF");
}

void handleSensorData() {
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();

  if (isnan(temperature) || isnan(humidity)) {
    server.send(500, "application/json", "{\"error\":\"Failed to read from DHT sensor\"}");
    return;
  }

  String json = "{\"temperature\":" + String(temperature) + ",\"humidity\":" + String(humidity) + "}";
  server.send(200, "application/json", json);
}


void setup() {
  Serial.begin(115200);
  EEPROM.begin(MAX_NOTES * NOTE_SIZE);
  dht.begin();
  pinMode(heaterPin, OUTPUT);
  
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  server.on("/", handleRoot);
  server.on("/addNote", HTTP_POST, handleAddNote);
  server.on("/deleteNote", HTTP_POST, handleDeleteNote);
  server.on("/editNote", HTTP_POST, handleEditNote);
  server.on("/heaterOn", HTTP_GET, handleHeaterOff);
  server.on("/heaterOff", HTTP_GET, handleHeaterOn);
  server.on("/sensor", HTTP_GET, handleSensorData);

  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();
}
