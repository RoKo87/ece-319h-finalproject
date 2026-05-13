#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <LittleFS.h>

// PA8 (MSPM0 UART1 TX) -> Wemos pin 15 (RX, GPIO3) — hardware Serial
// PA22 (MSPM0 UART2 RX) <- Wemos pin 16 (TX, GPIO1) — hardware Serial

ESP8266WebServer server(80);
int finalScore = 0;
bool mcConnected = false;
bool uartError = false;
unsigned long hiWemosTime = 0;

void handleBsad() {
  File f = LittleFS.open("/bsad.png", "r");
  if (!f) { Serial.println("bsad.png not found in LittleFS"); server.send(404, "text/plain", "Not found"); return; }
  server.streamFile(f, "image/png");
  f.close();
}

void handleUartError() {
  String html =
    "<html><head><title>PvZ ECE319H</title>"
    "<style>"
      "body{font-family:sans-serif;text-align:center;background:#222;color:#fff;margin-top:60px;}"
      "h1{color:#7fff7f;}"
      "h2{color:#aaa;font-weight:normal;}"
      "h3{color:#ff4444;}"
      "p{color:#aaa;font-size:18px;max-width:400px;margin:0 auto;}"
      "hr{border-color:#444;}"
    "</style>"
    "</head>"
    "<body>"
      "<h1>Plants vs. Zombies ECE319H</h1>"
      "<h2>Made by: Rohan Konda and William Mar</h2>"
      "<hr>"
      "<img src='/bsad.png' style='width:200px;'/>"
      "<h3>Oh no!</h3>"
      "<p>The microcontroller UART reception seems to not be working. "
        "Try checking your wiring connections between the microcontroller and the Wemos, "
        "then restart both devices.</p>"
    "</body></html>";
  server.send(200, "text/html", html);
}

void handleMcsad() {
  File f = LittleFS.open("/mcsad.png", "r");
  if (!f) { Serial.println("mcsad.png not found in LittleFS"); server.send(404, "text/plain", "Not found"); return; }
  Serial.printf("mcsad.png found, size: %d bytes\n", f.size());
  server.streamFile(f, "image/png");
  f.close();
}

void handleConnected() {
  server.send(200, "text/plain", mcConnected ? "1" : "0");
}

void handleDisconnected() {
  String html =
    "<html><head><title>PvZ ECE319H</title>"
    "<style>"
      "body{font-family:sans-serif;text-align:center;background:#222;color:#fff;margin-top:60px;}"
      "h1{color:#ff4444;}"
      "p{color:#aaa;font-size:18px;max-width:400px;margin:0 auto;}"
    "</style>"
    "<script>"
      "setInterval(function(){"
        "fetch('/connected').then(r=>r.text()).then(t=>{"
          "if(t==='1') location.reload();"
        "});"
      "},1000);"
    "</script>"
    "</head>"
    "<body>"
      "<h1>Plants vs. Zombies ECE319H</h1>"
      "<h2>Made by: Rohan Konda and William Mar</h2>"
      "<hr>"
      "<img src='/mcsad.png' style='width:200px;'/>"
      "<h1>Oh no!</h1>"
      "<p>The microcontroller has not been powered on yet. Plug a wire from this computer to the USB port of the microcontroller to proceed.</p>"
    "</body></html>";
  server.send(200, "text/html", html);
}

