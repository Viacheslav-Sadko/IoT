#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>

const char* ssid = "Decode";
const char* password = "1z2x3c4v";
const int ledPin = 2; // LED pin

const int MAX_NOTES = 5;  // Максимальна кількість заміток
const int NOTE_SIZE = 50; // Розмір пам’яті для кожної замітки

ESP8266WebServer server(80);

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
          "input[type='text'] { width: 60%; max-width: 300px; display: inline-block; color: #000000; }" // Чорний колір тексту
          "button.add-task { width: auto; display: inline-block; }"
          "ul { list-style-type: none; padding: 0; }"
          "li { display: flex; justify-content: space-between; align-items: center; padding: 10px; border-bottom: 1px solid #424242; }"
          "li button { margin-left: 10px; }"
          "strong { font-weight: bold; }"
          "</style>";

  html += "</head><body><h1>ESP second</h1>";
  html += "<button onclick='turnLEDOn()'>LED On</button>";
  html += "<button onclick='turnLEDOff()'>LED Off</button><br><br>";

  // Поле для введення завдання та кнопка Add Task в одному рядку
  html += "<input type='text' id='newNote' placeholder='Enter task here'>";
  html += "<button class='add-task' onclick='addNote()'>Add Task</button><br><br>";

  html += "<ul id='notesList'>";

  // Відображення заміток з EEPROM
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
          "function turnLEDOn() {"
          "  var xhttp = new XMLHttpRequest();"
          "  xhttp.open('GET', '/turnLEDOn', true);"
          "  xhttp.send();"
          "}"
          "function turnLEDOff() {"
          "  var xhttp = new XMLHttpRequest();"
          "  xhttp.open('GET', '/turnLEDOff', true);"
          "  xhttp.send();"
          "}"
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
    server.send(400, "text/plain; charset=utf-8", "Parameters missing");
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

void handleTurnLEDOn() {
  digitalWrite(ledPin, HIGH);  // Включаємо світлодіод
  server.send(200, "text/plain; charset=utf-8", "LED is ON");
}

void handleTurnLEDOff() {
  digitalWrite(ledPin, LOW);  // Вимикаємо світлодіод
  server.send(200, "text/plain; charset=utf-8", "LED is OFF");
}

void setup() {
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);  // Світлодіод вимкнений

  Serial.begin(9600);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
  Serial.println(WiFi.localIP());

  server.on("/", HTTP_GET, handleRoot);
  server.on("/addNote", HTTP_POST, handleAddNote);
  server.on("/editNote", HTTP_POST, handleEditNote);
  server.on("/deleteNote", HTTP_POST, handleDeleteNote);
  server.on("/turnLEDOn", HTTP_GET, handleTurnLEDOn);
  server.on("/turnLEDOff", HTTP_GET, handleTurnLEDOff);

  server.begin();
  EEPROM.begin(512); // 512 байт пам'яті для зберігання заміток
}

void loop() {
  server.handleClient();
}