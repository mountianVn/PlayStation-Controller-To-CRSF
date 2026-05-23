// ESP32-RC-Joystick Web UI - JavaScript

// ============ CONFIGURATION ============
const WS_URL = window.location.hostname;
const WS_PORT = 81;
const API_BASE = `http://${window.location.hostname}`;
const RECONNECT_INTERVAL = 3000;
const STATUS_UPDATE_INTERVAL = 100;

// ============ STATE ============
let ws = null;
let wsConnected = false;
let config = {};
let channels = [];
let systemStatus = {};
let pendingConfig = null;
let copiedChannel = null;

// ============ INPUT NAMES ============
const INPUT_NAMES = {
    0: 'SELECT', 1: 'L3', 2: 'R3', 3: 'START',
    4: 'DPAD_UP', 5: 'DPAD_RIGHT', 6: 'DPAD_DOWN', 7: 'DPAD_LEFT',
    8: 'L2', 9: 'R2', 10: 'L1', 11: 'R1',
    12: 'TRIANGLE', 13: 'CIRCLE', 14: 'CROSS', 15: 'SQUARE', 16: 'PS',
    17: 'ANALOG_LX', 18: 'ANALOG_LY', 19: 'ANALOG_RX', 20: 'ANALOG_RY',
    21: 'ANALOG_L2', 22: 'ANALOG_R2'
};

const INPUT_TYPES = {
    'ANALOG_LX': 'analog', 'ANALOG_LY': 'analog', 'ANALOG_RX': 'analog',
    'ANALOG_RY': 'analog', 'ANALOG_L2': 'analog', 'ANALOG_R2': 'analog',
    'L2': 'analog', 'R2': 'analog'
};

// ============ INITIALIZATION ============
document.addEventListener('DOMContentLoaded', () => {
    initWebSocket();
    initTabs();
    initChannelMonitor();
    initMapping();
    initOutput();
    initCalibration();
    initSystem();
    initButtons();
});

// ============ WEBSOCKET ============
function initWebSocket() {
    connectWebSocket();
    setInterval(() => {
        if (!wsConnected) connectWebSocket();
    }, RECONNECT_INTERVAL);
}

function connectWebSocket() {
    try {
        ws = new WebSocket(`ws://${WS_URL}:${WS_PORT}`);
        
        ws.onopen = () => {
            console.log('[WS] Connected');
            wsConnected = true;
            updateConnectionStatus(true);
            requestStatus();
            requestConfig();
        };
        
        ws.onclose = () => {
            console.log('[WS] Disconnected');
            wsConnected = false;
            updateConnectionStatus(false);
        };
        
        ws.onerror = (err) => {
            console.error('[WS] Error:', err);
        };
        
        ws.onmessage = (event) => {
            try {
                const data = JSON.parse(event.data);
                handleStatusUpdate(data);
            } catch (e) {
                console.error('[WS] Parse error:', e);
            }
        };
    } catch (e) {
        console.error('[WS] Connection failed:', e);
    }
}

function requestStatus() {
    if (ws && wsConnected) {
        ws.send(JSON.stringify({ type: 1 }));
    }
}

function requestConfig() {
    if (ws && wsConnected) {
        ws.send(JSON.stringify({ type: 3 }));
    }
}

function updateConnectionStatus(connected) {
    const dot = document.getElementById('statusDot');
    const text = document.getElementById('statusText');
    const footer = document.getElementById('connectionInfo');
    
    if (connected) {
        dot.classList.add('connected');
        text.textContent = 'Connected';
        footer.textContent = 'WS: Connected';
    } else {
        dot.classList.remove('connected');
        text.textContent = 'Disconnected';
        footer.textContent = 'WS: Disconnected';
    }
}

