const webserverVersion = "v2.1.0";

let ws;                         //Websocket variable
let wsOpen = false;             //Bool for checking if websocket has ever been opened => changes the warning message if there's no connection
let wsInterval;                 //Interval timer to detect disconnections
let settings = {};
let power = {};
let ir = {};
let network = {};
let fwVersion;
let hwVersion;
let hwVariant;
let forceRefresh = false;
let uptime;

let txArray = [];
let txCounter = 0;

let disableTimeout = false;

let displayUpdateAvailable = false;

startWebsocket();

/**
 * When the document is loaded
 */
document.addEventListener("DOMContentLoaded", function(event) { 
    
    openTab("Status");

    document.getElementById("webserverVersion").innerHTML = webserverVersion;
    document.getElementById("APs").addEventListener("change", (event) => { 
        let ssid = event.target.value.substring(1,event.target.value.length-1);
        document.getElementById("SSID").value = ssid;
    });
    
    for (let element of document.getElementsByClassName("expandable")) {
        element.addEventListener("click",(event) => {
            let thisElement = event.target;
            if (event.target.className == "expandableIcon") thisElement = event.target.parentElement;
            let nextElement = thisElement.nextElementSibling;
            const collapse = nextElement.className == "collapsed" ? false : true;
            nextElement.className = collapse ? "collapsed" : "";
            thisElement.children[0].src = collapse ? "right.png" : "down.png";
        })
    }
    
    
    document.getElementById("debugEn").addEventListener("change", (event) =>            { sendWS(JSON.stringify({settings:{debug:event.target.checked?'1':'0'}})) });
    document.getElementById("serialOut").addEventListener("change", (event) =>          { sendWS(JSON.stringify({settings:{serialMode:event.target.checked?'1':'0'}})) });
    
    document.getElementById("updateBtn").addEventListener("click", (event) => {
        const url = window.location.href + "update";
        window.open(url, "_blank");
    });

    document.getElementById("resetSettings").addEventListener("click", (event) =>       {
        //Reset popup
        document.getElementById("resetIr").checked = false;
        document.getElementById("resetBattery").checked = false;
        document.getElementById("resetNetwork").checked =false;
        document.getElementById("resetPopup").style.display = "block";
        document.getElementsByClassName("close")[0].addEventListener("click", (event) => {
            document.getElementById("resetPopup").style.display = "none";
        })
        window.onclick = function(event) {
            if (event.target == document.getElementById("resetPopup")) {
                document.getElementById("resetPopup").style.display = "none";
            }
        }
        document.getElementById("confirmResetSettings").addEventListener("click", (event) => {
            sendWS(
                JSON.stringify({
                    event:"reset", 
                    ir: document.getElementById("resetIr").checked,
                    battery: document.getElementById("resetBattery").checked,
                    network: document.getElementById("resetNetwork").checked
                })
                );
            document.getElementById("resetPopup").style.display = "none";
        })
        //if (confirm(`Are you sure you want to reset all settings to their default values? This is irreversable, and will require you to reconfigure everything, including the WiFi settings.`))
        //    sendWS(`SET DEFAULT \n`) 
    });
    document.getElementById("restart").addEventListener("click", (event) =>             { sendWS(JSON.stringify({event:"restart"})); });

    document.getElementById("updateDeviceName").addEventListener("click", (event) =>    { sendWS(JSON.stringify({network:{name:document.getElementById("deviceName").value}})); });
    document.getElementById("scanWiFi").addEventListener("click", (event) =>            { 
        sendWS(JSON.stringify({event:"scanWifi"})); 
        document.getElementById("scanWiFi").value = "Scanning.  ";
        let counter = 0;
        document.scanInterval = setInterval(()=>{
            if (counter >= 40) {
                clearInterval(document.scanInterval);
                document.getElementById("scanWiFi").value = "Scan";
                return;
            }
            const val = document.getElementById("scanWiFi").value;
            if (val == "Scanning.  ") document.getElementById("scanWiFi").value = "Scanning.. ";
            else if (val == "Scanning.. ") document.getElementById("scanWiFi").value = "Scanning...";
            else if (val == "Scanning...") document.getElementById("scanWiFi").value = "Scanning.  ";
            counter++;
        }, 1000);
    });
    document.getElementById("connectWiFi").addEventListener("click", (event) => {
        if (document.getElementById("SSID").value == "")
            alert("No access point selected");
        sendWS(JSON.stringify({event:"connectWifi", ssid:document.getElementById("SSID").value, pw: document.getElementById("password").value}))
    });
    //document.getElementById("updatePort").addEventListener("click", (event) =>                  { sendWS(`SET WS PORT ${document.getElementById("wsPort").value} \n`) });
    
    document.getElementById("mpAutoExposure").addEventListener("click", (event) =>                  { sendWS(JSON.stringify({event:'autoExposure'})); });

    document.getElementById("mpSensorUpdateRate").addEventListener("change", (event) =>             { sendWS(JSON.stringify({ir:{updateRate:event.target.value}})); });
    document.getElementById("mpSensorUpdateRateNumber").addEventListener("change", (event) =>       { sendWS(JSON.stringify({ir:{updateRate:event.target.value}})); });

    document.getElementById("mpSensorBrightness").addEventListener("change", (event) =>             { sendWS(JSON.stringify({ir:{brightness:event.target.value}})); });
    document.getElementById("mpSensorBrightnessNumber").addEventListener("change", (event) =>       { sendWS(JSON.stringify({ir:{brightness:event.target.value}})); });

    document.getElementById("mpSensorMinBrightness").addEventListener("change", (event) =>          { sendWS(JSON.stringify({ir:{minBrightness:event.target.value}})); });
    document.getElementById("mpSensorMinBrightnessNumber").addEventListener("change", (event) =>    { sendWS(JSON.stringify({ir:{minBrightness:event.target.value}})); });

    document.getElementById("mpSensorAverage").addEventListener("change", (event) =>                { sendWS(JSON.stringify({ir:{average:event.target.value}})); });
    document.getElementById("mpSensorAverageNumber").addEventListener("change", (event) =>          { sendWS(JSON.stringify({ir:{average:event.target.value}})); });

    document.getElementById("mpSensorMirrorX").addEventListener("change", (event) =>                { sendWS(JSON.stringify({ir:{mirrorX:event.target.checked ? '1' : '0'}})); });
    document.getElementById("mpSensorMirrorY").addEventListener("change", (event) =>                { sendWS(JSON.stringify({ir:{mirrorY:event.target.checked ? '1' : '0'}})); });
    document.getElementById("mpSensorRotate").addEventListener("change", (event) =>                 { sendWS(JSON.stringify({ir:{rotation:event.target.checked ? '1' : '0'}})); });

    document.getElementById("mpSensorOffsetX").addEventListener("change", (event) =>                { sendWS(JSON.stringify({ir:{offsetX:event.target.value}})); });
    document.getElementById("mpSensorOffsetXNumber").addEventListener("change", (event) =>          { sendWS(JSON.stringify({ir:{offsetX:event.target.value}})); });

    document.getElementById("mpSensorOffsetY").addEventListener("change", (event) =>                { sendWS(JSON.stringify({ir:{offsetY:event.target.value}})); });
    document.getElementById("mpSensorOffsetYNumber").addEventListener("change", (event) =>          { sendWS(JSON.stringify({ir:{offsetY:event.target.value}})); });

    document.getElementById("mpSensorScaleX").addEventListener("change", (event) =>                 { sendWS(JSON.stringify({ir:{scaleX:event.target.value}})); });
    document.getElementById("mpSensorScaleXNumber").addEventListener("change", (event) =>           { sendWS(JSON.stringify({ir:{scaleX:event.target.value}})); });

    document.getElementById("mpSensorScaleY").addEventListener("change", (event) =>                 { sendWS(JSON.stringify({ir:{scaleY:event.target.value}})); });
    document.getElementById("mpSensorScaleYNumber").addEventListener("change", (event) =>           { sendWS(JSON.stringify({ir:{scaleY:event.target.value}})); });

    document.getElementById("mpSensorCalEn").addEventListener("change", (event) =>                  { sendWS(JSON.stringify({ir:{calibration:event.target.checked ? '1' : '0'}})); });
    document.getElementById("mpSensorOffsetEn").addEventListener("change", (event) =>               { sendWS(JSON.stringify({ir:{offsetCalibration:event.target.checked ? '1' : '0'}})); });
    
    document.getElementById("mpConfigPerformCal").addEventListener('click', (event) =>              { sendWS(JSON.stringify({event:"calibration", state:"start", mode:document.getElementById('mpCalMethod').value})); })
    document.getElementById("closeCalibrationPopup").addEventListener('click', (event) =>           { stopCalibration(true, true); });
    document.getElementById("calCloseBtn").addEventListener('click', (event) =>                     { stopCalibration(true, true); });
    document.getElementById("calNextBtn").addEventListener('click', (event) =>                      { sendWS(JSON.stringify({event:"calibration", state:"next"})); });

    //Console
    document.getElementById("consoleClear").addEventListener("click", (event) => {
        document.getElementById("consoleRx").innerHTML = "";
    });
    document.getElementById("consoleTxText").addEventListener("keydown", (event) => {
        if (event.key === 'Enter') {
            const msg = event.target.value;
            if (msg != "") {
                printToConsole(msg, true);
                sendWS(JSON.stringify({debug:msg}));
                document.getElementById("consoleTxText").value = "";
                if (msg != txArray[txCounter])
                    txArray.push(msg);
                txCounter = txArray.length;
            }
        }
        else if (event.key === 'ArrowUp') {
            txCounter--;
            if (txCounter < 0) txCounter = 0;
            let elmnt = document.getElementById("consoleTxText");
            elmnt.value = txArray[txCounter];
        }
        else if (event.key === 'ArrowDown') {
            txCounter++;
            if (txCounter >= txArray.length) txCounter = txArray.length-1;
            let elmnt = document.getElementById("consoleTxText");
            elmnt.value = txArray[txCounter];
        }
    });
    document.getElementById("consoleSend").addEventListener("click", (event) => {
        const msg = document.getElementById("consoleTxText").value;
        if (msg != "") {
            printToConsole(msg, true);
            sendWS(JSON.stringify({debug:msg}));
            if (msg != txArray[txCounter])
                txArray.push(msg);
            txCounter = txArray.length;
        }
    });

    
});

