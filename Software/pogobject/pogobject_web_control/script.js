// Récupération de l'adresse IP et stockage de l'IP :
function update_ip_address() {
    // On récupère l'adresse IP depuis le stockage local (localStorage)
    let ip = localStorage.getItem("ip");
    // On met à jour la valeur de l'élément HTML avec l'ID "ip"
    document.getElementById("ip").value = ip;
}

// Construction de l'URL de base :
function url_construction() {
    // On récupère l'adresse IP saisie par l'utilisateur depuis l'élément HTML avec l'ID "ip"
    let input = document.getElementById("ip").value;
    // On stocker cette adresse IP dans le stockage local
    localStorage.setItem("ip", input);
    // On construit l'URL en ajoutant "http://" au début de l'adresse IP
    var url = "http://" + input;
    // Retourner l'URL construite
    return url;
}

// Appel au click du button "Status"
function status() {
    // Appel de la fonction de construction de l'URL de base, qu'on implémente dans la variable "url"
    var url = url_construction();
    // Envoie de la commande, gestion du retour et gestion des erreurs
    fetch(url).then((response) => {
        if (!response.ok) {
            // Si la réponse n'est pas OK, lancer une erreur avec le code HTTP
            throw new Error(`Erreur HTTP : ${response.status}`);
        }
        return response.json();
    }).then((json) => {
        // On récupère l'ID et le Nom du pogobot
        var id = json.id;
        var name = json.name;
        // On les affiche dans la div "infos"
        document.getElementById("infos").innerText = "ID : " + id + " \n Name : " + name;
        // On convertit l'objet JavaScript en chaîne
        let formattedJson = JSON.stringify(json, null, 4);
        // On affiche le résultat dans la div "reponse" avec la balise <pre> pour conserver les espaces
        document.getElementById("reponse").innerHTML = "<pre>" + formattedJson + "</pre>";
        // On enlève les bordures ajoutées en cas d'erreur
        document.getElementById("reponse-div").style = "border: none;";
    }).catch((error) => {
        // En cas d'erreur, on affiche un message d'erreur encadré en rouge
        document.getElementById("reponse").textContent = `Erreur lors de la récupération du vers : ${error}`;
        document.getElementById("reponse-div").style = "border: 2px solid #e60606 !important;";
    });
}

// Appel au clic du bouton "Send Command"
function command() {
    // On récupère l'URL de base
    var url = url_construction();
    // On récupère la commande depuis l'élément HTML avec l'ID "cmd"
    var cmd = document.getElementById("cmd").value;
    // On construit l'URL complète avec la commande en ajoutant "/cmd?params=" à l'URL de base
    var url_cmd = url + "/cmd?params=" + cmd;
    // Envoie de la commande, gestion du retour et gestion des erreurs
    fetch(url_cmd).then((response) => {
        if (!response.ok) {
            // Si la réponse n'est pas OK, lancer une erreur avec le code HTTP
            throw new Error(`Erreur HTTP : ${response.status}`);
        }
        return response.json();
    }).then((json) => {
        // On récupère l'ID et le Nom du pogobot
        var id = json.id;
        var name = json.name;
        // On affiche l'ID et le Nom dans la div "infos"
        document.getElementById("infos").innerText = "ID : " + id + " \n Name : " + name;
        // On convertit l'objet JavaScript en chaîne
        let formattedJson = JSON.stringify(json, null, 4);
        // On affiche le résultat dans la div "reponse" avec la balise <pre> pour conserver les espaces
        document.getElementById("reponse").innerHTML = "<pre>" + formattedJson + "</pre>";
        // On enlève les bordures ajoutées en cas d'erreur
        document.getElementById("reponse-div").style = "border: none;";
    }).catch((error) => {
        // En cas d'erreur, on affiche un message d'erreur encadré en rouge
        document.getElementById("reponse").textContent = `Erreur lors de la récupération du vers : ${error}`;
        document.getElementById("reponse-div").style = "border: 2px solid #e60606 !important;";
    });
}

// Appel au click du button "Send led control"
function led_control(){
    // On récupère l'URL de base
    var url=url_construction();
    // On récupère la commande depuis l'élément HTML avec l'ID "led_value"
    var led=document.getElementById("led_value");
    // On construit l'URL complète avec la commande en ajoutant "/led?params=" à l'URL de base
    var url_led=url+"/led?params="+led.value;
    // Envoie de la commande, gestion du retour et gestion des erreurs
    fetch(url_led).then((response) => {
        if (!response.ok) {
            // Si la réponse n'est pas OK, lancer une erreur avec le code HTTP
            throw new Error(`Erreur HTTP : ${response.status}`);
        }
        return response.json();
    }).then((json) => {
        // On récupère l'ID et le Nom du pogobot
        var id = json.id;
        var name = json.name;
        // On affiche l'ID et le Nom dans la div "infos"
        document.getElementById("infos").innerText = "ID : "+ id + " \n Name : " + name;
        // On convertit l'objet JavaScript en chaîne
        let formattedJson = JSON.stringify(json,null,4);
        // On affiche le résultat dans la div "reponse" avec la balise <pre> pour conserver les espaces
        document.getElementById("reponse").innerHTML = "<pre>" + formattedJson + "</pre>";
        // On enlève les bordures ajoutées en cas d'erreur
        document.getElementById("reponse-div").style = "border : none;"  ;
    }).catch((error) => {
        // En cas d'erreur, on affiche un message d'erreur encadré en rouge
        document.getElementById("reponse").textContent = `Erreur lors de la récupération du vers : ${error}`;
        document.getElementById("reponse-div").style = "border : 2px solid #e60606 !important;"  ;
    });
}