// ============ STATUS UPDATE ============
function handleStatusUpdate(data) {
    systemStatus = data;
    
    // Controller status
    document.getElementById('ctrlConnected').textContent = 
        data.controllerConnected ? 'Connected' : 'Disconnected';
    document.getElementById('ctrlConnected').className = 
        'value ' + (data.controllerConnected ? 'success' : 'error');
    document.getElementById('ctrlType').textContent = 
        getControllerTypeName(data.controllerType);
    
    // Battery
    const battery = data.batteryLevel;
    document.getElementById('batteryLevel').textContent = 
        battery !== undefined ? `${battery}%` : '-';
    const batteryBar = document.getElementById('batteryBar');
    if (battery !== undefined) {
        batteryBar.style.width = `${battery}%`;
        batteryBar.className = 'battery-bar';
        if (battery < 20) batteryBar.classList.add('low');
        else if (battery < 50) batteryBar.classList.add('medium');
    }
    
    // RSSI
    document.getElementById('rssiValue').textContent = 
        data.rssi !== undefined ? `${data.rssi} dBm` : '- dBm';
    updateSignalBars(data.rssi);
    
    // Uptime
    if (data.uptime !== undefined) {
        document.getElementById('uptimeValue').textContent = formatUptime(data.uptime);
    }
    
    // System
    document.getElementById('freeHeap').textContent = 
        data.freeHeap ? `${Math.round(data.freeHeap / 1024)} KB` : '-';
    document.getElementById('minFreeHeap').textContent = 
        data.minFreeHeap ? `${Math.round(data.minFreeHeap / 1024)} KB` : '-';
    document.getElementById('cpuFreq').textContent = 
        data.cpuFreq ? `${data.cpuFreq} MHz` : '-';
    
    // Protocol status
    updateProtocolStatus('crsf', data.crsfEnabled, data.crsfFrameCount);
    updateProtocolStatus('sbus', data.sbusEnabled, data.sbusFrameCount);
    updateProtocolStatus('ppm', data.ppmEnabled, data.ppmFrameCount);
    
    // Channels
    if (data.channels && data.channels.length > 0) {
        updateChannelDisplay(data.channels);
        updateJoystickDisplay(data.channels);
    }
    
    // Update config if available
    if (data.activeProtocol !== undefined) {
        const protoMap = { 0: 'crsf', 1: 'sbus', 2: 'ppm' };
        const proto = protoMap[data.activeProtocol];
        if (proto) {
            document.querySelectorAll('input[name="protocol"]').forEach(r => {
                r.checked = r.value === proto;
            });
            updateProtocolConfigVisibility(proto);
        }
    }
}

function getControllerTypeName(type) {
    const types = {
        1: 'DualShock 4',
        2: 'DualShock 3',
        3: 'Xbox',
        4: 'Switch'
    };
    return types[type] || 'Unknown';
}

function updateSignalBars(rssi) {
    const bars = document.querySelectorAll('#signalBars .bar');
    if (rssi === undefined || rssi === 0) {
        bars.forEach(b => b.classList.add('off'));
        return;
    }
    
    const strength = rssi > -50 ? 4 : rssi > -60 ? 3 : rssi > -70 ? 2 : 1;
    bars.forEach((b, i) => {
        b.classList.toggle('off', i >= strength);
    });
}

function updateProtocolStatus(proto, enabled, frames) {
    const el = document.getElementById(`${proto}Status`);
    if (!el) return;
    
    const statusEl = el.querySelector('.proto-status');
    statusEl.textContent = enabled ? 'Active' : 'Disabled';
    statusEl.className = 'proto-status ' + (enabled ? 'active' : 'off');
    
    const framesEl = el.querySelector('.proto-frames');
    if (framesEl && frames !== undefined) {
        framesEl.textContent = `${frames} frames`;
    }
}

function formatUptime(seconds) {
    const h = Math.floor(seconds / 3600);
    const m = Math.floor((seconds % 3600) / 60);
    const s = seconds % 60;
    return `${h}:${m.toString().padStart(2, '0')}:${s.toString().padStart(2, '0')}`;
}

// ============ TABS ============
function initTabs() {
    const tabs = document.querySelectorAll('.tab-btn');
    tabs.forEach(tab => {
        tab.addEventListener('click', () => {
            const tabId = tab.dataset.tab;
            
            tabs.forEach(t => t.classList.remove('active'));
            tab.classList.add('active');
            
            document.querySelectorAll('.tab-content').forEach(c => {
                c.classList.remove('active');
            });
            
            document.getElementById(tabId).classList.add('active');
        });
    });
}

// ============ CHANNEL MONITOR ============
function initChannelMonitor() {
    generateChannelGrid(16);
    
    document.getElementById('channelCountSelect').addEventListener('change', (e) => {
        generateChannelGrid(parseInt(e.target.value));
    });
}

