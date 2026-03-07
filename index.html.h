#pragma once
#include <string>

// The HTML payload sent to the user's mobile browser when connecting to localhost:8080

const std::string WEB_CLIENT_HTML = std::string(R"=====(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
    <title>PHONIC ░ BRIDGE</title>
    <style>
        @import url('https://fonts.googleapis.com/css2?family=Share+Tech+Mono&display=swap');

        :root {
            --glitch-color: #ff3333;
            --glitch-glow: #ff0000;
            --bg-color: #080808;
            --text-color: #f0f0f0;
        }

        body {
            background-color: var(--bg-color);
            color: var(--text-color);
            font-family: 'Share Tech Mono', monospace;
            margin: 0;
            padding: 20px;
            display: flex;
            flex-direction: column;
            align-items: center;
            justify-content: flex-start;
            min-height: 100vh;
            overflow-x: hidden;
            overflow-y: auto;
            text-transform: uppercase;
        }

        /* Deep CRT Scanline & Vignette Effect */
        .crt-overlay {
            position: fixed;
            top: 0; left: 0; width: 100vw; height: 100vh;
            background: linear-gradient(rgba(18, 16, 16, 0) 50%, rgba(0, 0, 0, 0.25) 50%), linear-gradient(90deg, rgba(255, 0, 0, 0.06), rgba(0, 255, 0, 0.02), rgba(0, 0, 255, 0.06));
            background-size: 100% 4px, 6px 100%;
            pointer-events: none;
            z-index: 999;
            box-shadow: inset 0 0 100px rgba(0,0,0,0.9);
        }

        .crt-flicker {
            animation: crt-flicker 0.15s infinite;
        }

        @keyframes crt-flicker {
            0% { opacity: 0.95; }
            50% { opacity: 1; }
            100% { opacity: 0.98; }
        }

        .ascii-container {
            position: relative;
            margin-top: 20px;
            margin-bottom: 5px;
            width: 100%;
            height: 120px;
        }

        .ascii-art {
            position: absolute;
            top: 0; left: 50%;
            transform: translateX(-50%);
            white-space: pre;
            font-size: 0.65rem;
            line-height: 1.1;
            color: #00e5ff;
            text-shadow: 0 0 5px #00e5ff;
            pointer-events: none;
            margin-bottom: 25px;
        }

        .layer2 {
            color: rgba(255, 0, 0, 0.7);
            text-shadow: 0 0 5px red;
            z-index: -1;
            left: calc(50% - 2px);
            opacity: 0;
            transition: opacity 0.05s;
        }

        .layer3 {
            color: rgba(0, 0, 255, 0.7);
            text-shadow: 0 0 5px blue;
            z-index: -2;
            left: calc(50% + 2px);
            opacity: 0;
            transition: opacity 0.05s;
        }

        .separator {
            width: 100%;
            max-width: 600px;
            text-align: center;
            color: #555;
            letter-spacing: 2px;
        }
        body::-webkit-scrollbar { display: none; } /* Hide Scrollbar on general body */
        ::-webkit-scrollbar { display: none; } /* Hide Desktop Scrollbar globally */

        .container {
            width: 100%;
            max-width: 500px;
            display: flex;
            flex-direction: column;
            align-items: center;
            z-index: 10;
        }

        .terminal-box {
            border: 1px solid var(--text-color);
            padding: 15px 20px;
            width: 90%;
            margin-bottom: 30px;
            background: rgba(255,255,255,0.02);
            position: relative;
        }

        .terminal-box::before, .terminal-box::after {
            content: ''; position: absolute; width: 10px; height: 10px; border: 1px solid var(--text-color);
        }
        .terminal-box::before { top: -4px; left: -4px; border-right: none; border-bottom: none; }
        .terminal-box::after { bottom: -4px; right: -4px; border-left: none; border-top: none; }

        .line {
            display: flex;
            justify-content: space-between;
            margin-bottom: 10px;
            font-size: 0.85rem;
            color: #aaa;
        }

        .line span.highlight { color: #fff; text-shadow: 0 0 3px #fff; }

        #dynamicLogs {
            max-height: 144px;
            overflow-y: auto;
            overflow-x: hidden;
            padding-right: 5px;
        }

        #dynamicLogs::-webkit-scrollbar { width: 5px; }
        #dynamicLogs::-webkit-scrollbar-track { background: rgba(255,255,255,0.05); }
        #dynamicLogs::-webkit-scrollbar-thumb { background: #555; }

        .status-text {
            font-size: 1.1rem;
            text-align: center;
            display: block;
            margin: 20px 0;
            color: var(--glitch-color);
            animation: blink 1.5s infinite;
            text-shadow: 0 0 5px var(--glitch-glow);
        }

        .btn {
            background: transparent;
            color: #00e5ff;
            border: 1px dashed #00e5ff;
            padding: 15px 40px;
            font-family: inherit;
            font-size: 1.2rem;
            cursor: pointer;
            transition: all 0.2s;
            text-shadow: 0 0 5px #00e5ff;
            box-shadow: inset 0 0 10px rgba(0, 229, 255, 0.1);
            width: 100%;
            letter-spacing: 2px;
        }

        .btn:hover, .btn:active {
            background: #00e5ff;
            color: #000;
            box-shadow: 0 0 20px #00e5ff, inset 0 0 20px #00e5ff;
            animation: none;
        }

        .volume-container {
            margin-top: 30px;
            width: 90%;
            text-align: center;
        }

        .vol-label {
            color: #555;
            font-size: 0.8rem;
            margin-bottom: 15px;
            display: block;
        }

        input[type=range] {
            -webkit-appearance: none;
            width: 100%;
            background: transparent;
        }

        input[type=range]::-webkit-slider-thumb {
            -webkit-appearance: none;
            height: 25px;
            width: 15px;
            background: #fff;
            cursor: pointer;
            margin-top: -10px;
            box-shadow: 0 0 8px #fff;
            border-radius: 0;
        }

        input[type=range]::-webkit-slider-runnable-track {
            width: 100%;
            height: 2px;
            cursor: pointer;
            background: #444;
            background-image: repeating-linear-gradient(90deg, #444, #444 2px, transparent 2px, transparent 6px);
        }

        @keyframes blink { 0%, 100% { opacity: 1; } 50% { opacity: 0.3; } }

        /* Success States */
        .connected .status-text { color: #00ff00; text-shadow: 0 0 5px #00ff00; animation: none; }
        .connected .btn { border-color: #ff3333; color: #ff3333; text-shadow: 0 0 5px #ff3333; box-shadow: inset 0 0 10px rgba(255,51,51,0.1); }
        .connected .btn:hover { background: #ff3333; color: #000; }
        
        @media screen and (max-height: 500px) {
            .ascii-art { font-size: 0.5rem; }
            .ascii-container { height: 45px; }
            .terminal-box { margin-bottom: 10px; }
        }

        .desktop-text { display: inline; }
        .mobile-text { display: none; }

        @media screen and (max-width: 600px) {
            .desktop-text { display: none; }
            .mobile-text { display: inline; }
            .separator {
                font-size: 0.70rem;
                letter-spacing: 1px;
                white-space: nowrap;
                margin-left: -40px;
            }
            .footer {
                margin-bottom: 5px;
                transform: translateY(20px);
            }
        }

        .footer {
            margin-top: auto;
            margin-bottom: 25px;
            font-size: 0.75rem;
            color: #444;
            letter-spacing: 2px;
            text-shadow: 0 0 2px rgba(255, 255, 255, 0.1);
            z-index: 10;
        }
    </style>
</head>
)=====") + std::string(R"=====(
<body class="crt-flicker">
    <div class="crt-overlay"></div>
    
    <div class="ascii-container">
        <div class="ascii-art layer1">
     ____  __  ______  _   ____________
    / __ \/ / / / __ \/ | / /  _/ ____/
   / /_/ / /_/ / / / /  |/ // // /   
  / ____/ __  / /_/ / /|  // // /___ 
 /_/   /_/ /_/\____/_/ |_/___/\____/ 
     ____  ____  ________  ____________  
    / __ )/ __ \/  _/ __ \/ ____/ ____/  
   / __  / /_/ // // / / / / __/ __/     
  / /_/ / _, _// // /_/ / /_/ / /___     
 /_____/_/ |_/___/_____/\____/_____/     
        </div>
        <div class="ascii-art layer2">
     ____  __  ______  _   ____________
    / __ \/ / / / __ \/ | / /  _/ ____/
   / /_/ / /_/ / / / /  |/ // // /   
  / ____/ __  / /_/ / /|  // // /___ 
 /_/   /_/ /_/\____/_/ |_/___/\____/ 
     ____  ____  ________  ____________  
    / __ )/ __ \/  _/ __ \/ ____/ ____/  
   / __  / /_/ // // / / / / __/ __/     
  / /_/ / _, _// // /_/ / /_/ / /___     
 /_____/_/ |_/___/_____/\____/_____/     
        </div>
        <div class="ascii-art layer3">
     ____  __  ______  _   ____________
    / __ \/ / / / __ \/ | / /  _/ ____/
   / /_/ / /_/ / / / /  |/ // // /   
  / ____/ __  / /_/ / /|  // // /___ 
 /_/   /_/ /_/\____/_/ |_/___/\____/ 
     ____  ____  ________  ____________  
    / __ )/ __ \/  _/ __ \/ ____/ ____/  
   / __  / /_/ // // / / / / __/ __/     
  / /_/ / _, _// // /_/ / /_/ / /___     
 /_____/_/ |_/___/_____/\____/_____/     
        </div>
    </div>
    <div class="separator">
        .:[PHONIC]<span class="desktop-text">...........................</span><span class="mobile-text">...........................</span>B.R.I.D.G.E.:.
        <br><br>
    </div>

    <div class="container">
        <div class="terminal-box">
            <div id="dynamicLogs">
                <div class="line"><span>ACCESS LEVEL</span> <span class="highlight">GUEST</span></div>
                <div class="line"><span>PROTOCOL</span> <span>UDP / WebAudio</span></div>
                <div class="line"><span>STREAM STATUS</span> <span>MONITORING...</span></div>
            </div>
            
            <div class="separator" style="margin: 15px 0 5px 0;">........................</div>
            
            <span id="statusText" class="status-text">AWAITING CONNECTION</span>
        </div>

        <button id="connectBtn" class="btn">[ CONNECT_AUDIO ]</button>

        <div class="volume-container">
            <span class="vol-label">... [x] S.Y.S.T.E.M..V.O.L.U.M.E [x] ...</span>
            <input type="range" id="volume" min="0" max="1.5" step="0.01" value="1">
            
            <div style="margin-top:25px; font-size: 0.8rem; letter-spacing: 1px;">
                <span id="bufferLabel" style="color: #00e5ff; font-size: 0.8rem;">BUFFER: 1.5 sec (ANTI-STUTTER)</span>
                <input type="range" id="bufferMode" min="0" max="5.0" step="0.5" value="1.5" style="margin-top: 10px; accent-color: #00e5ff;">
            </div>
        </div>
    </div>

    <div class="footer">.:[x] MADE BY XXLRN [x]:.</div>

    <script>
        const bodyEl = document.body;
        const statusEl = document.getElementById('statusText');
        const btn = document.getElementById('connectBtn');
        const volSlider = document.getElementById('volume');

        // Restore saved volume and buffer from localStorage (device-local persistence)
        const savedVol = localStorage.getItem('pb_volume');
        if (savedVol !== null) volSlider.value = savedVol;
        const savedBuf = localStorage.getItem('pb_buffer');
        if (savedBuf !== null) document.getElementById('bufferMode').value = savedBuf;
        
        let audioCtx;
        let ws;
        let nextPlayTime = 0;
        let isConnecting = false;
        let isConnected = false;
        let screenLock = null;

        const requestWakeLock = async () => {
            try { if ('wakeLock' in navigator) { screenLock = await navigator.wakeLock.request('screen'); } } catch (err) {}
        };
        const releaseWakeLock = async () => {
            if (screenLock !== null) { await screenLock.release(); screenLock = null; }
        };

        function addLogLine(severity, msg, color) {
            const container = document.getElementById('dynamicLogs');
            const newLine = document.createElement('div');
            newLine.className = 'line';
            newLine.innerHTML = `<span style="color:${color}">${severity}</span> <span style="color:${color}">${msg}</span>`;
            container.appendChild(newLine);
            while (container.children.length > 50) {
                container.removeChild(container.firstChild);
            }
            container.scrollTop = container.scrollHeight;
        }

        function lockAndPollServer() {
            btn.disabled = true;
            btn.style.opacity = '0.5';
            btn.style.cursor = 'not-allowed';
            btn.innerText = "[ ACCESS RESTRICTED ]";
            
            let poll = setInterval(() => {
                fetch('/api/status').then(res => {
                    if (res.ok) {
                        clearInterval(poll);
                        btn.disabled = false;
                        btn.style.opacity = '1';
                        btn.style.cursor = 'pointer';
                        btn.innerText = "[ RE-CONNECT ]";
                    }
                }).catch(e => {});
            }, 3000);
        }

        let keepAliveOsc = null; // Background mobile CPU keep-alive oscillator
        let silentAudioEl = null; // Chrome OS MediaSession background heartbeat

        function disconnectStream(lockButton = false) {
            if (window.pingInterval) clearInterval(window.pingInterval);
            
            releaseWakeLock();

            if (ws) {
                ws.onclose = null; // Prevent trigger
                ws.close();
                ws = null;
            }
            if (keepAliveOsc) {
                try { keepAliveOsc.stop(); keepAliveOsc.disconnect(); } catch(e){}
                keepAliveOsc = null;
            }
            if (silentAudioEl) {
                silentAudioEl.pause();
            }
            isConnected = false;
            isConnecting = false;
            bodyEl.classList.remove('connected');
            statusEl.innerText = "STREAM SEVERED";
            statusEl.style.color = '';
            
            if (lockButton) {
                lockAndPollServer();
            } else {
                btn.innerText = "[ RE-CONNECT ]";
            }
        }

        function initAudioStream() {
            if (isConnected) {
                disconnectStream();
                return;
            }
            if (isConnecting) return;
            isConnecting = true;

            statusEl.innerText = "HANDSHAKING...";
            statusEl.style.color = '';
            
            if (!audioCtx) {
                try {
                    audioCtx = new (window.AudioContext || window.webkitAudioContext)({
                        sampleRate: 48000
                    });
                } catch (e) {
                    statusEl.innerText = "[ FATAL: WEB_AUDIO_UNSUPPORTED ]";
                    isConnecting = false;
                    return;
                }
            } else if (audioCtx.state === 'suspended') {
                audioCtx.resume();
            }

            // Sync Context Lock 1: MediaSession Registry
            if ('mediaSession' in navigator) {
                navigator.mediaSession.metadata = new MediaMetadata({
                    title: 'PHONIC BRIDGE',
                    artist: 'Live Audio Stream',
                    album: 'Connected',
                    artwork: [
                        { src: 'data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAQAAAC1HAwCAAAAC0lEQVR42mNkYAAAAAYAAjCB0C8AAAAASUVORK5CYII=', sizes: '1x1', type: 'image/png' }
                    ]
                });
                navigator.mediaSession.setActionHandler('play', function() { /* Dummy */ });
                navigator.mediaSession.setActionHandler('pause', function() { /* Dummy */ });
            }

            // Sync Context Lock 2: Physical DOM Audio Hierarchy (Required for MediaSession wake lock on Chrome 120+)
            if (!silentAudioEl) {
                silentAudioEl = new Audio('data:audio/mp3;base64,//OExAAAAANIAAAAAExBTUUzLjEwMKqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq');
                silentAudioEl.loop = true;
                silentAudioEl.volume = 0.01;
                silentAudioEl.style.display = 'none';
                document.body.appendChild(silentAudioEl);
            }
            silentAudioEl.play().catch(e => console.log(e));

            // Sync Context Lock 3: Web Audio API Oscillator
            if (audioCtx) {
                keepAliveOsc = audioCtx.createOscillator();
                const gainNode = audioCtx.createGain();
                gainNode.gain.value = 0.0001; 
                keepAliveOsc.connect(gainNode);
                gainNode.connect(audioCtx.destination);
                keepAliveOsc.start();
            }

            requestWakeLock();

            const protocol = window.location.protocol === 'https:' ? 'wss:' : 'ws:';
            const wsUrl = protocol + '//' + window.location.host + '/stream';
            ws = new WebSocket(wsUrl);
            ws.binaryType = 'arraybuffer';

            ws.onopen = () => {
                isConnected = true;
                isConnecting = false;
                bodyEl.classList.add('connected');
                statusEl.innerText = "[ STREAM ACTIVE ]";
                statusEl.style.color = '';
                btn.innerText = "[ DISCONNECT ]";
                
                // Mongoose Time-Out Heartbeat (Fixes random Signal Lost drops)
                window.pingInterval = setInterval(() => {
                    if (ws && ws.readyState === WebSocket.OPEN) {
                        ws.send("PING");
                    }
                }, 3000);
            };

            ws.onclose = () => {
)=====") + std::string(R"=====(
                isConnected = false;
                isConnecting = false;
                bodyEl.classList.remove('connected');
                statusEl.innerText = "[ WARN: SIGNAL LOST - RECONNECTING ]";
                statusEl.style.color = '#ffff00';
                ws = null;
                if (window.masterGain) window.masterGain = null;
                
                // Infinite Auto-Reconnect on organic network drops natively
                setTimeout(() => {
                    initAudioStream();
                }, 1500);
            };

            ws.onerror = (e) => {
                console.error("WebSocket Error: ", e);
            };

            ws.onmessage = (event) => {
                if (typeof event.data === 'string') {
                    if (event.data.startsWith('MSG:')) {
                        const msg = event.data.substring(4);
                        if (msg === 'KICKED') {
                            addLogLine('WARN', 'KICKED BY HOST', '#ff3333');
                            statusEl.innerText = 'CONNECTION SEVERED (KICKED)';
                            statusEl.style.color = '#ff3333';
                            disconnectStream(true);
                        } else if (msg === 'BANNED') {
                            addLogLine('FATAL', 'BANNED BY HOST', '#ff3333');
                            statusEl.innerText = 'ACCESS DENIED (BANNED)';
                            statusEl.style.color = '#ff3333';
                            disconnectStream(true);
                        } else if (msg === 'REVOKED') {
                            addLogLine('FATAL', 'ACCESS REVOKED', '#ff3333');
                            statusEl.innerText = 'ACCESS DENIED (REVOKED)';
                            statusEl.style.color = '#ff3333';
                            disconnectStream(true);
                        } else if (msg === 'CONNECTION_LOST') {
                            addLogLine('WARN', 'WAITING FOR HOST...', '#ffff00');
                            statusEl.innerText = 'DISCONNECTED PLEASE WAIT...';
                            statusEl.style.color = '#ffff00';
                            bodyEl.classList.remove('connected');
                            btn.innerText = '[ CANCEL_RETRY ]';
                        } else if (msg === 'RECONNECTED') {
                            addLogLine('STATUS', 'SIGNAL RE-ESTABLISHED', '#00ff00');
                            statusEl.innerText = '[ STREAM ACTIVE ]';
                            statusEl.style.color = '';
                            bodyEl.classList.add('connected');
                            btn.innerText = '[ DISCONNECT ]';
                        } else if (msg === 'MUTED') {
                            addLogLine('WARN', 'YOU ARE MUTED BY THE HOST', '#ffff00');
                            statusEl.innerText = 'MUTED BY HOST... PLEASE WAIT';
                            statusEl.style.color = '#ffff00';
                            bodyEl.classList.remove('connected');
                        } else if (msg === 'UNMUTED') {
                            addLogLine('STATUS', 'YOU ARE NOT MUTED ANYMORE', '#00ff00');
                            statusEl.innerText = '[ STREAM ACTIVE ]';
                            statusEl.style.color = '';
                            bodyEl.classList.add('connected');
                        } else if (msg === 'UNMUTED_SILENT') {
                            statusEl.innerText = '[ STREAM ACTIVE ]';
                            statusEl.style.color = '';
                            bodyEl.classList.add('connected');
                        }
                    }
                    return;
                }

                if (!audioCtx) return;
                if (audioCtx.state === 'suspended') audioCtx.resume();

                if (event.data.byteLength < 4) return;
                
                let data = new Float32Array(event.data);
                let frames = data.length / 2;
                let buffer = audioCtx.createBuffer(2, frames, 48000);
                let leftObj = buffer.getChannelData(0);
                let rightObj = buffer.getChannelData(1);
                
                for (let i = 0; i < frames; i++) {
                    leftObj[i] = data[i * 2];
                    rightObj[i] = data[i * 2 + 1];
                }

                // Extrapolate Adaptive Buffer Window
                const sliderVal = parseFloat(document.getElementById('bufferMode').value);
                
                const maxDelay = sliderVal === 0 ? 1.0 : sliderVal + 1.0;
                const minDelay = sliderVal === 0 ? 0.08 : sliderVal;
                
                const source = audioCtx.createBufferSource();
                source.buffer = buffer;
                
                if (!window.masterGain) {
                    window.masterGain = audioCtx.createGain();
                    window.masterGain.gain.value = parseFloat(volSlider.value);
                    window.masterGain.connect(audioCtx.destination); // Direct to OS hardware
                }
                source.connect(window.masterGain);
                
                const currentTime = audioCtx.currentTime;
                // If we fall behind severely or jump too far dynamically, snap back to present to avoid glitch loops
                if (nextPlayTime < currentTime || nextPlayTime > currentTime + maxDelay) {
                    nextPlayTime = currentTime + minDelay; 
                }
                
                source.start(nextPlayTime);
                nextPlayTime += buffer.duration;
            };
        }

        let currentBufferOffset = 1.5; // Starts natively matched with HTML Slider Default

        btn.addEventListener('click', initAudioStream);
        
        volSlider.addEventListener('input', (e) => {
            if (window.masterGain && audioCtx) {
                // Instantly sets volume independent of the 3000ms buffers.
                window.masterGain.gain.setValueAtTime(parseFloat(e.target.value), audioCtx.currentTime);
            }
            localStorage.setItem('pb_volume', e.target.value);
        });

        const bufferSliderUI = document.getElementById('bufferMode');
        const bufferLabelUI = document.getElementById('bufferLabel');
        bufferSliderUI.addEventListener('input', (e) => {
            let val = parseFloat(e.target.value);
            
            // Native dynamic buffer pushing (Add/Subtract audio sequence offset on the fly inside Audio API)
            let delta = val - currentBufferOffset;
            if (audioCtx && isConnected && nextPlayTime > 0) {
                nextPlayTime += delta;
                if (nextPlayTime < audioCtx.currentTime) nextPlayTime = audioCtx.currentTime + 0.08;
            }
            currentBufferOffset = val;
            
            if (val === 0) bufferLabelUI.innerText = "BUFFER: 0.0 sec (ZERO LATENCY)";
            else bufferLabelUI.innerText = "BUFFER: " + val.toFixed(1) + " sec (ANTI-STUTTER)";
            
            if (val === 0) bufferSliderUI.style.accentColor = "#00e5ff"; // Cyan for zero latency
            else bufferSliderUI.style.accentColor = "#00ff00"; // Green for buffered stable
            localStorage.setItem('pb_buffer', e.target.value);
        });

        // Sporadic Javascript Glitch logic
        setInterval(() => {
            if (Math.random() > 0.4) return; // 60% chance to skip this interval, making it unpredictable
            
            const arts = document.querySelectorAll('.ascii-art');
            const layers = document.querySelectorAll('.layer2, .layer3');
            
            // Randomly skew and translate the core
            arts.forEach(el => {
                el.style.transform = `translate(calc(-50% + ${Math.random() * 10 - 5}px), ${Math.random() * 6 - 3}px) skewX(${Math.random() * 15 - 7.5}deg)`;
                if (Math.random() > 0.7) el.style.color = '#ff3333';
            });
            
            // Show chromatic aberration layers with random clip rectangles
            layers.forEach(l => {
                l.style.clip = `rect(${Math.random() * 60}px, 9999px, ${Math.random() * 60 + 10}px, 0)`;
                l.style.opacity = '1';
                l.style.transform = `translate(calc(-50% + ${Math.random() * 20 - 10}px), ${Math.random() * 10 - 5}px)`;
            });

            // Snap back instantly to normal after a tiny fraction of a second
            setTimeout(() => {
                arts.forEach(el => {
                    el.style.transform = 'translateX(-50%) skewX(0)';
                    el.style.color = '#00e5ff';
                });
                layers.forEach(l => {
                    l.style.opacity = '0';
                    l.style.clip = 'auto';
                });
            }, 50 + Math.random() * 150);

        }, 2000 + Math.random() * 3000); // Check every 2-5 seconds randomly
    </script>
</body>
</html>
)=====");
