#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

#include <Wire.h>
#include <hd44780.h>
#include <hd44780ioClass/hd44780_I2Cexp.h>
#include <pgmspace.h>

// ======================================================
// WIFI
// ======================================================

const char* ssid = "ESP_DISTANCE";
const char* password = "12345678";

ESP8266WebServer server(80);

// ======================================================
// LCD
// ======================================================

hd44780_I2Cexp lcd;

// ======================================================
// PINS
// ======================================================

#define LCD_SDA D2
#define LCD_SCL D1

#define TRIG_PIN D5
#define ECHO_PIN D6

#define BUZZER_PIN D4
#define LED_PIN D7

// ======================================================
// LIMITS
// ======================================================

const float SAFE_DISTANCE = 5.0;
const float WARNING_DISTANCE = 2.0;

const float OBSTACLE_WARNING = 50.0;

// ======================================================
// DATA FROM ESP8266 #2
// ======================================================

float espDistance = 0.0;
float remoteObstacle = 999.0;
int remoteRSSI = 0;

unsigned long lastRemoteUpdate = 0;

// ======================================================
// ULTRASONIC
// ======================================================

float measureDistance()
{
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(3);

  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);

  digitalWrite(TRIG_PIN, LOW);

  unsigned long duration =
    pulseIn(ECHO_PIN, HIGH, 30000);

  if (duration == 0)
    return 999.0;

  return duration * 0.0343 / 2.0;
}

// ======================================================
// RECEIVE ESP #2 DATA
// ======================================================

void receiveData()
{
  if (server.hasArg("distance"))
    espDistance = server.arg("distance").toFloat();

  if (server.hasArg("obstacle"))
    remoteObstacle = server.arg("obstacle").toFloat();

  if (server.hasArg("rssi"))
    remoteRSSI = server.arg("rssi").toInt();

  lastRemoteUpdate = millis();

  server.send(200, "text/plain", "OK");
}

// ======================================================
// API
// ======================================================

void sendAPI()
{
  float localObstacle = measureDistance();

  bool vehicle2Online =
    (millis() - lastRemoteUpdate < 5000);

  String json;

  json.reserve(180);

  json = "{";

  json += "\"distance\":";
  json += String(espDistance, 2);

  json += ",\"rssi\":";
  json += String(remoteRSSI);

  json += ",\"obstacle1\":";
  json += String(localObstacle, 1);

  json += ",\"obstacle2\":";
  json += String(remoteObstacle, 1);

  json += ",\"online\":";
  json += vehicle2Online ? "true" : "false";

  json += "}";

  server.send(
    200,
    "application/json",
    json
  );
}

// ======================================================
// DASHBOARD HTML
// STORED IN FLASH MEMORY
// ======================================================

const char webpage[] PROGMEM = R"rawliteral(

<!DOCTYPE html>
<html>

<head>

<meta name="viewport"
content="width=device-width,initial-scale=1">

<title>Vehicle Safety Monitor</title>

<style>

/* ================================
   BASIC
   ================================ */

*{
box-sizing:border-box;
}