// Main page — JS polls /score every second and updates display without reloading
void handleRoot() {
  if (uartError) { handleUartError(); return; }
  if (!mcConnected) { handleDisconnected(); return; }
  String html =
    "<html><head><title>PvZ ECE319H</title>"
    "<style>"
      "body{font-family:sans-serif;text-align:center;background:#222;color:#fff;margin-top:60px;}"
      "h1{color:#7fff7f;}"
      "h2{color:#aaa;font-weight:normal;}"
      "#score{font-size:72px;font-weight:bold;color:#ffd700;margin-top:30px;}"
      "hr{border-color:#444;}"
    "</style>"
    "<script>"
      "var lastScore=-1;"
      "function setStatus(msg){"
        "document.getElementById('newScoreMsg').innerText=msg;"
      "}"
      "function refresh(){"
        "fetch('/score').then(r=>r.text()).then(t=>{"
          "var s=parseInt(t);"
          "document.getElementById('score').innerText='Final Score: '+t;"
          "if(lastScore!==-1&&s!==lastScore&&s>0){"
            "setStatus('New score received from microcontroller!');"
          "}"
          "lastScore=s;"
        "});"
      "}"
      "function submitScore(){"
        "var s=parseInt(document.getElementById('score').innerText.replace('Final Score: ',''));"
        "var err=document.getElementById('scoreErr');"
        "if(s<=0){err.style.display='block';return;}"
        "err.style.display='none';"
        "var n=document.getElementById('nameInput').value;"
        "window.open('https://script.google.com/macros/s/AKfycbwV3wjHUOYhBdZ0yq7Xi33JPQ2Nwsn8tjBmioprsyL_zr8V_d-S2Z1nB81VJL-ZF3o8/exec?score='+s+'&name='+encodeURIComponent(n),'_blank');"
        "fetch('/reset');"
        "document.getElementById('score').innerText='Final Score: 0';"
        "lastScore=0;"
        "setStatus('Awaiting score...play the game!');"
        "setTimeout(function(){var f=document.getElementById('sheet');f.src=f.src;},3000);"
      "}"
      "window.onload=function(){"
        "setStatus('Awaiting score...play the game!');"
        "setInterval(refresh,1000);"
      "};"
    "</script>"
    "</head>"
    "<body>"
      "<h1>Plants vs. Zombies ECE319H</h1>"
      "<h2>Made by: Rohan Konda and William Mar</h2>"
      "<hr>"
      "<div id='score'>Final Score: " + String(finalScore) + "</div>"
      "<br><a href='https://docs.google.com/spreadsheets/d/17XLwJf7fK_qbGYVvtzdaBRfzNK1tphRussgSaInHNRA/edit?usp=sharing' "
        "style='color:#88ccff;font-size:20px;'>See the leaderboard here! &#128279;</a>"
      "<br><br>"
      "<input id='nameInput' type='text' placeholder='Enter your name' "
        "style='font-size:18px;padding:8px 12px;border-radius:6px;border:none;width:220px;'/>"
      "&nbsp;"
      "<button onclick='submitScore()' style='font-size:18px;padding:8px 16px;border-radius:6px;border:none;"
        "background:#7fff7f;color:#222;cursor:pointer;'>Update Leaderboard</button>"
      "<br>"
      "<div id='scoreErr' style='display:none;color:#ff4444;font-size:14px;margin-top:6px;'>Score must be higher than 0.</div>"
      "<div id='newScoreMsg' style='color:#fff;font-size:13px;margin-top:6px;'></div>"
      "<br>"
      "<p style='color:#aaa;font-size:13px;'>The leaderboard might take some time to update. Please be patient.</p>"
      "<iframe id='sheet' src='https://docs.google.com/spreadsheets/d/e/2PACX-1vTOj5gcU4e7OCB6JBbpZhmgvxT3BmaDzSKCzIM_cn6rNPPSQdlQu1quPoML2tPq5E_eChavNuVdYiSm/pubhtml?gid=0&amp;single=true&amp;widget=true&amp;headers=false'"
        " style='width:600px;height:400px;border:none;'></iframe>"
    "</body></html>";
  server.send(200, "text/html", html);
}

// Lightweight endpoint the JS polls — returns just the score number
void handleScore() {
  server.send(200, "text/plain", String(finalScore));
}

void handleReset() {
  finalScore = 0;
  server.send(200, "text/plain", "OK");
}

void setup() {
  Serial.begin(115200); // matches UART1.cpp IBRD=21, FBRD=45
  LittleFS.begin();
  WiFi.begin("utexas-iot", "82540726679404513554");
  server.on("/", handleRoot);
  server.on("/score", handleScore);
  server.on("/reset", handleReset);
  server.on("/mcsad.png", handleMcsad);
  server.on("/bsad.png", handleBsad);
  server.on("/connected", handleConnected);
  server.begin();
}

void loop() {
  server.handleClient();
  if (hiWemosTime > 0 && !mcConnected && !uartError && millis() - hiWemosTime > 5000) {
    uartError = true;
  }

  // Read "SCORE:42\n" sent by MSPM0 via PA8 -> Wemos pin 15 (GPIO3/RX)
  if (Serial.available() > 0) {
    String msg = Serial.readStringUntil('\n');
    msg.trim();
    if (msg.startsWith("SCORE:")) {
      finalScore = msg.substring(6).toInt();
    }
    if (msg.startsWith("hi wemos")) {
      Serial.println("hello mc!");
      hiWemosTime = millis();
    }
    if (msg.startsWith("let's begin! :)")) {
      mcConnected = true;
      uartError = false;
      if (WiFi.status() == WL_CONNECTED) Serial.println(WiFi.localIP());
      else Serial.println("Not connected");
    }
    if (msg.startsWith("pma")) {
      Serial.println(WiFi.macAddress());
    }
    if (msg.startsWith("wip")) {
      if (WiFi.status() == WL_CONNECTED) Serial.println(WiFi.localIP());
      else Serial.println("Not connected");
    }
  }
}