// Appel au click du button "Reboot pogobot"
function reboot(){
    // On récupère l'URL de base
    var url=url_construction();
    var url_reboot=url+"/reboot_p";
    
    // Envoie de la commande, gestion du retour et gestion des erreurs
    fetch(url_reboot).then((response) => {
        if (!response.ok) {
            // Si la réponse n'est pas OK, lancer une erreur avec le code HTTP
            throw new Error(`Erreur HTTP : ${response.status}`);
        }
        return response.json();
    }).then((json) => {
        // On récupère l'ID et le Nom du pogobot
        var id = json.id;
        var name = json.name;
        // On affiche l'ID et le Nom dans la div "infos"
        document.getElementById("infos").innerText = "ID : "+ id + " \n Name : " + name;
        // On convertit l'objet JavaScript en chaîne
        let formattedJson = JSON.stringify(json,null,4);
        // On affiche le résultat dans la div "reponse" avec la balise <pre> pour conserver les espaces
        document.getElementById("reponse").innerHTML = "<pre>" + formattedJson + "</pre>";
        // On enlève les bordures ajoutées en cas d'erreur
        document.getElementById("reponse-div").style = "border : none;"  ;
    }).catch((error) => {
        // En cas d'erreur, on affiche un message d'erreur encadré en rouge
        document.getElementById("reponse").textContent = `Erreur lors de la récupération du vers : ${error}`;
        document.getElementById("reponse-div").style = "border : 2px solid #e60606 !important;"  ;
    });
}

// Appel au click du button "Update_bat"
function update_bat(){
    // On récupère l'URL de base
    var url=url_construction();
    var url_update=url+"/update_bat";

    // Envoie de la commande, gestion du retour et gestion des erreurs
    fetch(url_update).then((response) => {
        if (!response.ok) {
            // Si la réponse n'est pas OK, lancer une erreur avec le code HTTP
            throw new Error(`Erreur HTTP : ${response.status}`);
        }
        return response.json();
    }).then((json) => {
        // On récupère la valeur de la batterie restante :
        var value = json.variables.bat_percentage;
        colors=bat(value);
        // On affiche la valeur dans la div "bat_value" :
        document.getElementById("bat_value").textContent = value+"%";
        // On change la couleur de la valeur en fonction de la valeur de couleur retourné par la fonction bat(value) : 
        document.getElementById("bat_value").style.color=colors;
        // On récupère l'ID et le Nom du pogobot
        var id = json.id;
        var name = json.name;
        // On affiche l'ID et le Nom dans la div "infos"
        document.getElementById("infos").innerText = "ID : "+ id + " \n Name : " + name;
        // On convertit l'objet JavaScript en chaîne
        let formattedJson = JSON.stringify(json,null,4);
        // On affiche le résultat dans la div "reponse" avec la balise <pre> pour conserver les espaces
        document.getElementById("reponse").innerHTML = "<pre>" + formattedJson + "</pre>";
        // On enlève les bordures ajoutées en cas d'erreur
        document.getElementById("reponse-div").style = "border : none;"  ;
    }).catch((error) => {
        // En cas d'erreur, on affiche un message d'erreur encadré en rouge
        document.getElementById("reponse").textContent = `Erreur lors de la récupération du vers : ${error}`;
        document.getElementById("reponse-div").style = "border : 2px solid #e60606 !important;"  ;
    });
}

// On verifie la valeur rentré en paramètre.  
function bat(e){
    // Si elle est inférieur ou égale à 10, alors sa couleur associé est rouge
    if (e<=10)
    var colors = "#FF2D00";
    // Si elle est inférieur ou égale à 50, sa couleur associé est orange
    else if (e<=50)
    var colors = "#FF8F00";
    // Sinon, sa couleur associé est vert
    else
    var colors = "#00C403";
    return colors;
}

// On syncronise l'input number et l'input range
function sync_range() {
    // On récupère l'input range
    let range=document.getElementById("led_range");
    // On récupère l'input number
    let number=document.getElementById("led_value");
    // On remplace la valeur de la range par la valeur de number
    range.value = number.value;
    // On appelle la function slider avec comme paramètre range
    slider(range);
}

function sync_number() {
    // On récupère l'input range
    let range=document.getElementById("led_range");
    // On récupère l'input number
    let number=document.getElementById("led_value");
    // On remplace la valeur de number par la valeur de range
    number.value = range.value;
    // On appelle la function slider avec comme paramètre range
    slider(range);
}


// Permet de réaliser le changement de couleur d'un slider
// Prend en entrée l'objet slider
function slider( slider ){

    // On mappe la valeur du slider par rapport à la valeur max de la LED (1024)
    x = slider.value*100/1024;
    
    // Modification du style du slider
    color = 'linear-gradient(90deg, rgb(117,214,117)' + x + '%,rgb(214,214,214) ' + x + '%)';
    slider.style.background = color;
        
}