/**
 * When a button is pressed, reload the page after 500ms
 */
function submitMessage() {
    //alert("Saved value to ESP SPIFFS");
    setTimeout(function(){ document.location.reload(false); }, 500);
}

/**
 * Hide or show display elements when a tab selector is pressed
 */
function openTab(tabName) {
    let content = document.getElementsByClassName("tabContent");
    for (let i=0; i<content.length; i++)
        content[i].style.display = "none";

    let buttons = document.getElementsByClassName("tabButtons");
    for (let i=0; i<buttons.length; i++) 
        buttons[i].className = buttons[i].className.replace(" active","");

    document.getElementById(tabName).style.display = "block";

    document.getElementById(tabName + "Btn").className = "tabButtons active";
}

/**
 * Start the websocket connection
 */
async function startWebsocket() {
    console.log("starting WS");
    const ip = 'ws://'+document.location.host + ":3000";
    console.log('IP',ip);
    ws = new WebSocket(ip);

    clearInterval(wsInterval);

    ws.onopen = function() {
        if (forceRefresh) window.location.href = window.location.href;
        console.log("Material Sensor: Websocket connected");
        wsOpen = true;
        clearInterval(wsInterval);
        wsInterval = setInterval(resetWS, 5000);
    }

    ws.onmessage = function(msg){
        analyzeMessage(msg.data);
        clearInterval(wsInterval);
        wsInterval = setInterval(resetWS, 5000);
    }

    clearInterval(wsInterval);
    wsInterval = setInterval(resetWS, 1000);
}