body{
margin:0;
padding:0;
font-family:Arial,Helvetica,sans-serif;
background:
radial-gradient(circle at top,#172a40,#07111e 65%,#040a11);
color:#f4f7fb;
}


/* ================================
   HEADER
   ================================ */

.header{
min-height:90px;
display:flex;
align-items:center;
justify-content:space-between;
padding:18px 3%;
border-bottom:1px solid #26384c;
background:#07111e;
}

.logoArea{
display:flex;
align-items:center;
gap:14px;
}

.logo{
font-size:40px;
}

.title{
font-size:26px;
font-weight:bold;
letter-spacing:.5px;
}

.subtitle{
color:#9aaabd;
font-size:13px;
margin-top:5px;
}

.backend{
display:flex;
align-items:center;
gap:8px;
font-weight:bold;
font-size:14px;
}

.onlineDot{
width:13px;
height:13px;
border-radius:50%;
background:#00e676;
box-shadow:0 0 12px #00e676;
}

.offlineDot{
background:#ff3d3d;
box-shadow:0 0 12px #ff3d3d;
}


/* ================================
   MAIN
   ================================ */

.dashboard{
width:94%;
max-width:1450px;
margin:25px auto;
display:grid;
grid-template-columns:1fr 1fr;
gap:25px;
}


/* ================================
   VEHICLE CARD
   ================================ */

.vehicle{
background:
linear-gradient(145deg,#112338,#081522);
border:1px solid #294158;
border-radius:16px;
padding:20px;
box-shadow:0 8px 30px rgba(0,0,0,.35);
transition:.3s;
}

.vehicle.safe{
border-color:#00a95a;
}

.vehicle.warning{
border-color:#d5a800;
}

.vehicle.danger{
border-color:#d93636;
box-shadow:0 0 22px rgba(255,50,50,.15);
}


/* ================================
   VEHICLE HEADER
   ================================ */

.vehicleHeader{
display:flex;
align-items:center;
justify-content:space-between;
margin-bottom:18px;
}

.vehicleName{
font-size:24px;
font-weight:bold;
}

.signal{
color:#2196f3;
font-size:23px;
margin-right:7px;
}

.status{
padding:9px 18px;
border-radius:22px;
font-weight:bold;
font-size:14px;
}

.status.safe{
color:#00e676;
background:#06351f;
border:1px solid #087a49;
}

.status.warning{
color:#ffc107;
background:#392f05;
border:1px solid #a17d00;
}

.status.danger{
color:#ff5252;
background:#3d1010;
border:1px solid #a52b2b;
}


/* ================================
   CONTENT
   ================================ */

.content{
display:grid;
grid-template-columns:40% 60%;
gap:15px;
}


/* ================================
   DISTANCE
   ================================ */

.distanceBox{
background:#06101c;
border:1px solid #1c3046;
border-radius:11px;
padding:18px;
}

.label{
color:#8fa2b7;
font-size:12px;
font-weight:bold;
letter-spacing:.6px;
}

.distance{
font-size:39px;
font-weight:bold;
margin-top:10px;
}

.unit{
font-size:16px;
font-weight:normal;
color:#9faebe;
}


/* ================================
   SMALL DATA
   ================================ */

.smallGrid{
display:grid;
grid-template-columns:1fr 1fr;
gap:10px;
margin-top:12px;
}

.smallBox{
background:#0b1a2a;
border:1px solid #263a50;
border-radius:9px;
padding:13px;
margin-top:10px;
}

.value{
font-size:19px;
font-weight:bold;
margin-top:7px;
}

.on{
color:#ff5252;
}

.off{
color:#a3afbd;
}


/* ================================
   MAP
   ================================ */

.map{
position:relative;
height:310px;
overflow:hidden;
border-radius:11px;
border:1px solid #526474;

background:
linear-gradient(135deg,
#b7d89b,
#dce9bd 48%,
#a9cf8b);
}


/* roads */

.road{
position:absolute;
height:9px;
background:#e8dfca;
border:2px solid #c5b99e;
z-index:2;
}

.road1{
width:120%;
left:-10%;
top:44%;
transform:rotate(-10deg);
}

.road2{
width:100%;
left:-5%;
top:72%;
transform:rotate(12deg);
}

.road3{
width:75%;
left:20%;
top:25%;
transform:rotate(70deg);
}


/* river */

.river{
position:absolute;
width:180%;
height:35px;
left:-40%;
top:57%;
background:#80b7d5;
transform:rotate(-18deg);
opacity:.7;
z-index:1;
}


/* map controls */

.controls{
position:absolute;
top:10px;
left:10px;
z-index:20;
}

.controls button{
display:block;
width:35px;
height:35px;
border:1px solid #aaa;
background:white;
font-size:22px;
color:#222;
}

.controls button:first-child{
border-radius:5px 5px 0 0;
}

.controls button:last-child{
border-radius:0 0 5px 5px;
}


/* map label */

.mapLabel{
position:absolute;
z-index:10;
background:white;
color:#222;
padding:6px 9px;
border-radius:5px;
font-size:12px;
font-weight:bold;
box-shadow:0 2px 5px #555;
}


/* vehicle marker */

.marker{
position:absolute;
z-index:15;
width:42px;
height:42px;
border-radius:50% 50% 50% 0;
background:#1976d2;
transform:rotate(-45deg);
box-shadow:0 3px 8px #555;
transition:left .7s,top .7s;
}

.marker span{
display:block;
transform:rotate(45deg);
text-align:center;
padding-top:11px;
font-size:17px;
font-weight:bold;
color:white;
}


/* map distance */

.mapDistance{
position:absolute;
top:10px;
left:50%;
transform:translateX(-50%);
background:white;
color:#222;
padding:7px 12px;
border-radius:6px;
font-weight:bold;
z-index:30;
box-shadow:0 2px 6px #555;
}


/* line */

.connection{
position:absolute;
height:4px;
background:#333;
z-index:7;
transform-origin:left center;
}


/* ================================
   LEGEND
   ================================ */

.legend{
width:94%;
max-width:1450px;
margin:0 auto 30px;
padding:20px;
background:#0d1d2e;
border:1px solid #263d55;
border-radius:14px;
display:flex;
justify-content:center;
gap:35px;
flex-wrap:wrap;
}

.legendItem{
display:flex;
align-items:center;
gap:8px;
font-size:13px;
color:#d4dce6;
}

.dot{
width:13px;
height:13px;
border-radius:50%;
}

.green{
background:#00e676;
box-shadow:0 0 8px #00e676;
}

.yellow{
background:#ffc107;
box-shadow:0 0 8px #ffc107;
}

.red{
background:#ff3d4d;
box-shadow:0 0 8px #ff3d4d;
}


/* ================================
   UPDATE
   ================================ */

.lastUpdate{
font-size:11px;
color:#8293a5;
margin-top:12px;
}


/* ================================
   MOBILE
   ================================ */

@media(max-width:900px){

.dashboard{
grid-template-columns:1fr;
}

}

@media(max-width:600px){

.header{
align-items:flex-start;
flex-direction:column;
gap:12px;
}

.title{
font-size:21px;
}

.content{
grid-template-columns:1fr;
}

.map{
height:280px;
}

.distance{
font-size:35px;
}

}

</style>

</head>


<body>


<!-- =========================================
     HEADER
     ========================================= -->

<div class="header">

<div class="logoArea">

<div class="logo">
🚚
</div>

<div>

<div class="title">
MINE VEHICLE SAFETY MONITOR
</div>

<div class="subtitle">
Safe and Efficient Operation in Fog and
Low-Visibility Conditions
</div>

</div>

</div>


<div class="backend">

<div id="online"
class="onlineDot"></div>

<span id="backend">
BACKEND ONLINE
</span>

</div>

</div>



<!-- =========================================
     VEHICLES
     ========================================= -->

<div class="dashboard">


<!-- =========================================
     VEHICLE 1
     ========================================= -->

<div id="vehicle1"
class="vehicle safe">

<div class="vehicleHeader">

<div class="vehicleName">

<span class="signal">◉</span>
VEHICLE 1

</div>

<div id="status1"
class="status safe">

SAFE

</div>

</div>


<div class="content">


<div>


<div class="distanceBox">

<div class="label">
PROXIMITY DISTANCE
</div>

<div class="distance">

<span id="distance1">
--
</span>

<span class="unit">
meters
</span>

</div>

</div>


<div class="smallGrid">


<div class="smallBox">

<div class="label">
⚡ LED
</div>

<div id="led1"
class="value off">
OFF
</div>

</div>


<div class="smallBox">

<div class="label">
🔊 BUZZER
</div>

<div id="buzzer1"
class="value off">
OFF
</div>

</div>

</div>


<div class="smallBox">

<div class="label">
OBSTACLE DISTANCE
</div>

<div class="value">

<span id="obs1">
--
</span>

 cm

</div>

</div>


<div class="smallBox">

<div class="label">
Wi-Fi RSSI
</div>

<div class="value">

<span id="rssi1">
--
</span>

 dBm

</div>

</div>


</div>


<!-- MAP -->

<div class="map">

<div class="river"></div>

<div class="road road1"></div>
<div class="road road2"></div>
<div class="road road3"></div>


<div class="controls">

<button>+</button>
<button>−</button>

</div>


<div class="mapDistance">
<span id="mapDistance1">-- m</span>
</div>


<div class="mapLabel"
style="left:40%;top:22%;">

Vehicle 1

</div>


<div id="marker1"
class="marker"
style="left:50%;top:60%;">

<span>1</span>

</div>


</div>

</div>


<div class="lastUpdate">

◷ Last Updated:
<span id="time1">--</span>

</div>

</div>



<!-- =========================================
     VEHICLE 2
     ========================================= -->

<div id="vehicle2"
class="vehicle warning">

<div class="vehicleHeader">

<div class="vehicleName">

<span class="signal">◉</span>
VEHICLE 2

</div>

<div id="status2"
class="status warning">

WAITING

</div>

</div>


<div class="content">


<div>


<div class="distanceBox">

<div class="label">
PROXIMITY DISTANCE
</div>

<div class="distance">

<span id="distance2">
--
</span>

<span class="unit">
meters
</span>

</div>

</div>


<div class="smallGrid">


<div class="smallBox">

<div class="label">
⚡ LED
</div>

<div id="led2"
class="value off">
OFF
</div>

</div>


<div class="smallBox">

<div class="label">
🔊 BUZZER
</div>

<div id="buzzer2"
class="value off">
OFF
</div>

</div>

</div>


<div class="smallBox">

<div class="label">
OBSTACLE DISTANCE
</div>

<div class="value">

<span id="obs2">
--
</span>

 cm

</div>

</div>


<div class="smallBox">

<div class="label">
Wi-Fi RSSI
</div>

<div class="value">

<span id="rssi2">
--
</span>

 dBm

</div>

</div>


</div>


<!-- MAP -->

<div class="map">

<div class="river"></div>

<div class="road road1"></div>
<div class="road road2"></div>
<div class="road road3"></div>


<div class="controls">

<button>+</button>
<button>−</button>

</div>


<div class="mapDistance">
<span id="mapDistance2">-- m</span>
</div>


<div class="mapLabel"
style="left:40%;top:22%;">

Vehicle 2

</div>


<div id="marker2"
class="marker"
style="left:50%;top:60%;">

<span>2</span>

</div>


</div>

</div>


<div class="lastUpdate">

◷ Last Updated:
<span id="time2">--</span>

</div>

</div>

</div>



<!-- =========================================
     LEGEND
     ========================================= -->

<div class="legend">


<div class="legendItem">

<div class="dot green"></div>

<b>SAFE:</b>

Distance &gt; 5.0 m
(LED OFF, Buzzer OFF)

</div>


<div class="legendItem">

<div class="dot yellow"></div>

<b>WARNING:</b>

2.0 m – 5.0 m
(LED ON, Buzzer OFF)

</div>


<div class="legendItem">

<div class="dot red"></div>

<b>DANGER:</b>

Distance &lt; 2.0 m
(LED ON, Buzzer ON)

</div>


</div>



<script>

// ======================================================
// UPDATE
// ======================================================

function updateDashboard()
{

fetch('/api')

.then(function(response)
{
return response.json();
})

.then(function(data)
{

let distance =
Number(data.distance);

let obstacle1 =
Number(data.obstacle1);

let obstacle2 =
Number(data.obstacle2);

let rssi =
Number(data.rssi);


// ====================================================
// VALUES
// ====================================================

document.getElementById('distance1').innerHTML =
distance.toFixed(1);

document.getElementById('distance2').innerHTML =
distance.toFixed(1);

document.getElementById('mapDistance1').innerHTML =
distance.toFixed(1)+' m';

document.getElementById('mapDistance2').innerHTML =
distance.toFixed(1)+' m';

document.getElementById('obs1').innerHTML =
obstacle1.toFixed(0);

document.getElementById('obs2').innerHTML =
obstacle2.toFixed(0);

document.getElementById('rssi1').innerHTML =
rssi;

document.getElementById('rssi2').innerHTML =
rssi;


// ====================================================
// TIME
// ====================================================

let now =
new Date();

let time =
now.toLocaleTimeString();

document.getElementById('time1').innerHTML =
time;

document.getElementById('time2').innerHTML =
time;


// ====================================================
// ONLINE
// ====================================================

let online =
data.online;

if(online)
{

document.getElementById('online').className =
'onlineDot';

document.getElementById('backend').innerHTML =
'BACKEND ONLINE';

}

else
{

document.getElementById('online').className =
'onlineDot offlineDot';

document.getElementById('backend').innerHTML =
'VEHICLE 2 OFFLINE';

}


// ====================================================
// STATUS
// ====================================================

setVehicleStatus(
1,
distance,
obstacle1
);

setVehicleStatus(
2,
distance,
obstacle2
);


// ====================================================
// MOVE VEHICLE 2
// ====================================================

let marker2 =
document.getElementById('marker2');

let d =
Math.min(Math.max(distance,0.5),10);

let pos =
85 - ((d/10)*60);

marker2.style.left =
pos+'%';


// Change marker color

if(distance < 2)
{
marker2.style.background =
'#d32f2f';
}
else
{
marker2.style.background =
'#1976d2';
}

})

.catch(function(error)
{
console.log(error);
});

}


// ======================================================
// STATUS FUNCTION
// ======================================================

function setVehicleStatus(
vehicle,
distance,
obstacle
)
{

let state =
'SAFE';


// obstacle

if(obstacle <= 50)
{
state = 'DANGER';
}


// ESP distance

else if(
distance > 0 &&
distance < 2
)
{
state = 'DANGER';
}

else if(
distance >= 2 &&
distance <= 5
)
{
state = 'WARNING';
}


// ====================================================
// ELEMENTS
// ====================================================

let card =
document.getElementById(
'vehicle'+vehicle
);

let status =
document.getElementById(
'status'+vehicle
);

let led =
document.getElementById(
'led'+vehicle
);

let buzzer =
document.getElementById(
'buzzer'+vehicle
);


// ====================================================
// REMOVE OLD CLASSES
// ====================================================

card.classList.remove(
'safe',
'warning',
'danger'
);

status.classList.remove(
'safe',
'warning',
'danger'
);


// ====================================================
// SAFE
// ====================================================

if(state === 'SAFE')
{

card.classList.add('safe');

status.classList.add('safe');

status.innerHTML =
'SAFE';

led.innerHTML =
'OFF';

led.className =
'value off';

buzzer.innerHTML =
'OFF';

buzzer.className =
'value off';

}


// ====================================================
// WARNING
// ====================================================

else if(state === 'WARNING')
{

card.classList.add('warning');

status.classList.add('warning');

status.innerHTML =
'WARNING';

led.innerHTML =
'ON';

led.className =
'value on';

buzzer.innerHTML =
'OFF';

buzzer.className =
'value off';

}


// ====================================================
// DANGER
// ====================================================

else
{

card.classList.add('danger');

status.classList.add('danger');

status.innerHTML =
'DANGER';

led.innerHTML =
'ON';

led.className =
'value on';

buzzer.innerHTML =
'ON';

buzzer.className =
'value on';

}

}


// ======================================================
// UPDATE EVERY 1 SECOND
// ======================================================

setInterval(
updateDashboard,
1000
);

updateDashboard();

</script>

</body>

</html>

)rawliteral";

// ======================================================
// ROOT
// ======================================================

void handleRoot()
{
  server.send_P(
    200,
    "text/html",
    webpage
  );
}

// ======================================================
// SETUP
// ======================================================

void setup()
{
  Serial.begin(115200);

  delay(500);

  // ====================================================
  // I2C
  // ====================================================

  Wire.begin(
    LCD_SDA,
    LCD_SCL
  );

  // ====================================================
  // LCD
  // ====================================================

  int lcdStatus =
    lcd.begin(16, 2);

  if (lcdStatus)
  {
    Serial.print("LCD Error: ");
    Serial.println(lcdStatus);
  }

  lcd.backlight();

  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("VEHICLE 1");

  lcd.setCursor(0, 1);
  lcd.print("Starting...");

  // ====================================================
  // GPIO
  // ====================================================

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);

  digitalWrite(BUZZER_PIN, LOW);
  digitalWrite(LED_PIN, LOW);

  // ====================================================
  // WIFI AP
  // ====================================================

  WiFi.mode(WIFI_AP);

  bool apOK =
    WiFi.softAP(
      ssid,
      password
    );

  Serial.println();
  Serial.println("==============================");
  Serial.println("VEHICLE 1");
  Serial.println("==============================");

  Serial.print("AP: ");
  Serial.println(apOK ? "OK" : "FAILED");

  Serial.print("SSID: ");
  Serial.println(ssid);

  Serial.print("IP: ");
  Serial.println(WiFi.softAPIP());

  // ====================================================
  // WEB SERVER
  // ====================================================

  server.on(
    "/",
    handleRoot
  );

  server.on(
    "/api",
    sendAPI
  );

  server.on(
    "/data",
    receiveData
  );

  server.begin();

  Serial.println("Web server started");
  Serial.println("==============================");

  // ====================================================
  // LCD
  // ====================================================

  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("WiFi Ready");

  lcd.setCursor(0, 1);
  lcd.print("192.168.4.1");

  delay(1500);
}

// ======================================================
// LOOP
// ======================================================

void loop()
{
  server.handleClient();

  float obstacle =
    measureDistance();

  // ====================================================
  // LCD
  // ====================================================

  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("D:");
  lcd.print(espDistance, 1);
  lcd.print("m");

  lcd.setCursor(0, 1);
  lcd.print("OBS:");
  lcd.print(obstacle, 0);
  lcd.print("cm");

  // ====================================================
  // STATUS
  // ====================================================

  bool danger = false;
  bool warning = false;

  if (obstacle <= OBSTACLE_WARNING)
  {
    danger = true;
  }

  else if (
    espDistance > 0 &&
    espDistance < WARNING_DISTANCE
  )
  {
    danger = true;
  }

  else if (
    espDistance >= WARNING_DISTANCE &&
    espDistance <= SAFE_DISTANCE
  )
  {
    warning = true;
  }

  // ====================================================
  // OUTPUT
  // ====================================================

  if (danger)
  {
    digitalWrite(LED_PIN, HIGH);
    digitalWrite(BUZZER_PIN, HIGH);
  }

  else if (warning)
  {
    digitalWrite(LED_PIN, HIGH);
    digitalWrite(BUZZER_PIN, LOW);
  }

  else
  {
    digitalWrite(LED_PIN, LOW);
    digitalWrite(BUZZER_PIN, LOW);
  }

  delay(100);
}