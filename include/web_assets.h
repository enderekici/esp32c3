#pragma once

static const char *DASHBOARD_HTML = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ESP32-C3 Super Mini</title>
    <style>
        :root {
            --bg-color: #121212;
            --card-bg: #1e1e1e;
            --text-primary: #e0e0e0;
            --text-secondary: #a0a0a0;
            --accent: #00e676;
            --danger: #ff5252;
        }
        body {
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            background-color: var(--bg-color);
            color: var(--text-primary);
            margin: 0;
            padding: 20px;
            display: flex;
            justify-content: center;
            align-items: center;
            min-height: 100vh;
        }
        .container {
            width: 100%;
            max-width: 400px;
        }
        .card {
            background-color: var(--card-bg);
            border-radius: 16px;
            padding: 24px;
            box-shadow: 0 4px 20px rgba(0,0,0,0.5);
            text-align: center;
        }
        h1 {
            margin-top: 0;
            font-weight: 300;
            letter-spacing: 1px;
        }
        .status-item {
            margin: 16px 0;
            padding: 12px;
            background: rgba(255,255,255,0.05);
            border-radius: 8px;
            display: flex;
            justify-content: space-between;
            align-items: center;
        }
        .label {
            color: var(--text-secondary);
            font-size: 0.9em;
        }
        .value {
            font-weight: 600;
            color: var(--accent);
        }
        .btn {
            background-color: var(--accent);
            color: #000;
            border: none;
            padding: 12px 24px;
            border-radius: 8px;
            font-size: 1em;
            font-weight: 600;
            cursor: pointer;
            transition: transform 0.2s, opacity 0.2s;
            width: 100%;
            margin-top: 16px;
        }
        .btn:active {
            transform: scale(0.98);
        }
        .btn:disabled {
            opacity: 0.5;
            cursor: not-allowed;
        }
        .loader {
            border: 3px solid rgba(255,255,255,0.1);
            border-top: 3px solid var(--accent);
            border-radius: 50%;
            width: 20px;
            height: 20px;
            animation: spin 1s linear infinite;
            display: inline-block;
            margin-left: 10px;
            vertical-align: middle;
            display: none;
        }
        @keyframes spin { 0% { transform: rotate(0deg); } 100% { transform: rotate(360deg); } }
    </style>
</head>
<body>
    <div class="container">
        <div class="card">
            <h1>ESP32-C3</h1>
            <div class="status-item">
                <span class="label">Firmware</span>
                <span class="value" id="version">Loading...</span>
            </div>
            <div class="status-item">
                <span class="label">Uptime</span>
                <span class="value" id="uptime">00:00:00</span>
            </div>
            <div class="status-item">
                <span class="label">WiFi Signal</span>
                <span class="value" id="rssi">0 dBm</span>
            </div>
            <div class="status-item">
                <span class="label">IP Address</span>
                <span class="value" id="ip">...</span>
            </div>
            
            <button id="updateBtn" class="btn" onclick="triggerUpdate()">
                Check & Update <div class="loader" id="loader"></div>
            </button>
            <p id="msg" style="margin-top: 10px; font-size: 0.9em; color: var(--text-secondary); min-height: 20px;"></p>
        </div>
    </div>

    <script>
        function formatTime(ms) {
            let seconds = Math.floor(ms / 1000);
            let minutes = Math.floor(seconds / 60);
            let hours = Math.floor(minutes / 60);
            seconds = seconds % 60;
            minutes = minutes % 60;
            return `${hours.toString().padStart(2, '0')}:${minutes.toString().padStart(2, '0')}:${seconds.toString().padStart(2, '0')}`;
        }

        async function fetchStatus() {
            try {
                const response = await fetch('/status');
                const data = await response.json();
                document.getElementById('version').textContent = data.version;
                document.getElementById('uptime').textContent = formatTime(data.uptime);
                document.getElementById('rssi').textContent = data.rssi + ' dBm';
                document.getElementById('ip').textContent = data.ip;
            } catch (e) {
                console.error('Failed to fetch status');
            }
        }

        async function triggerUpdate() {
            const btn = document.getElementById('updateBtn');
            const loader = document.getElementById('loader');
            const msg = document.getElementById('msg');
            
            btn.disabled = true;
            loader.style.display = 'inline-block';
            msg.textContent = "Checking for updates...";

            try {
                const response = await fetch('/update', { method: 'POST' });
                const text = await response.text();
                msg.textContent = text;
            } catch (e) {
                msg.textContent = "Request failed";
            }

            setTimeout(() => {
                btn.disabled = false;
                loader.style.display = 'none';
            }, 5000);
        }

        setInterval(fetchStatus, 2000);
        fetchStatus();
    </script>
</body>
</html>
)rawliteral";