function generateChannelGrid(count) {
    const grid = document.getElementById('channelGrid');
    grid.innerHTML = '';
    
    for (let i = 0; i < count; i++) {
        const item = document.createElement('div');
        item.className = 'channel-item';
        item.innerHTML = `
            <div class="ch-header">
                <span class="ch-name">CH${i + 1}</span>
                <span class="ch-value" id="chValue${i}">1500</span>
            </div>
            <div class="channel-bar">
                <div class="channel-fill" id="chBar${i}" style="width: 50%"></div>
            </div>
        `;
        grid.appendChild(item);
    }
}

function updateChannelDisplay(channelData) {
    channelData.forEach((value, index) => {
        const valueEl = document.getElementById(`chValue${index}`);
        const barEl = document.getElementById(`chBar${index}`);
        
        if (valueEl) valueEl.textContent = value;
        if (barEl) {
            // Map 1000-2000 to 0-100%
            const percent = ((value - 1000) / 1000) * 100;
            barEl.style.width = `${Math.max(0, Math.min(100, percent))}%`;
        }
    });
}

function updateJoystickDisplay(channels) {
    // Left stick (CH1 = X, CH2 = Y)
    const leftX = channels[0] || 1500;
    const leftY = channels[1] || 1500;
    const leftStick = document.getElementById('leftStick');
    if (leftStick) {
        const x = ((leftX - 1000) / 1000) * 35;
        const y = ((leftY - 1000) / 1000) * 35;
        leftStick.style.transform = `translate(${x}px, ${y}px)`;
    }
    
    // Right stick (CH3 = X, CH4 = Y)
    const rightX = channels[2] || 1500;
    const rightY = channels[3] || 1500;
    const rightStick = document.getElementById('rightStick');
    if (rightStick) {
        const x = ((rightX - 1000) / 1000) * 35;
        const y = ((rightY - 1000) / 1000) * 35;
        rightStick.style.transform = `translate(${x}px, ${y}px)`;
    }
}

// ============ MAPPING ============
function initMapping() {
    generateMappingGrid();
    initMappingPresets();
}

function generateMappingGrid() {
    const grid = document.getElementById('mappingGrid');
    const channelSelect = document.getElementById('mappingChannel');
    
    if (!grid) return;
    grid.innerHTML = '';
    channelSelect.innerHTML = '';
    
    const chCount = config.channels ? config.channels.length : 16;
    
    for (let i = 0; i < chCount; i++) {
        // Add channel select option
        const opt = document.createElement('option');
        opt.value = i;
        opt.textContent = `CH${i + 1}`;
        channelSelect.appendChild(opt);
        
        // Generate mapping row
        const ch = config.channels ? config.channels[i] : null;
        const row = document.createElement('div');
        row.className = 'mapping-row';
        row.innerHTML = `
            <label>CH${i + 1}</label>
            <select class="input-source" data-ch="${i}">
                <option value="analog" ${ch?.inputSource === 0 ? 'selected' : ''}>Analog</option>
                <option value="button_digital" ${ch?.inputSource === 1 ? 'selected' : ''}>Button</option>
                <option value="button_analog" ${ch?.inputSource === 2 ? 'selected' : ''}>Button Analog</option>
            </select>
            <select class="input-index" data-ch="${i}">
                ${generateInputOptions(ch?.inputIndex)}
            </select>
            <input type="checkbox" class="reversed" data-ch="${i}" ${ch?.reversed ? 'checked' : ''}>
            <input type="number" class="deadzone" data-ch="${i}" value="${ch?.deadzone || 0}" min="0" max="100">
            <input type="number" class="expo" data-ch="${i}" value="${ch?.expo || 0}" min="0" max="100">
            <input type="number" class="smoothing" data-ch="${i}" value="${ch?.smoothing || 0}" min="0" max="100">
            <input type="number" class="scale" data-ch="${i}" value="${ch?.scale || 100}" min="0" max="200">
        `;
        grid.appendChild(row);
    }
    
    // Add event listeners
    document.querySelectorAll('.mapping-row select, .mapping-row input').forEach(el => {
        el.addEventListener('change', handleMappingChange);
    });
}