/**
 * Try to reset the websocket if a connection is lost
 */
 function resetWS(delay){
    if (wsOpen && !disableTimeout) {
        document.getElementById('mpAutoExposure').style.background = '';
        document.getElementById("waitMessage").style = "";
        document.getElementById("mainBody").style = "display:none";
        document.getElementById("waitMessageTxt").innerHTML = `
            Lost connection with the sensor.<br>
            Attempting to reestablish, please wait.`;

        wsOpen = false;
        console.log("Websocket disconnected");
        if (delay != undefined) setTimeout(startWebsocket,delay);
        else startWebsocket();
    }
    else if (ws.readyState == 3){
        console.log("Can't connect to websocket server");
        startWebsocket();
    }
}

/**
 * Send data to the websocket server
 * @param {String} txt Data to send
 */
function sendWS(txt){
    if (wsOpen)
        ws.send(txt);
}

const pointColors = ['#FF0000', '#00AD00', '#0000FF', '#FFFF00', '#FF00FF', '#7F00FF', '#007FFF', '#FF7F00', '#000000'];

/**
 * Analyze the data received from the websocket server
 * @param {String} msg Received data
 */
function analyzeMessage(msg) {
    //console.log("wsMessage",msg);
    let data;
    try {
        data = JSON.parse(msg);
    } catch (error) {
        console.warn("Could not parse JSON: " + msg);
        return;
    }
    //console.log('data',data)

    if (data.status == "ping") {
        document.getElementById("waitMessage").style = "display:none";
        document.getElementById("mainBody").style = "";
    }
    //else console.log('data',data)

    if (data.status == "disableTimeout") {
        disableTimeout = true;
    }
    else if (data.status == "enableTimeout") {
        disableTimeout = false;
    }

    else if (data.status == "update") {
        settings = data.settings;
        power = data.power;
        ir = data.ir;
        network = data.network;
        fwVersion = data.firmwareVersion;
        hwVersion = data.hardwareVersion;
        hwVariant = data.hardwareVariant;
        hwVariantFilter(hwVariant);
        uptime = new Date(data.uptime * 1000).toISOString().slice(11, 16);

        /*
        let elements = document.getElementsByClassName("mpBeta");
        //let display = hwVersion == 'Beta' ? '' : 'none';
        let display = '';
        for (let elmnt of elements) elmnt.style.display = display;

        elements = document.getElementsByClassName("diy_basic");
        display = hwVariant == 'DIY Basic' ? 'none' : '';
        for (let elmnt of elements) elmnt.style.display = display;
        */

        document.getElementById("hwVariant").innerHTML = hwVariant;
        document.getElementById("hwVersion").innerHTML = `v${hwVersion}`;
        document.getElementById("fwVersion").innerHTML = `v${fwVersion}`;
        document.getElementById("uptime").innerHTML = uptime;
        document.getElementById("debugEn").checked = settings.debug;
        document.getElementById("serialOut").checked = settings.serialMode == 'default';

        if (hwVariant == "Production" || hwVariant == "DIY Full" || hwVariant == "DIY Full S3") {
            //Power settings
            document.getElementById("chargingState").innerHTML = power.chargerStatus;
            if (power.chargerStatus != "Not Charging" && power.chargerStatus != "Charging" && power.chargerStatus != "Charged" && power.chargerStatus != "USB Not Connected") document.getElementById("chargingState").style.color = '#FF0000';
            else document.getElementById("chargingState").style.color = '#000000';
            //document.getElementById("usbConnected").innerHTML = settings.usbConnected == 1 ? "Yes" : "No";
            document.getElementById("batteryPercentage").innerHTML = `${power.percentage}%`;
            if (power.chargerStatus == "Charging") document.getElementById("batteryTimeToEmpty_label").innerHTML = 'Time to Full';
            else if (power.chargerStatus == "Not Charging") document.getElementById("batteryTimeToEmpty_label").innerHTML = 'Time to Empty';
            if (power.chargerStatus == "Charging" || power.chargerStatus == "Not Charging") {
                const hr = Math.floor(power.time/3600);
                const min = Math.floor((power.time%3600)/60);
                const sec = (power.time%3600)%60;
                let timeStr = "";
                timeStr += hr + ':';
                timeStr += min<10 ? `0${min}` : min;
                if (hwVariant == "Production") {
                    document.getElementById("batteryTimeToEmpty_div").style.display = "";
                    document.getElementById("batteryTimeToEmpty").innerHTML = timeStr;
                }
                else if (hwVariant == "DIY Full" || hwVariant == "DIY Full S3") {
                    document.getElementById("batPercentage").style.display = "none";
                }
            }
            else if (hwVariant == "Production") document.getElementById("batteryTimeToEmpty_div").style.display = "none";
            else if (hwVariant == "DIY Full" || hwVariant == "DIY Full S3") {
                document.getElementById("batPercentage").style.display = "";
            }
            const voltage = Math.round(parseInt(power.voltage)/100)/10;
            document.getElementById("batteryVoltage").innerHTML = `${voltage.toPrecision(2)} V`;
            if (hwVariant == "Production") {
                document.getElementById("batteryCurrent").innerHTML = `${power.current} mA`;
                document.getElementById("batteryCapacity").innerHTML = `${power.capacity}/${power.fullCapacity} mAh`;
            }
        }
        
        document.getElementById("connectionStatus").innerHTML = network.wifiConnected ? "Connected" : "Not Connected";
        document.getElementById("ssid").innerHTML = `"${network.ssid}"`;
        document.getElementById("ipAddress").innerHTML = network.ip;
        document.getElementById("deviceName").value = network.name;
        //document.getElementById("wsMode").innerHTML = network.websocketMode;
        document.getElementById("wsPort").value = network.websocketPort;
        let clients = `${network.connectedClients}`;
        clients += network.clients.length == 1 ? ` client<br>` : ` clients<br>`;
        for (let i=0; i<network.clients.length; i++)
            clients += `${i+1}: ${network.clients[i]}<br>`;
        document.getElementById("wsClients").innerHTML = clients;

        //IR settings
        document.getElementById("mpSensorUpdateRate").value=ir.updateRate;
        document.getElementById("mpSensorUpdateRateNumber").value=ir.updateRate;

        
        document.getElementById("mpSensorBrightness").value=ir.brightness;
        document.getElementById("mpSensorBrightnessNumber").value=ir.brightness;

        document.getElementById("mpSensorMinBrightness").value=ir.minBrightness;
        document.getElementById("mpSensorMinBrightnessNumber").value=ir.minBrightness;

        document.getElementById("mpSensorAverage").value=ir.average;
        document.getElementById("mpSensorAverageNumber").value=ir.average;

        document.getElementById("mpSensorMirrorX").checked=ir.mirrorX;
        document.getElementById("mpSensorMirrorY").checked=ir.mirrorY;
        document.getElementById("mpSensorRotate").checked=ir.rotation;
        document.getElementById("mpSensorOffsetX").value=ir.offsetX;
        document.getElementById("mpSensorOffsetXNumber").value=ir.offsetX;
        document.getElementById("mpSensorOffsetY").value=ir.offsetY;
        document.getElementById("mpSensorOffsetYNumber").value=ir.offsetY;
        document.getElementById("mpSensorScaleX").value=ir.scaleX;
        document.getElementById("mpSensorScaleXNumber").value=ir.scaleX;
        document.getElementById("mpSensorScaleY").value=ir.scaleY;
        document.getElementById("mpSensorScaleYNumber").value=ir.scaleY;
        document.getElementById("mpSensorCalEn").checked=ir.calibrationEnable;
        document.getElementById("mpSensorOffsetEn").checked=ir.offsetEnable;

        checkForUpdates(fwVersion, webserverVersion.split('v')[1]);
    }
    else if (data.status == "debug") {
        printToConsole(data.message);
    }
   
    else if (data.status == "wifiStations") {
        document.getElementById("scanWiFi").value = "Scan";
        const stations = data.stations;
        ssidElement = document.getElementById("APs");
        for (let i=0; i<stations.length; i++) {
            const station = stations[i];
            let newOption = document.createElement("option");
            newOption.value = `"${station.ssid}"`;
            newOption.innerHTML = `${station.ssid} (${station.rssi}dBm/${station.authMode})`;
            ssidElement.appendChild(newOption);
        }
    }
     
    else if (data.status == "IR data") {
        if (calibrationActive)
            updateCalibrationPoint(data);
        else 
            drawIrCoordinates(data);
    }
    else if (data.status == 'autoExposure') {
        if (data.state == 'starting') {
            document.getElementById('mpAutoExposure').style.background = 'green';
        }
        else if (data.state == 'done') {
            document.getElementById('mpAutoExposure').style.background = '';
        }
        if (data.state == 'cancelled') {
            document.getElementById('mpAutoExposure').style.background = '';
            alert("Autoexposure failed. Please make sure only 1 base/pen is active and try again.")
        }
    }
    else if (data.status == 'calibration') {
        if (data.state == 'starting') startCalibration(data.mode);
        else if (data.state == 'newPoint' && data.mode == 'MultiPoint') updateMultiCalibrationPoint(data.points);
        else if (data.state == 'newPoint') setCalibrationPoint(data);
        else if (data.state == 'done') calibrationDone();
        else if (data.state == 'cancelled') stopCalibration();
        
    }

}

