
let ttyUSBDevicesList;

// recover list of TTYUsb devices
window.electron.receiveFromMain('ttyUSBDevicesList', (data) => {

    ttyUSBDevicesList = JSON.parse(data);

    const devicesContainer = document.getElementById('communication');

    for (let index = 0; index < ttyUSBDevicesList.length; index++) {
        const element = ttyUSBDevicesList[index];
        
        console.log(element);

        // create the html dynamically
        const deviceDiv = document.createElement('div');
        deviceDiv.classList.add('device');

        const cmdDiv = document.createElement('div');
        cmdDiv.classList.add('device_cmd');
        const cmd_title = document.createElement('h2');
        cmd_title.textContent = element.name;
        cmd_title.id = 'title'+element.index;
        cmdDiv.appendChild(cmd_title);
        //<input type="text" id="uart-command" placeholder="Entrez une commande UART">
        const input_cmd = document.createElement('input');
        input_cmd.id="uart-command"+element.index;
        input_cmd.type="text";
        input_cmd.autocomplete="off";
        input_cmd.setAttribute('list', 'suggestions-cmd-list'+element.index);
        input_cmd.placeholder="Entrez une commande UART";
        cmdDiv.appendChild(input_cmd);
        //<datalist id="suggestions-cmd-list">
        //<option value="suggestion 1">
        //</datalist>
        const input_cmd_list = document.createElement('datalist');
        input_cmd_list.id='suggestions-cmd-list'+element.index;

        // Tableau de suggestions
        const suggestions = ['rc_send_user_msg', 'rc_send_user_msg_cont','rc_send_bios_cmd', 'rc_send_bios_cmd_cont'];

        // Ajoutez des options à la datalist en utilisant une boucle
        suggestions.forEach((suggestionText) => {
            const option = document.createElement('option');
            option.value = suggestionText;
            input_cmd_list.appendChild(option);
        });
        cmdDiv.appendChild(input_cmd_list);

        //<button id="send-command">Envoyer</button>
        const cmd_button = document.createElement('button');
        cmd_button.id = "send-command"+element.index;
        cmd_button.textContent="Send";
        cmdDiv.appendChild(cmd_button);

        deviceDiv.appendChild(cmdDiv);

        const recpDiv = document.createElement('div');
        recpDiv.classList.add('device_recp');
        //const recp_title = document.createElement('h2');
        //recp_title.textContent = element.name + ' return';
        //recpDiv.appendChild(recp_title);
        //<div id="uart-reception" class="uart-reception"></div>
        const recp_text = document.createElement('div');
        recp_text.id = "uart-reception"+element.index;
        recp_text.classList.add('uart-reception');
        recpDiv.appendChild(recp_text);

        deviceDiv.appendChild(recpDiv);

        devicesContainer.appendChild(deviceDiv);

        //connect the data from the UART
        window.electron.receiveFromMain('uart-data'+element.index, (data) => {
            const uartReceptionDiv = document.getElementById('uart-reception'+element.index);
          
            var datas=data.split("\n\r");
            var dataCheck=data.slice(-1);
          
            // process the data to handle ANSI escape caracters and
            // the fact that message can be cut in the middle because of the buffer size
            for (let index = 0; index < datas.length; index++) {
              const element = datas[index];

              //\e[92;1mlitex-demo-app\e[0m> ");
              const elementFormated = parseANSIWithEscape(element);

              uartReceptionDiv.innerHTML += `<span>${elementFormated}</span>`;
              if ((index != datas.length-1) || (index == datas.length-1 && dataCheck == '\n') ) {
                  uartReceptionDiv.innerHTML += '<br>';
              } 
            }
          
            // automatically scroll
            uartReceptionDiv.scrollTop = uartReceptionDiv.scrollHeight;
        });

        // connect the send command button
        const sendButton = document.getElementById('send-command'+element.index);
        sendButton.addEventListener('click', () => {
            sendUARTCommandFromUser(element.index);
        });
        // connect the "enter" key press for the send command
        const uartCommandInput = document.getElementById('uart-command'+index);
        uartCommandInput.addEventListener("keyup", ({key}) => {
            if (key === "Enter") {
                sendUARTCommandFromUser(element.index);
            }
        })

        // allow the autocompletion
        input_cmd.addEventListener('input', function() {
            const userInput = input_cmd.value.toLowerCase(); // Convert in lowerCase
            const datalist = document.getElementById('suggestions-cmd-list'+element.index);
            const options = datalist.getElementsByTagName('option');
          
            // check each datalist option
            for (let i = 0; i < options.length; i++) {
              const optionValue = options[i].value.toLowerCase();
          
              // compare to user text
              if (optionValue.indexOf(userInput) !== -1) {
                options[i].style.display = 'block'; // Show
              } else {
                options[i].style.display = 'none'; // Hide
              }
            }
        });
    }

    // Generates options lists for the layout section
    generateOptions('ttyUSB-selector-top');
    generateOptions('ttyUSB-selector-left');
    generateOptions('ttyUSB-selector-right');
    generateOptions('ttyUSB-selector-bottom');

});

