// preload.js
const { contextBridge, ipcRenderer } = require('electron');

contextBridge.exposeInMainWorld('electron', {
  // Expose require to the renderer process
  require: require,
  
  // Expose a function to send data to the main process
  sendToMain: (channel, data) => {
    ipcRenderer.send(channel, data);
  },
  
  // To receive data from the main process
  receiveFromMain: (channel, callback) => {
    ipcRenderer.on(channel, (event, ...args) => {
      callback(...args);
    });
  },

});