let calibrationActive = false;
let calibrationPointCount = 0;
let countdown;
let calibrationMode = '';

function startCalibration(mode) {
    calibrationMode = mode;
    calibrationActive = true;
    document.getElementById("noMovement").style="";
    document.getElementById("waiting").style="display:none";
    document.getElementById("calDone").style="display:none";
    document.getElementById("calCancel").style="display:none";
    document.getElementById("calCloseBtn").style="display:none";
    document.getElementById("calNextBtn").style="";
    document.getElementById("calibrationPopup").style.display = "block";
    countdownCount = 5;
    calibrationPointCount = 0;
    clearInterval(countdown);
    for (let i=0; i<4; i++) {
        document.getElementById("mpCalPoint_x-"+i).innerHTML=Math.round(0);
        document.getElementById("mpCalPoint_y-"+i).innerHTML=Math.round(0);
        document.getElementById("iterationPoint-"+i).style.color='grey';
        document.getElementById("mpCalPoint_x-"+i).style.color='grey';
        document.getElementById("mpCalPoint_y-"+i).style.color='grey';
    }
    
    let msg = '';
    if (mode == 'SinglePoint') msg += "Starting single-point calibration.<br>";
    else if (mode == 'Offset') msg += "Starting offset calibration.<br>";
    if (mode == 'SinglePoint' || mode == 'Offset') msg +=
        `
            To calibrate, move the base to one of the corners of your display and wait a few seconds (production hardware) or hold the button on the base for a few seconds and then wait (DIY hardware).<br>
            The center of the base must align with the corner of the display.
        `
    if (mode == 'MultiPoint') {
        msg += `
            Starting multipoint calibration.<br>
            To calibrate, make sure all 4 IR points are visible and their coordinates are displayed. Then press 'Calibrate'.
        `;
        document.getElementById('calNextBtn').innerHTML='Calibrate';
        document.getElementById('calNextBtn').disabled=true;
    }
    else {
        document.getElementById('calNextBtn').innerHTML='Next';
        document.getElementById('calNextBtn').disabled=false;
    }
    document.getElementById("calibrationContent").innerHTML = msg;
}

