#ifndef WEB_AIR_H
#define WEB_AIR_H

const char MAIN_page[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Air Monitoring System</title>
  <link href="https://fonts.googleapis.com/css2?family=Poppins:wght@400;600;800&display=swap" rel="stylesheet">
  <script type="text/javascript" src="https://www.gstatic.com/charts/loader.js"></script>
  <style>
    body {
      font-family: 'Poppins', sans-serif;
      margin: 0;
      background: #f6f8fc;
      color: #222;
      line-height: 1.6;
    }
    header {
      display: flex;
      justify-content: center;
      align-items: center;
      padding: 12px 24px;
      background: #fff;
      box-shadow: 0 2px 5px rgba(0,0,0,0.08);
    }
    .container { 
      display: grid; 
      grid-template-columns: 1fr 1fr; 
      gap: 20px; 
      padding: 20px; 
    }
    .card {
      background: #fff;
      border-radius: 14px;
      padding: 20px;
      box-shadow: 0 3px 8px rgba(0,0,0,0.05);
      text-align: center;
    }
    h2 { 
      margin-top: 0; 
      font-size: 20px; 
      font-weight: 600; 
      text-align: center; 
      margin-bottom: 15px; 
    }
    .env-grid { 
      display: grid; 
      grid-template-columns: 1fr 1fr; 
      gap: 10px; 
      font-weight: 500;
    }
    .status-dot.online  { fill: #28a745; }
    .status-dot.offline { fill: #d9534f; }
    footer {
      text-align: center;
      padding: 18px;
      background: #f0f0f0;
      margin-top: 20px;
      font-size: 14px;
      color: #555;
    }

    /* Căn giữa Gauge */
    .gauge-container {
      display: flex;
      justify-content: center;
      align-items: center;
      width: 100%;
    }
    #gauge_speed {
      width: 250px;
      height: 250px;
    }

    /* Căn giữa Compass */
    .compass-container {
      display: flex;
      justify-content: center;
      align-items: center;
      width: 100%;
    }
  </style>
</head>
<body>
  <!-- HEADER -->
  <header>
    <div class="logo">
      <svg width="320" height="60" viewBox="0 0 320 60" xmlns="http://www.w3.org/2000/svg">
        <rect width="320" height="60" rx="12" fill="#5FD1B8"></rect>
        <text x="20" y="38" font-family="Poppins, Arial, sans-serif" font-size="22" fill="white" font-weight="800">
          Air Monitoring System
        </text>
        <circle id="uartStatus" class="status-dot offline" cx="300" cy="30" r="8"></circle>
      </svg>
    </div>
  </header>

  <!-- MAIN -->
  <div class="container">
    <div class="card">
      <h2>Temperature & Humidity</h2>
      <div class="env-grid">
        <p>🌡 Temp: <span id="temp">--</span> °C</p>
        <p>💧 Humidity: <span id="humidity">--</span> %</p>
      </div>
    </div>

    <div class="card">
      <h2>Light & Weather</h2>
      <div class="env-grid">
        <p>🌞 Light: <span id="light">--</span> lux</p>
        <p>⛅ Weather: <span id="weather">--</span></p>
      </div>
    </div>

    <div class="card">
      <h2>💨 Wind Speed</h2>
      <div class="gauge-container">
        <div id="gauge_speed"></div>
      </div>
    </div>

    <div class="card">
      <h2>🧭 Wind Direction</h2>
      <div class="compass-container">
        <svg id="compassSVG" viewBox="0 0 200 200" width="200" height="200">
          <circle cx="100" cy="100" r="90" stroke="#333" stroke-width="6" fill="white"/>
          <text x="100" y="25" text-anchor="middle" font-size="16" font-weight="bold">N</text>
          <text x="175" y="105" text-anchor="middle" font-size="16" font-weight="bold">E</text>
          <text x="100" y="185" text-anchor="middle" font-size="16" font-weight="bold">S</text>
          <text x="25" y="105" text-anchor="middle" font-size="16" font-weight="bold">W</text>
          <polygon id="needle" points="100,40 95,100 105,100" fill="red"/>
          <circle cx="100" cy="100" r="8" fill="#333"/>
        </svg>
      </div>
      <p id="direcText">--</p>
    </div>
  </div>

  <!-- JS -->
  <script>
    google.charts.load('current', {packages:['gauge']});
    let chart, data, options;

    google.charts.setOnLoadCallback(initGauge);

    function initGauge() {
      data = google.visualization.arrayToDataTable([
        ['Label', 'Value'],
        ['m/s', 0]
      ]);

      options = {
        width: 250, height: 250,
        redFrom: 2, redTo: 5,
        yellowFrom: 0, yellowTo: 2,
        minorTicks: 0.01,
        max: 5
      };

      chart = new google.visualization.Gauge(document.getElementById('gauge_speed'));
      chart.draw(data, options);
    }

    async function fetchData() {
      try {
        const res = await fetch('/data');
        if (!res.ok) throw new Error("HTTP error " + res.status);
        const json = await res.json();

        document.getElementById("temp").textContent = json.temperature.toFixed(2);
        document.getElementById("humidity").textContent = json.humidity.toFixed(2);
        document.getElementById("light").textContent = json.Light;
        document.getElementById("weather").textContent = json.weather;

        // Wind Speed Gauge
        data.setValue(0, 1, json.speed);
        chart.draw(data, options);

        // Wind Direction Compass
        updateCompass(json.direc);

        setStatus(true);
      } catch (err) {
        console.error(err);
        setStatus(false);
      }
    }

    function updateCompass(direction) {
      let angle = 0;
      let fullText = "";

      switch(direction) {
        case "N": angle = 0; fullText = "North"; break;
        case "NE": angle = 45; fullText = "North-East"; break;
        case "E": angle = 90; fullText = "East"; break;
        case "SE": angle = 135; fullText = "South-East"; break;
        case "S": angle = 180; fullText = "South"; break;
        case "SW": angle = 225; fullText = "South-West"; break;
        case "W": angle = 270; fullText = "West"; break;
        case "NW": angle = 315; fullText = "North-West"; break;
      }

      // Xoay kim
      document.getElementById("needle")
        .setAttribute("transform","rotate(" + angle + " 100 100)");

      // Hiển thị tên đầy đủ
      document.getElementById("direcText").textContent = fullText;
    }

    function setStatus(isOnline) {
      const dot = document.getElementById("uartStatus");
      if (isOnline) {
        dot.classList.remove("offline");
        dot.classList.add("online");
      } else {
        dot.classList.remove("online");
        dot.classList.add("offline");
      }
    }

    setInterval(fetchData, 1000);
    fetchData();
  </script>
</body>
</html>
)rawliteral";

#endif
