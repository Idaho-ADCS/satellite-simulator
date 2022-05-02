// Import required libraries
#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPmDNS.h>
#include <ESPAsyncWebServer.h>

#include <comm.h>

// Replace with your network credentials
const char* ssid = "TES";
const char* password = "attitude";

const char* PARAM_INPUT_1 = "output";
const char* PARAM_INPUT_2 = "state";

const char* PARAM_INPUT_3 = "command";

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<html>
<head>
  <title>TES Simulator</title>
  <style>
    html { 
      font-family: Arial; 
      background-color: #423f3f;
      display: inline-block;
    }

    header {
      border-radius: 4px;
          font-size: 3.0rem;
      text-align: center;
      left: 0;
      right: 0;
      height: 60px;
      padding: 15px;
      background-size: 100%;
      background-color: #FFFF00;
    }

    div {
      background-color: #757171;
      border-radius: 4px;
      width: 50%;
      text-align: center;
    }

    body {
    }

    form * {
      display: block;
      margin: 10px;
    }

    .container th h1 {
      font-weight: bold;
      font-size: 1em;
      text-align: left;
      color: #185875;
    }
 
    .container td {
      font-weight: normal;
      font-size: 1em;
      -webkit-box-shadow: 0 2px 2px -2px #0e1119;
      -moz-box-shadow: 0 2px 2px -2px #0e1119;
      box-shadow: 0 2px 2px -2px #0e1119;
    }
 
    .container {
      text-align: left;
      overflow: hidden;
      width: 80%;
      margin: 0 auto;
      display: table;
      padding: 0 0 8em 0;
    }
 
    .container td,
    .container th {
      padding-bottom: 2%;
      padding-top: 2%;
      padding-left: 2%;
    }
     
    /* Background-color of the odd rows */
    .container tr:nth-child(odd) {
      background-color: #323c50;
    }
     
    /* Background-color of the even rows */
    .container tr:nth-child(even) {
      background-color: #2c3446;
    }
     
    .container th {
      background-color: #1f2739;
    }
     
    .container td:first-child {
      color: #fb667a;
    }
    
    .switch {position: relative; display: inline-block; width: 120px; height: 68px} 
    .switch input {display: none}
    .slider {position: absolute; top: 0; left: 0; right: 0; bottom: 0; background-color: #ccc; border-radius: 6px}
    .slider:before {position: absolute; content: ""; height: 52px; width: 52px; left: 8px; bottom: 8px; background-color: #fff; -webkit-transition: .4s; transition: .4s; border-radius: 3px}
    input:checked+.slider {background-color: #008000}
    input:checked+.slider:before {-webkit-transform: translateX(52px); -ms-transform: translateX(52px); transform: translateX(52px)}

  </style>

</head>
<body>
  <header><b>TES Simulator</b></header>
  %BUTTONPLACEHOLDER%
  
  <script>
    var intervalId;

    function toggleCheckbox(element) {
      var xhr = new XMLHttpRequest();
      if(element.checked){ xhr.open("GET", "/update?output="+element.id+"&state=1", true); }
      else { xhr.open("GET", "/update?output="+element.id+"&state=0", true); }
      xhr.send();
    }

    function relayCommand(element){
      var xhr = new XMLHttpRequest();
      xhr.open("GET", "/update?output="+element.id+"&state="+element.value, true);
      xhr.send();
    }

    function updateTable(element){
      console.log("updating table");
      var xhr = new XMLHttpRequest();
      xhr.onreadystatechange = function(){
        if(this.readyState == 4 && this.status == 200){
          appendToTable(this);
        }
      }
      xhr.open("GET", "/table", true);
      xhr.send();

    }

    function appendToTable(arg){
      console.log("appending to table");
      var raw_row = arg.responseText; 
      var tbody = document.getElementById('adcs_data').getElementsByTagName('tbody')[0];
      var newRow = tbody.insertRow(-1);
      newRow.innerHTML = raw_row;

    }

    function changeCollectionInterval(element){
      console.log("changed collection interval");
      clearInterval(intervalId);
      intervalId = setInterval(updateTable, parseInt(element.value));

    }

    function setCollectionState(element){
      console.log("changed collection state");
      if(element.checked){
        console.log("ON");
        intervalId = setInterval(updateTable, 1000);

      }else{
        console.log("OFF");
        clearInterval(intervalId);
      }
    }

    function download(filename, text) {
      var element = document.createElement('a');
      element.setAttribute('href', 'data:application/json;charset=utf-8,' + encodeURIComponent(text));
      element.setAttribute('download', filename);

      element.style.display = 'none';
      document.body.appendChild(element);

      element.click();

      document.body.removeChild(element);
    }

  </script>

  <h3>Send Command</h3>
  %DROPDOWNPLACEHOLDER% 
  </select>

  <h3>Collect Data</h3>
  <label class="switch"><input type="checkbox", onchange="setCollectionState(this)"><span class="slider"></span></label>

  <h4>Collection Frequency</h4>
  <label>Collect Once Every...  </label>
    <select name="collection_freq" id="command" onchange="changeCollectionInterval(this);">
      <option value="1000">second</option>
      <option value="2000">2 seconds</option>
      <option value="3000">3 seconds</option>
      <option value="4000">4 seconds</option>
      <option value="5000" selected>5 seconds</option>"
    </select>

  <h3>ADCS Data</h3>
  <table id="adcs_data", onClick="updateTable(this)", class="container">
    <thead>
      <tr>
        <th>Status</th>
        <th>Voltage (V)</th>
        <th>Current (mA)</th>
        <th>Motor RPS</th>
        <th>Mag X (uT)</th>
        <th>Mag Y (uT)</th>
        <th>Mag Z (uT)</th>
        <th>Gyro X (degrees/s)</th>
        <th>Gyro Y (degrees/s)</th>
        <th>Gyro Z (degrees/s)</th>
      </tr>
    </thead>
    <tbody>
    </tbody>
  </table>

  <h3>Download Output</h3>
  <form onsubmit="download(this['name'].value, this['text'].value)">
    <input type="text" name="name" value="data.json">
      <textarea name="text">{
      "a": "b",
      "arr": [
        {
          "c": "d"
        },
        {
          "d": "e"
        }
      ]
      }</textarea>
    <input type="submit" value="Download">
  </form>
  
</body>
</html>
)rawliteral";

String outputState(int output){
  if(digitalRead(output)){
    return "checked";
  }
  else {
    return "";
  }
}

// Replaces placeholder with button section in your web page
String processor(const String& var){
  //Serial.println(var);
  if(var == "BUTTONPLACEHOLDER"){
    String buttons = "";
    buttons += "<h4>ADCS ENABLE</h4><label class=\"switch\"><input type=\"checkbox\" onchange=\"toggleCheckbox(this)\" id=\"2\" " + outputState(2) + "><span class=\"slider\"></span></label>";
    return buttons;
  }else if(var == "DROPDOWNPLACEHOLDER"){
    return "<label for=\"mode\">Command </label><select name=\"command\" id=\"command\" onchange=\"relayCommand(this);\"\"><option value=\"0\" selected>Standby</option><option value=\"1\">Heartbeat</option><option value=\"2\">Detumble</option><option value=\"3\">Orient</option><option value=\"4\">Motion</option>";
  }
  return String();
}

// translate the index into a command to send 
void sendCommand(int cmdIndex){
  switch(cmdIndex){
    case 0: // standby
      send_command(CMD_STANDBY);
      break;
    case 1: // heartbeat
      send_command(CMD_HEARTBEAT);
      break;
    case 2: // orient
      send_command(CMD_TST_SIMPLE_ORIENT);
      break;
    case 3: // motion
      send_command(CMD_TST_BASIC_MOTION);
      break;

    default:
      Serial.println("[ERROR] cmd index selected from webserver is out of bounds");
  }
}


void setup(){

  // Serial port for debugging purposes
  initUSB();
  // Serial1 port for UART commands
  initUART();

  pinMode(2, OUTPUT);
  digitalWrite(2, LOW);
  
  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi.");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  SERCOM_USB.println(".");
  Serial.println("[SUCCESS] Connected to WiFi");

  if(!MDNS.begin("tes-sat")) {
    SERCOM_USB.println("Error starting mDNS");
    return;
  }else{
    SERCOM_USB.println("\tTest Rig Control Panel @ http://tes-sat");
    SERCOM_USB.println("\t\tcontrol click if on Windows to follow link...");

  }

  // Print ESP Local IP Address
  //Serial.println(WiFi.localIP());

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });

  server.on("/table", HTTP_GET, [](AsyncWebServerRequest *request){
    SERCOM_USB.println("updated table");
    String table_row = "<tr><td>OK</td><td>" + String(0) + "</td><td>" + String(0) + "</td><td>" + String(0) + "</td><td>" + String(0) + "</td><td>" + String(0) + "</td><td>" + String(0) + "</td><td>" + String(0) + "</td><td>" + String(0) + "</td><td>" + String(0) + "</td></tr>";
    request->send(200, "text/html", table_row);

  });

  // Send a GET request to <ESP_IP>/update?output=<inputMessage1>&state=<inputMessage2>
  server.on("/update", HTTP_GET, [] (AsyncWebServerRequest *request) {
    Serial.println(request->url());
    int numParams = request->params();

    for(int i = 0; i < numParams; i++){
      AsyncWebParameter *p = request->getParam(i);
      Serial.print("\t");
      Serial.print(p->name());
      Serial.print(" = ");
      Serial.println(p->value());

    }

    String inputMessage1;
    String inputMessage2;
    // GET input1 value on <ESP_IP>/update?output=<inputMessage1>&state=<inputMessage2>
    if (request->hasParam(PARAM_INPUT_1) && request->hasParam(PARAM_INPUT_2)) {
      inputMessage1 = request->getParam(PARAM_INPUT_1)->value();
      inputMessage2 = request->getParam(PARAM_INPUT_2)->value();

      if(inputMessage1.compareTo("command") == 0){
        Serial.print(inputMessage1);
        Serial.print("/mode - set to: ");
        Serial.println(inputMessage2);

        sendCommand(inputMessage2.toInt());

      }else{
        Serial.print("GPIO: ");
        Serial.print(inputMessage1);
        Serial.print(" - Set to: ");
        Serial.println(inputMessage2);
        digitalWrite(inputMessage1.toInt(), inputMessage2.toInt());

      }

    }else {
        Serial.println("No message sent");
    }
    request->send(200, "text/plain", "OK");
  });

  // Start server
  server.begin();
}

void loop() {

}