function updateCalibrationPoint(data) {
    if (calibrationMode == 'MultiPoint') return;

    const point = data.irPoints[0];

    document.getElementById("noMovement").style="display:none";
    document.getElementById("waiting").style="";
    countdownCount = 5;
    const txt = `<b>Locking in point in 5 seconds.</b>`;
    document.getElementById("waiting").innerHTML=txt;
    clearInterval(countdown);
    countdown = setInterval(calibrationTimer,1000);

    if (point.x != undefined && point.y != undefined && point.x != 0 && point.y != 0 && point.x != -9999 && point.y != -9999) {
        document.getElementById("mpCalPoint_x-"+calibrationPointCount).innerHTML=Math.round(point.x);
        document.getElementById("mpCalPoint_y-"+calibrationPointCount).innerHTML=Math.round(point.y);
    }

}

function updateMultiCalibrationPoint(data) {
    if (calibrationMode != 'MultiPoint') return;

    let points = 0;
    for (let i = 0; i<4; i++) {
        if (data[i]?.x != undefined && data[i]?.y != undefined) {
            if (data[i].x != 0 && data[i].y != 0 && data[i].x != -9999 && data[i].y != -9999) {
                points++;
                document.getElementById("mpCalPoint_x-"+i).innerHTML=Math.round(data[i].x);
                document.getElementById("mpCalPoint_y-"+i).innerHTML=Math.round(data[i].y);
            }
            document.getElementById("iterationPoint-"+i).style.color='black';
            document.getElementById("mpCalPoint_x-"+i).style.color='black';
            document.getElementById("mpCalPoint_y-"+i).style.color='black';
        }
        else {
            document.getElementById("mpCalPoint_x-"+i).innerHTML=0;
            document.getElementById("mpCalPoint_y-"+i).innerHTML=0;
            document.getElementById("iterationPoint-"+i).style.color='grey';
            document.getElementById("mpCalPoint_x-"+i).style.color='grey';
            document.getElementById("mpCalPoint_y-"+i).style.color='grey';
        }
    }

    if (points == 4) {
        document.getElementById('calNextBtn').disabled=false;
    }
    else {
        document.getElementById('calNextBtn').disabled=true;
    }
}

