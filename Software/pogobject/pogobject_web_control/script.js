
function sendRebootRequest() {
	var ipAddress = document.getElementById("ipAddress").value;
	var url = "http://" + ipAddress + "/reboot_p";
	
	fetch(url)
        .then(response => response.json())
        .then(result => {
			// Formatez la réponse JSON de manière jolie
            var formattedJson = JSON.stringify(result, null, 2); // Le deuxième argument spécifie la mise en forme
            var statusResultDiv = document.getElementById("resultDiv");
            statusResultDiv.innerHTML = "<pre>Server response:\n" + formattedJson + "</pre>";
        })
        .catch(error => {
            console.error("Error:", error);
			var statusResultDiv = document.getElementById("resultDiv");
			statusResultDiv.textContent = "Error fetching server status: " + error.message;
        });
}

function toggleCustomParams() {
	var commandType = document.getElementById("commandType").value;
	var customParamsField = document.getElementById("customParamsField");
	
	if (commandType === "autotest" 
			|| commandType === "rc_send_user_msg" 
			|| commandType === "rc_send_cmd_continusly" 
			|| commandType === "rc_send_cmd" ) {
		customParamsField.style.display = "block";
	} else {
		customParamsField.style.display = "none";
	}
}

function sendCommandRequest() {
	var ipAddress = document.getElementById("ipAddress").value;
	var commandType = document.getElementById("commandType").value;
	var customParams = document.getElementById("customParams").value;
	var fullCommand = commandType + (customParams ? " " + customParams : "");
	var url = "http://" + ipAddress + "/cmd?params=" + encodeURIComponent(fullCommand);

	fetch(url)
        .then(response => response.json())
        .then(result => {
			// Formatez la réponse JSON de manière jolie
            var formattedJson = JSON.stringify(result, null, 2); // Le deuxième argument spécifie la mise en forme
            var statusResultDiv = document.getElementById("resultDiv");
            statusResultDiv.innerHTML = "<pre>Server response:\n" + formattedJson + "</pre>";
        })
        .catch(error => {
            console.error("Error:", error);
			var statusResultDiv = document.getElementById("resultDiv");
			statusResultDiv.textContent = "Error fetching server status: " + error.message;
        });
}

function updateSliderFromTextbox() {
	var sliderValue = document.getElementById("sliderValueTextbox").value;
	document.getElementById("ledSlider").value = sliderValue;
	updateSliderValue(sliderValue);
}

function sendLedRequest() {
	var ipAddress = document.getElementById("ipAddress").value;
	var ledValue = document.getElementById("ledSlider").value;
	var url = "http://" + ipAddress + "/led?params=" + ledValue;

	fetch(url)
        .then(response => response.json())
        .then(result => {
			// Formatez la réponse JSON de manière jolie
            var formattedJson = JSON.stringify(result, null, 2); // Le deuxième argument spécifie la mise en forme
            var statusResultDiv = document.getElementById("resultDiv");
            statusResultDiv.innerHTML = "<pre>Server response:\n" + formattedJson + "</pre>";
        })
        .catch(error => {
            console.error("Error:", error);
			var statusResultDiv = document.getElementById("resultDiv");
			statusResultDiv.textContent = "Error fetching server status: " + error.message;
        });

}

// Pour sauvegarder et récupérer l'adresse IP depuis le localStorage

// Pour sauvegarder l'adresse IP
function saveIpAddressToLocalStorage() {
	localStorage.setItem("ipAddress", document.getElementById("ipAddress").value);
}

// Pour récupérer l'adresse IP lors du chargement de la page
function loadIpAddressFromLocalStorage() {
	var savedIpAddress = localStorage.getItem("ipAddress");
	if (savedIpAddress) {
		document.getElementById("ipAddress").value = savedIpAddress;
	}
}

 // Pour afficher la valeur du slider
function updateSliderValue(value) {
	document.getElementById("currentSliderValue").textContent = value;
}

// Pour effacer le contenu du champ "customParams"
function clearCustomParams() {
	document.getElementById("customParams").value = "";
}

function sendManualCommand() {
    var ipAddress = document.getElementById("ipAddress").value;
    var commandParams = document.getElementById("manualCommandParams").value;

    if (!ipAddress || !commandParams) {
        alert("Please enter both IP Address and Command Params.");
        return;
    }

    var url = "http://" + ipAddress + "/cmd?params=" + encodeURIComponent(commandParams);


    fetch(url)
        .then(response => response.json())
        .then(result => {
			// Formatez la réponse JSON de manière jolie
            var formattedJson = JSON.stringify(result, null, 2); // Le deuxième argument spécifie la mise en forme
            var statusResultDiv = document.getElementById("resultDiv");
            statusResultDiv.innerHTML = "<pre>Server response:\n" + formattedJson + "</pre>";
        })
        .catch(error => {
            console.error("Error:", error);
			var statusResultDiv = document.getElementById("resultDiv");
			statusResultDiv.textContent = "Error fetching server status: " + error.message;
        });
}

function getStatus() {
    var ipAddress = document.getElementById("ipAddress").value; // Récupérez l'adresse IP du champ
    var statusUrl = "http://" + ipAddress + "/"; 

    fetch(statusUrl)
        .then(response => response.json())
        .then(data => {
            // Formatez la réponse JSON de manière jolie
            var formattedJson = JSON.stringify(data, null, 2); // Le deuxième argument spécifie la mise en forme
            var statusResultDiv = document.getElementById("statusResult");
            statusResultDiv.innerHTML = "<pre>Server Status:\n" + formattedJson + "</pre>";
        })
        .catch(error => {
            console.error("Error:", error);
            var statusResultDiv = document.getElementById("statusResult");
            statusResultDiv.textContent = "Error fetching server status: " + error.message;
        });
}