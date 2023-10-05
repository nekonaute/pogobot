const { app, BrowserWindow, ipcMain } = require('electron');
const path = require('node:path');
const SerialPort = require('serialport').SerialPort ;
const fs = require('fs');

let mainWindow;
let port;
let ttyUSBDevicesList;

let recovering_help = 0;
let help_text;

// function to find all the ttyUSB device connected
function findTTYUSBDevices(directory) {
  try {
    // Read the directory "/dev/"
    const files = fs.readdirSync(directory);

    // Filter name containing "ttyUSB"
    const ttyUSBDevices = files.filter((file) => file.includes('ttyUSB'));

    // Creates un device structure for each ttyUSB
    const devicesList = ttyUSBDevices.map((device) => ({
      name: device,
      index: device.slice(-1),
      address: directory+device,
    }));

    return devicesList;
  } catch (error) {
    console.error('Error during ttyUSB scaning :', error);
    return [];
  }
}

app.on('ready', () => {
  // create the main windows
  mainWindow = new BrowserWindow({ width: 1024, height: 800, 
                                  webPreferences: {
                                    preload: path.join(__dirname, 'preload.js')                                 
                                  }});
  // load the main web page
  mainWindow.loadFile('index.html');

  // find all ttyUSB device
  ttyUSBDevicesList = findTTYUSBDevices('/dev/');
  console.log('ttyUSB Devices found :', ttyUSBDevicesList);

  // exit if no ttyUSB are found
  if (ttyUSBDevicesList.length == 0) {
    app.quit();
  }

  //create an array for the device
  port = Array(ttyUSBDevicesList.length);

  for (let index = 0; index < ttyUSBDevicesList.length; index++) {
    const element = ttyUSBDevicesList[index];

    element.index = index;
    
    // open the port
    port[index] = new SerialPort({ path: element.address,
                                  baudRate: 115200,
                                  dataBits: 8,
                                  stopBits: 1,
                                  parity: 'none',});

    // listen the port
    port[index].on('data', (data) => {
      //console.log(data);
      mainWindow.webContents.send('uart-data'+index, data.toString());
    });

    //preload le prompt   
    port[index].write('\n', function(error) {
      if (error) {
        console.error('Error when writing on ttyUSB:', error);
      }
    });

    // connect the listener from the web page
    ipcMain.on('send_cmd'+index, (event, data) => {
      if (port[index]) {
        port[index].write(data + '\n', function(error) {
          if (error) {
            console.error('Error when writing on ttyUSB:', error);
          }
        });
      } else {
        console.error('Serial Port not available');
      }
    });

  }

  // send the device list to the front
  mainWindow.webContents.on('did-finish-load', () => {
    mainWindow.webContents.send('ttyUSBDevicesList', JSON.stringify(ttyUSBDevicesList));
  });

});