function setCalibrationPoint(data) {
    if (data.x != undefined && data.y != undefined && data.x != 0 && data.y != 0 && data.x != -9999 && data.y != -9999) {
        document.getElementById("mpCalPoint_x-"+data.point).innerHTML=Math.round(data.x);
        document.getElementById("mpCalPoint_y-"+data.point).innerHTML=Math.round(data.y);
        calibrationPointCount++;
        document.getElementById("iterationPoint-"+data.point).style.color='black';
        document.getElementById("mpCalPoint_x-"+data.point).style.color='black';
        document.getElementById("mpCalPoint_y-"+data.point).style.color='black';
    }
}

function calibrationTimer() {
    countdownCount = countdownCount-1;
    if (!calibrationActive) {
        clearInterval(countdown);
        return;
    }
        
    if (countdownCount > 0) {
        const txt = `<b>Locking in point in ` + countdownCount + ` seconds.</b>`;
        document.getElementById("waiting").innerHTML=txt;
    }
    else {
        clearInterval(countdown);
        countdownCount = 5;
        document.getElementById("noMovement").style="";
        document.getElementById("waiting").style="display:none";
        sendWS(JSON.stringify({event:"calibration", state:"next"}));
    }
}

function nextCalibrationPoint() {
    if (!calibrationActive) return;
    sendWS(JSON.stringify({event:"calibration", state:"next"}));
}