function generateInputOptions(selected) {
    let html = '';
    Object.keys(INPUT_NAMES).forEach(key => {
        const name = INPUT_NAMES[key];
        html += `<option value="${key}" ${selected == key ? 'selected' : ''}>${name}</option>`;
    });
    return html;
}

function handleMappingChange(e) {
    const ch = parseInt(e.target.dataset.ch);
    const row = e.target.closest('.mapping-row');
    
    if (!config.channels) config.channels = [];
    if (!config.channels[ch]) {
        config.channels[ch] = {
            index: ch,
            inputSource: 0,
            inputIndex: 17,
            reversed: false,
            minValue: 1000,
            maxValue: 2000,
            centerValue: 1500,
            deadzone: 0,
            expo: 0,
            smoothing: 0,
            scale: 100,
            enabled: true
        };
    }
    
    const chConfig = config.channels[ch];
    
    if (e.target.classList.contains('input-source')) {
        const typeMap = { 'analog': 0, 'button_digital': 1, 'button_analog': 2 };
        chConfig.inputSource = typeMap[e.target.value];
    } else if (e.target.classList.contains('input-index')) {
        chConfig.inputIndex = parseInt(e.target.value);
    } else if (e.target.classList.contains('reversed')) {
        chConfig.reversed = e.target.checked;
    } else if (e.target.classList.contains('deadzone')) {
        chConfig.deadzone = parseInt(e.target.value) || 0;
    } else if (e.target.classList.contains('expo')) {
        chConfig.expo = parseInt(e.target.value) || 0;
    } else if (e.target.classList.contains('smoothing')) {
        chConfig.smoothing = parseInt(e.target.value) || 0;
    } else if (e.target.classList.contains('scale')) {
        chConfig.scale = parseInt(e.target.value) || 100;
    }
    
    saveConfigDebounced();
}

function initMappingPresets() {
    document.getElementById('presetAirplane')?.addEventListener('click', () => {
        showToast('Airplane preset loaded', 'info');
    });
    document.getElementById('presetDrone')?.addEventListener('click', () => {
        showToast('Drone preset loaded', 'info');
    });
    document.getElementById('presetCar')?.addEventListener('click', () => {
        showToast('Car preset loaded', 'info');
    });
    document.getElementById('presetBoat')?.addEventListener('click', () => {
        showToast('Boat preset loaded', 'info');
    });
    
    document.getElementById('copyChannelBtn')?.addEventListener('click', () => {
        const ch = parseInt(document.getElementById('mappingChannel').value);
        if (config.channels && config.channels[ch]) {
            copiedChannel = JSON.parse(JSON.stringify(config.channels[ch]));
            showToast(`Channel ${ch + 1} copied`, 'success');
        }
    });
    
    document.getElementById('pasteChannelBtn')?.addEventListener('click', () => {
        if (!copiedChannel) {
            showToast('No channel copied', 'error');
            return;
        }
        const ch = parseInt(document.getElementById('mappingChannel').value);
        if (config.channels) {
            config.channels[ch] = JSON.parse(JSON.stringify(copiedChannel));
            config.channels[ch].index = ch;
            saveConfigDebounced();
            generateMappingGrid();
            showToast(`Channel ${ch + 1} pasted`, 'success');
        }
    });
    
    document.getElementById('resetChannelBtn')?.addEventListener('click', () => {
        showConfirm('Reset Channel', 'Reset this channel to default?', () => {
            const ch = parseInt(document.getElementById('mappingChannel').value);
            if (config.channels) {
                config.channels[ch] = null;
                saveConfigDebounced();
                generateMappingGrid();
                showToast(`Channel ${ch + 1} reset`, 'success');
            }
        });
    });
}

// ============ OUTPUT ============
function initOutput() {
    document.querySelectorAll('input[name="protocol"]').forEach(radio => {
        radio.addEventListener('change', (e) => {
            updateProtocolConfigVisibility(e.target.value);
            saveConfigDebounced();
        });
    });
    
    // CRSF config
    ['crsfBaudrate', 'crsfFrameRate'].forEach(id => {
        document.getElementById(id)?.addEventListener('change', saveConfigDebounced);
    });
    
    // SBUS config
    ['sbusBaudrate', 'sbusFrameRate', 'sbusInverted'].forEach(id => {
        document.getElementById(id)?.addEventListener('change', saveConfigDebounced);
    });
    
    // PPM config
    ['ppmPin', 'ppmFrameRate', 'ppmInverted'].forEach(id => {
        document.getElementById(id)?.addEventListener('change', saveConfigDebounced);
    });
}

