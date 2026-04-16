#ifndef WIFI_CONFIG_H
#define WIFI_CONFIG_H

const char configPage[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
<title>Cấu hình Wi-Fi</title>
<style>
body { font-family: Arial; text-align: center; background-color: #f4f4f4; }
.container { background: #fff; padding: 20px; margin: 40px auto; width: 300px; border-radius: 8px; }
select, input {
  width: 100%;
  padding: 8px;
  margin: 6px 0;
  box-sizing: border-box;  /* thêm dòng này */
  display: block;          /* đảm bảo xuống dòng */
}

button { padding: 10px; width: 100%; background: #00aaff; color: white; border: none; border-radius: 4px; }
</style>
</head>
<body>
<div class="container">
<h2>Cấu hình Wi-Fi</h2>
<select id="ssid"></select>
<input type="password" id="pass" placeholder="Mật khẩu">
<button onclick="saveWiFi()">Lưu</button>
</div>
<script>
function loadWiFiList(){
  fetch('/scanwifi')
    .then(res => res.json())
    .then(list => {
      let sel = document.getElementById('ssid');
      sel.innerHTML = "";
      list.forEach(ssid => {
        let opt = document.createElement('option');
        opt.value = ssid;
        opt.innerText = ssid;
        sel.appendChild(opt);
      });
    });
}
function saveWiFi(){
  let ssid = document.getElementById('ssid').value;
  let pass = document.getElementById('pass').value;
  fetch(`/savewifi?ssid=${encodeURIComponent(ssid)}&pass=${encodeURIComponent(pass)}`)
    .then(res => res.text())
    .then(msg => alert(msg));
}
loadWiFiList();
</script>
</body>
</html>
)rawliteral";

#endif