function calibrationDone() {
    setTimeout(function(){document.getElementById("calibrationPopup").style.display = 'none';},5000);
    calibrationActive = false;
    document.getElementById("calDone").style="";
    document.getElementById("calNextBtn").style="display:none";
    document.getElementById("calCloseBtn").style="";
    document.getElementById("noMovement").style="display:none";
    document.getElementById("waiting").style="display:none";
}

function stopCalibration(closeNow = false, sendWs = false) {
    calibrationActive = false;
    if (sendWs) sendWS(JSON.stringify({event:"calibration", state:"cancel"}));
    if (closeNow) document.getElementById("calibrationPopup").style.display = 'none'
    else {
        document.getElementById("calCancel").style="";
        document.getElementById("calNextBtn").style="display:none";
        document.getElementById("calCloseBtn").style="";
        document.getElementById("noMovement").style="display:none";
        document.getElementById("waiting").style="display:none";
        setTimeout(function(){document.getElementById("calibrationPopup").style.display = 'none';},5000);
    }
}

function updateIrPoint(data, point) {
    if (point.number > 3) return;
    document.getElementById("mpCoordsBaseId").innerHTML=data.id;
    document.getElementById("mpCoordsBaseCmd").innerHTML=data.command;
    document.getElementById("mpCoordsBaseBat").innerHTML=data.id==0?'0%':`${data.battery}%`;

    if (point == undefined || isNaN(point.x)  || isNaN(point.y) || point.x == -9999 || point.y == -9999) {
        point.x = 0;
        point.y = 0;
        point.avgBrightness = 0;
        point.maxBrightness = 0;
        point.area = 0;
    }
    document.getElementById("mpCoordsX-"+point.number).innerHTML = Math.round(point.x);
    document.getElementById("mpCoordsY-"+point.number).innerHTML = Math.round(point.y);
    document.getElementById("mpCoordsAvgBrightness-"+point.number).innerHTML = Math.round(point.avgBrightness);
    document.getElementById("mpCoordsMaxBrightness-"+point.number).innerHTML = Math.round(point.maxBrightness);
    document.getElementById("mpCoordsArea-"+point.number).innerHTML = Math.round(point.area);

    let color = "black";
    if (point.x < 0 || point.x > 4096 || point.y < 0 || point.y > 4096) color = "red";
    if (point.maxBrightness == 0) color = "grey";

    document.getElementById("mpCoordsPoint-"+point.number).style.color = (point.maxBrightness == 0) ? 'grey' : pointColors[point.number];
    document.getElementById("mpCoordsX-"+point.number).style.color = color;
    document.getElementById("mpCoordsY-"+point.number).style.color = color;
    document.getElementById("mpCoordsAvgBrightness-"+point.number).style.color = color;
    document.getElementById("mpCoordsMaxBrightness-"+point.number).style.color = color;
    document.getElementById("mpCoordsArea-"+point.number).style.color = color;
}

function drawIrCoordinates(data) {
    if ( document.getElementById('coordinatesDiv').classList.contains("collapsed") ) return;
    var stage = document.getElementById('stage');
    if(stage.getContext) {
        var ctx = stage.getContext('2d');
    }

    ctx.clearRect(0, 0, stage.width, stage.height);
    for (let point of data.irPoints) {
        updateIrPoint(data, point);
        if (point == undefined || point.x == undefined) continue;
        if (point.x > 0 && point.y < 4096) {
            const x = point.x/4096*stage.width;
            const y = point.y/4096*stage.height;
            ctx.fillStyle = pointColors[point.number];
            ctx.beginPath();
            ctx.arc(x,y,3,0,2*Math.PI,false);
            ctx.fill();
            ctx.fillText(point.number, x + 5, y + 10);
        }
    }
}

function getTimestamp() {
    let msg = "";
    let t = new Date();
    msg += t.getHours()<10 ? `0${t.getHours()}` : t.getHours();
    msg += ":";
    msg += t.getMinutes()<10 ? `0${t.getMinutes()}` : t.getMinutes();
    msg += ":";
    msg += t.getSeconds()<10 ? `0${t.getSeconds()}` : t.getSeconds();
    msg += ".";
    if (t.getMilliseconds() < 100) msg += '0';
    if (t.getMilliseconds() < 10) msg += '0';
    msg += t.getMilliseconds() + " - ";
    return msg;
}