// parsing ANSI escape caracter function
function parseANSIWithEscape(text) {
    // Regular expression used to search 
    const ansiRegex = /\x1B\[[1-9;]*[A-Za-z]/g;
    const ansiRegexEnd = /\x1B\[0m/g;

    let position = text.search(ansiRegex);
    let positionEnd = text.search(ansiRegexEnd);
    if (position != -1 && positionEnd == -1 ) {
        console.log(text);
    }


    // Replace ANSI carater by HTML span
    const htmlText = text.replace(ansiRegex, (match) => {
      const code = match.slice(2, -1); // Remove "\e[" and "m"

      const params = code.split(';');
      const style = [];
  
      params.forEach((param) => {
        const num = parseInt(param);
  
        if (!isNaN(num)) {
          if (num === 1) {
            style.push('font-weight:bold'); // Bold
          } else if (num >= 30) {
            style.push(`color: green`); // green
          }
        }
      });
  
      // Return the jointed style
      return `<span style="${style.join(';')}">`;
    });
  
    // Close the HTML span 
    return htmlText.replace(ansiRegexEnd, '</span>');
}

// Function to generated option list
function generateOptions(selectorId) {
    const selector = document.getElementById(selectorId);
    
    ttyUSBDevicesList.forEach((device) => {
        const option = document.createElement('option');
        option.value = device.name;
        option.textContent = device.name;
        selector.appendChild(option);
    });
}

// Function to send message from the user
function sendUARTCommandFromUser(index) {
    const uartCommandInput = document.getElementById('uart-command'+index);
    const command = uartCommandInput.value;
    console.log('send command:', command);
    window.electron.sendToMain('send_cmd'+index, command);
    uartCommandInput.value = ''; // Remove the old text
    
}

// Function to chosse the color
function assignColorByIndex(index) {
    const colors = ['0x800000', '0x000080', '0x008000', '0x800080']; // Rouge, Bleu, Vert, Violet
    return colors[index % colors.length];
}

// function to send the right color to each device
const identificationButton = document.getElementById('identification-button');
identificationButton.addEventListener('click', () => {
  // for each device ttyUSB
  ttyUSBDevicesList.forEach((device) => {

    const color = assignColorByIndex(device.index);

    // Create the command
    const uartCommand = `set_led ${color}`;

    // Send it
    sendUARTCommand(uartCommand, device.index);
  });
});

// Function to send message from the controler
function sendUARTCommand(command, index) {
    // recover the input id
    const uartCommandInput = document.getElementById(`uart-command${index}`);
  
    // Right the message send by the controler
    uartCommandInput.value = command;
  
    //send it
    window.electron.sendToMain('send_cmd'+index, command);
  }

// function to update the name of each device interface
const changeDeviceNameButton = document.getElementById('change-device-name');
changeDeviceNameButton.addEventListener('click', () => {

  // Recover selector
  const selectorTop = document.getElementById('ttyUSB-selector-top');
  const selectorLeft = document.getElementById('ttyUSB-selector-left');
  const selectorRight = document.getElementById('ttyUSB-selector-right');
  const selectorBottom = document.getElementById('ttyUSB-selector-bottom');
  // Recover value of the selector
  const valueTop = selectorTop.value;
  const valueLeft = selectorLeft.value;
  const valueRight = selectorRight.value;
  const valueBottom = selectorBottom.value;

  // update the name
  ttyUSBDevicesList.forEach(element => {
    if ( element.name === valueTop) {
        add_title_prefix( element.index , element.name+'-top');
    } else if ( element.name === valueLeft) {
        add_title_prefix( element.index , element.name+'-left');
    } else if ( element.name === valueRight) {
        add_title_prefix( element.index , element.name+'-right');
    } else if ( element.name === valueBottom) {
        add_title_prefix( element.index , element.name+'-down');
    }
  });

});

// function to update the name of h2
function add_title_prefix(index, name) {
    const title = document.getElementById('title'+index);
    title.textContent = name;
}