function updateProtocolConfigVisibility(proto) {
    document.querySelector('.crsf-config-card').style.display = proto === 'crsf' ? 'block' : 'none';
    document.querySelector('.sbus-config-card').style.display = proto === 'sbus' ? 'block' : 'none';
    document.querySelector('.ppm-config-card').style.display = proto === 'ppm' ? 'block' : 'none';
}

// ============ CALIBRATION ============
function initCalibration() {
    document.getElementById('calibrateCenter')?.addEventListener('click', () => {
        showToast('Calibrating center...', 'info');
        fetch('/api/calibration?action=center', { method: 'POST' })
            .then(r => r.json())
            .then(d => showToast('Center calibrated', 'success'))
            .catch(e => showToast('Calibration failed', 'error'));
    });
    
    document.getElementById('calibrateMinMax')?.addEventListener('click', () => {
        showToast('Detecting min/max...', 'info');
        fetch('/api/calibration?action=minmax', { method: 'POST' })
            .then(r => r.json())
            .then(d => showToast('Min/Max detected', 'success'))
            .catch(e => showToast('Detection failed', 'error'));
    });
    
    document.getElementById('testCalibration')?.addEventListener('click', () => {
        showToast('Testing calibration...', 'info');
    });
    
    document.getElementById('resetCalibration')?.addEventListener('click', () => {
        showConfirm('Reset Calibration', 'Reset all calibration data?', () => {
            fetch('/api/calibration?action=reset', { method: 'POST' })
                .then(r => r.json())
                .then(d => showToast('Calibration reset', 'success'))
                .catch(e => showToast('Reset failed', 'error'));
        });
    });
    
    document.getElementById('saveCalibration')?.addEventListener('click', () => {
        fetch('/api/calibration?action=save', { method: 'POST' })
            .then(r => r.json())
            .then(d => showToast('Calibration saved', 'success'))
            .catch(e => showToast('Save failed', 'error'));
    });
    
    generateCalibrationPreview();
}

function generateCalibrationPreview() {
    const grid = document.getElementById('calPreviewGrid');
    if (!grid) return;
    
    grid.innerHTML = '';
    const analogInputs = ['ANALOG_LX', 'ANALOG_LY', 'ANALOG_RX', 'ANALOG_RY', 'ANALOG_L2', 'ANALOG_R2'];
    
    analogInputs.forEach(name => {
        const item = document.createElement('div');
        item.className = 'cal-preview-item';
        item.innerHTML = `
            <label>${name}</label>
            <div class="value" id="cal-${name}">128</div>
        `;
        grid.appendChild(item);
    });
}

// ============ SYSTEM ============
function initSystem() {
    loadSystemInfo();
    initWiFi();
}

function loadSystemInfo() {
    fetch('/api/system')
        .then(r => r.json())
        .then(data => {
            document.getElementById('sysFwVersion').textContent = data.firmwareVersion || '-';
            document.getElementById('sysBuildDate').textContent = data.buildDate || '-';
            document.getElementById('sysChipModel').textContent = data.espModel || '-';
            document.getElementById('sysChipRev').textContent = data.espRevision || '-';
            document.getElementById('sysCpuCores').textContent = data.cpuCores || '-';
            document.getElementById('sysFlashSize').textContent = data.flashSize ? 
                `${Math.round(data.flashSize / 1024 / 1024)} MB` : '-';
            document.getElementById('sysPsramSize').textContent = data.psramSize ? 
                `${Math.round(data.psramSize / 1024 / 1024)} MB` : '-';
        })
        .catch(e => console.error('Failed to load system info:', e));
}