function printToConsole(txt, tx=false) {
    let msg = "";
    if (tx) msg += "<i>"
    if (document.getElementById("consoleTimeStamp").checked) {
        msg += getTimestamp();
    }
    msg += txt;
    if (tx) msg += "<i>"
    msg += "<br>";
    let rxElmnt = document.getElementById("consoleRx");
    rxElmnt.innerHTML += msg;
    if (document.getElementById("consoleAutoscroll").checked) 
        rxElmnt.scrollTop = rxElmnt.scrollHeight;
}

let hwVariantFiltered = "";

function hwVariantFilter(variant) {
    if (variant == 'DIY Basic') variant = 'DIY_Basic';
    else if (variant == 'DIY Full' || hwVariant == "DIY Full S3") variant = 'DIY_Full';
    if (hwVariantFiltered == variant) return;

    hideElementsByClassName('Production', variant != 'Production');
    hideElementsByClassName('Beta', variant != 'Beta');
    hideElementsByClassName('DIY_Basic', variant != 'DIY_Basic');
    hideElementsByClassName('DIY_Full', variant != 'DIY_Full');
    hideElementsByClassName('Production/Full', variant != 'DIY_Full' && variant != 'Production');

    hwVariantFiltered = variant;
}

function hideElementsByClassName(className, hide) {
    const elmnts = document.getElementsByClassName(className);
    for (let elmnt of elmnts) {
        elmnt.style.display = hide ? 'none' : '';
    }
}

function checkForUpdates(fwVersion, wsVersion) {
    if (displayUpdateAvailable) return;
    displayUpdateAvailable = true;

    const url = 'https://api.github.com/repos/MaterialFoundry/MaterialPlane_Sensor/releases';

    var request = new XMLHttpRequest();
    request.open('GET', url, true);
    request.send(null);
    request.onreadystatechange = function () {
        if (request.readyState === 4 && request.status === 200) {
            var type = request.getResponseHeader('Content-Type');
            if (type.indexOf("text") !== 1) {
                let response;
                try {
                    response = JSON.parse(request.responseText);
                }
                catch (error) {
                    return null;
                }
                let fw, ws = false;

                const firmwareTagName = response.filter(r => r.tag_name.includes('Firmware'))[0].tag_name;
                let firmware = null;
                if (firmwareTagName != undefined) {
                    firmware = firmwareTagName.split('_v')[1];
                    fw = compareVersions(fwVersion,firmware);
                }

                const webserverTagName = response.filter(r => r.tag_name.includes('Webserver'))[0].tag_name;
                let webserver = null;
                if (webserverTagName != undefined) {
                    webserver = webserverTagName.split('_v')[1];
                    ws = compareVersions(wsVersion,webserver);
                }

                let msg = "";
                if (!fw && !ws) return;
                let fwColor, wsColor = "";
                if (fw) fwColor = "style='color:red'";
                if (ws) wsColor = "style='color:red'";

                msg = `
                <table>
                <tr><th></th><th>Current</th><th>Latest</th</tr>
                <tr><td>Firmware</td><td ${fwColor}>v${fwVersion}</td><td>v${firmware}</td></tr>
                <tr><td>Webserver</td><td ${wsColor}>v${wsVersion}</td><td>v${webserver}</td></tr>
                </table>`;

                document.getElementById("popupContent").innerHTML = msg;
                document.getElementById("updatePopup").style.display = "block";
                document.getElementsByClassName("close")[0].addEventListener("click", (event) => {
                    document.getElementById("updatePopup").style.display = "none";
                })
                window.onclick = function(event) {
                    if (event.target == document.getElementById("updatePopup")) {
                        document.getElementById("updatePopup").style.display = "none";
                    }
                }
                return;
            }
            
        }
    }
    request.onerror = function () {
        
    }
}

function compareVersions(currentVersion, latestVersion) {
    latestVersion = latestVersion.split(".");
    currentVersion = currentVersion.split(".");
    
    for (let i=0; i<3; i++) {
        latestVersion[i] = isNaN(parseInt(latestVersion[i])) ? 0 : parseInt(latestVersion[i]);
        currentVersion[i] = isNaN(parseInt(currentVersion[i])) ? 0 : parseInt(currentVersion[i]);
    }
    
    if (currentVersion[0] > latestVersion[0]) return false;
    if (currentVersion[0] < latestVersion[0]) return true;
    if (currentVersion[1] > latestVersion[1]) return false;
    if (currentVersion[1] < latestVersion[1]) return true;
    if (currentVersion[2] >= latestVersion[2]) return false;
    return true;
  }