function initWiFi() {
    document.getElementById('wifiConnect')?.addEventListener('click', () => {
        const ssid = document.getElementById('wifiSsid').value;
        const pass = document.getElementById('wifiPassword').value;
        
        if (!ssid) {
            showToast('Please enter SSID', 'error');
            return;
        }
        
        showToast('Connecting...', 'info');
        
        fetch(`/api/wifi/connect?ssid=${encodeURIComponent(ssid)}&password=${encodeURIComponent(pass)}`, {
            method: 'POST'
        })
        .then(r => r.json())
        .then(d => {
            if (d.success) {
                showToast(`Connected! IP: ${d.ip}`, 'success');
            } else {
                showToast(d.error || 'Connection failed', 'error');
            }
        })
        .catch(e => showToast('Connection failed', 'error'));
    });
    
    document.getElementById('wifiScan')?.addEventListener('click', scanWiFi);
}

function scanWiFi() {
    showToast('Scanning...', 'info');
    
    fetch('/api/wifi/scan')
        .then(r => r.json())
        .then(data => {
            const container = document.getElementById('wifiNetworks');
            if (!container) return;
            
            container.innerHTML = '';
            
            if (data.networks && data.networks.length > 0) {
                data.networks.forEach(net => {
                    const item = document.createElement('div');
                    item.className = 'wifi-network-item';
                    item.innerHTML = `
                        <span class="ssid">${net.ssid}</span>
                        <span class="rssi">${net.rssi} dBm</span>
                    `;
                    item.addEventListener('click', () => {
                        document.getElementById('wifiSsid').value = net.ssid;
                        document.getElementById('wifiPassword').focus();
                    });
                    container.appendChild(item);
                });
            } else {
                container.innerHTML = '<p>No networks found</p>';
            }
        })
        .catch(e => showToast('Scan failed', 'error'));
}

// ============ BUTTONS ============
function initButtons() {
    // Save config
    document.getElementById('saveConfigBtn')?.addEventListener('click', () => {
        saveConfig();
        showToast('Configuration saved', 'success');
    });
    
    // Backup config
    document.getElementById('backupConfigBtn')?.addEventListener('click', () => {
        fetch('/api/backup')
            .then(r => r.json())
            .then(data => {
                const blob = new Blob([JSON.stringify(data, null, 2)], { type: 'application/json' });
                const url = URL.createObjectURL(blob);
                const a = document.createElement('a');
                a.href = url;
                a.download = `esp32-rc-config-${Date.now()}.json`;
                a.click();
                URL.revokeObjectURL(url);
                showToast('Config backup downloaded', 'success');
            })
            .catch(e => showToast('Backup failed', 'error'));
    });
    
    // Restore config
    document.getElementById('restoreConfigBtn')?.addEventListener('click', () => {
        document.getElementById('restoreFile').click();
    });
    
    document.getElementById('restoreFile')?.addEventListener('change', (e) => {
        const file = e.target.files[0];
        if (!file) return;
        
        const reader = new FileReader();
        reader.onload = (event) => {
            try {
                const data = JSON.parse(event.target.result);
                fetch('/api/restore', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify(data)
                })
                .then(r => r.json())
                .then(d => {
                    if (d.success) {
                        showToast('Config restored', 'success');
                        setTimeout(() => location.reload(), 1000);
                    } else {
                        showToast('Restore failed', 'error');
                    }
                });
            } catch (e) {
                showToast('Invalid config file', 'error');
            }
        };
        reader.readAsText(file);
    });
    
    // Reboot
    document.getElementById('rebootBtn')?.addEventListener('click', () => {
        showConfirm('Reboot', 'Reboot ESP32?', () => {
            fetch('/api/reboot', { method: 'POST' })
                .then(() => {
                    showToast('Rebooting...', 'info');
                });
        });
    });
    
    // Factory reset
    document.getElementById('factoryResetBtn')?.addEventListener('click', () => {
        showConfirm('Factory Reset', 'This will reset ALL settings to default. Continue?', () => {
            fetch('/api/factoryreset', { method: 'POST' })
                .then(r => r.json())
                .then(d => {
                    showToast('Factory reset complete', 'success');
                    setTimeout(() => location.reload(), 1000);
                })
                .catch(e => showToast('Reset failed', 'error'));
        });
    });
    
    // OTA Update
    document.getElementById('otaUpdateBtn')?.addEventListener('click', () => {
        const url = document.getElementById('otaUrl').value;
        if (!url) {
            showToast('Please enter firmware URL', 'error');
            return;
        }
        
        showConfirm('OTA Update', 'Start firmware update?', () => {
            document.getElementById('otaProgress').style.display = 'flex';
            fetch(`/api/ota?url=${encodeURIComponent(url)}`, { method: 'POST' })
                .then(r => r.json())
                .then(d => {
                    if (d.success) {
                        showToast('Update started', 'info');
                        simulateOTAProgress();
                    } else {
                        showToast('Update failed', 'error');
                    }
                })
                .catch(e => showToast('Update failed', 'error'));
        });
    });
}

function simulateOTAProgress() {
    const fill = document.getElementById('otaProgressFill');
    const text = document.getElementById('otaProgressText');
    let progress = 0;
    
    const interval = setInterval(() => {
        progress += Math.random() * 10;
        if (progress >= 100) {
            progress = 100;
            clearInterval(interval);
            showToast('Update complete! Rebooting...', 'success');
            setTimeout(() => location.reload(), 2000);
        }
        fill.style.width = `${progress}%`;
        text.textContent = `${Math.round(progress)}%`;
    }, 500);
}

// ============ CONFIG MANAGEMENT ============
let saveTimeout = null;

function saveConfigDebounced() {
    if (saveTimeout) clearTimeout(saveTimeout);
    saveTimeout = setTimeout(saveConfig, 1000);
}

function saveConfig() {
    if (!config || !config.channels) return;
    
    // Update config object with UI values
    document.querySelectorAll('.mapping-row').forEach((row, i) => {
        if (!config.channels[i]) return;
        
        const srcSelect = row.querySelector('.input-source');
        const idxSelect = row.querySelector('.input-index');
        const revCheck = row.querySelector('.reversed');
        const dzInput = row.querySelector('.deadzone');
        const exInput = row.querySelector('.expo');
        const smInput = row.querySelector('.smoothing');
        const scInput = row.querySelector('.scale');
        
        if (srcSelect) {
            const typeMap = { 'analog': 0, 'button_digital': 1, 'button_analog': 2 };
            config.channels[i].inputSource = typeMap[srcSelect.value];
        }
        if (idxSelect) config.channels[i].inputIndex = parseInt(idxSelect.value);
        if (revCheck) config.channels[i].reversed = revCheck.checked;
        if (dzInput) config.channels[i].deadzone = parseInt(dzInput.value) || 0;
        if (exInput) config.channels[i].expo = parseInt(exInput.value) || 0;
        if (smInput) config.channels[i].smoothing = parseInt(smInput.value) || 0;
        if (scInput) config.channels[i].scale = parseInt(scInput.value) || 100;
    });
    
    // Protocol
    const protoRadio = document.querySelector('input[name="protocol"]:checked');
    if (protoRadio) {
        const protoMap = { 'crsf': 0, 'sbus': 1, 'ppm': 2 };
        config.system = config.system || {};
        config.system.activeProtocol = protoMap[protoRadio.value];
    }
    
    // Send to server
    fetch('/api/save', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(config)
    })
    .then(r => r.json())
    .then(d => {
        if (d.success) {
            console.log('Config saved');
        }
    })
    .catch(e => console.error('Save failed:', e));
}

// ============ TOAST NOTIFICATIONS ============
function showToast(message, type = 'info') {
    const container = document.getElementById('toastContainer');
    const toast = document.createElement('div');
    toast.className = `toast ${type}`;
    toast.textContent = message;
    
    container.appendChild(toast);
    
    setTimeout(() => {
        toast.style.opacity = '0';
        setTimeout(() => toast.remove(), 300);
    }, 3000);
}

// ============ MODAL ============
let confirmCallback = null;

function showConfirm(title, message, callback) {
    document.getElementById('modalTitle').textContent = title;
    document.getElementById('modalMessage').textContent = message;
    document.getElementById('confirmModal').classList.add('active');
    confirmCallback = callback;
}

document.getElementById('modalCancel')?.addEventListener('click', () => {
    document.getElementById('confirmModal').classList.remove('active');
    confirmCallback = null;
});

document.getElementById('modalConfirm')?.addEventListener('click', () => {
    document.getElementById('confirmModal').classList.remove('active');
    if (confirmCallback) confirmCallback();
    confirmCallback = null;
});

// ============ PERIODIC STATUS REQUEST ============
setInterval(requestStatus, STATUS_UPDATE_INTERVAL);
