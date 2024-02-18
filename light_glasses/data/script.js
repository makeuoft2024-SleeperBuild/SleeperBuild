const url = "ws://%IP_ADDR%:%PORT%/";
let luxBufferSize = 50;

let xValues = Array.from(new Array(luxBufferSize), (x, i) => i);
let yValues = Array.from(new Array(luxBufferSize), (x, i) => 0);
let brightness = Array.from(new Array(luxBufferSize), (x, i) => 0);

let canvas = document.getElementById("myChart"); 
let ctx = canvas.getContext("2d");
let gradient = ctx.createLinearGradient(0, 0, 0, 800);
let gradientGreen = ctx.createLinearGradient(0, 0, 0, 800);

let slider = document.getElementById("myRange");
let output = document.getElementById("target");
let acc = document.getElementById("acc");

const chart = new Chart("myChart", {
	type: "line",
	data: {
		labels: xValues,
		datasets: [{
			backgroundColor: gradient,
			borderColor: gradient,
			data: yValues
		}]
	},
	options: {
		elements: {
			point:{
				radius: 0
			}
		},
		legend: {
			display: false
		},
		scales: {
			xAxes: [{
				display: false
			}],
			yAxes: [{
				display: true
			}]
		},
		responsive: true,
		maintainAspectRatio: false,
		title: {
			display: true,
			text: 'Lux vs Seconds',
			color: '#000',
			font: {
				family: 'Courier New',
				size: 20
			}
		}
	}
});

const brightnessChart = new Chart("brightnessChart", {
	type: "line",
	data: {
		labels: xValues,
		datasets: [{
			backgroundColor: gradientGreen,
			borderColor: gradientGreen,
			data: brightness
		}]
	},
	options: {
		elements: {
			point:{
				radius: 0
			}
		},
		legend: {
			display: false
		},
		scales: {
			xAxes: [{
				display: false
			}],
			yAxes: [{
				display: true
			}]
		},
		responsive: true,
		maintainAspectRatio: false,
		title: {
			display: true,
			text: 'Accumulated Lux vs Seconds',
			color: '#0',
			font: {
				family: 'Courier New',
			},
		}
		
	}
});

function init() {
	gradient.addColorStop(0, 'rgba(255, 175, 56, 1)');
	gradient.addColorStop(0.9, 'rgba(181, 82, 5, 0)');
	gradientGreen.addColorStop(0, 'rgba(175, 255, 56, 1)');
	gradientGreen.addColorStop(0.9, 'rgba(82, 181, 5, 0)');

	
	output.innerHTML = slider.value; // Display the default slider value

	// Update the current slider value (each time you drag the slider handle)
	slider.oninput = function() {
		output.innerHTML = this.value;
	}

	slider.onchange = function() {
		doSend(`bl ${this.value}`);
	}

	// Connect to WebSocket server
	websocket = new WebSocket(url);
	websocket.onopen = function(evt) { onOpen(evt) };
	websocket.onclose = function(evt) { onClose(evt) };
	websocket.onmessage = function(evt) { onMessage(evt) };
	websocket.onerror = function(evt) { onError(evt) };
}


// Called when a WebSocket connection is established with the server
function onOpen(evt) {
    console.log("Connected");
}

// Called when the WebSocket connection is closed
function onClose(evt) {
    console.log("Disconnected");
    // Try to reconnect after a few seconds
    setTimeout(function() { wsConnect(url) }, 2000);
}

// Called when a message is received from the server
function onMessage(evt) {
    console.log("Received: " + evt.data);
    const data = evt.data.split(" ");
    if (data[0] == "lux") {
		console.log(data[1]);
		chart.data.labels.shift();
		chart.data.labels.push(luxBufferSize);
		chart.data.datasets[0].data.shift();
		chart.data.datasets[0].data.push(data[1]);
		chart.update();

		brightnessChart.data.labels.shift();
		brightnessChart.data.labels.push(luxBufferSize);
		brightnessChart.data.datasets[0].data.shift();
		brightnessChart.data.datasets[0].data.push(data[2]);
		brightnessChart.update();
		acc.innerHTML = data[2];
		luxBufferSize+=1;
	}
}

// Called when a WebSocket error occurs
function onError(evt) {
    console.log("ERROR: " + evt.data);
}

// Sends a message to the server (and prints it to the console)
function doSend(message) {
    console.log("Sending: " + message);
    websocket.send(message);
}

// Called whenever the HTML button is pressed
function onPress() {
    doSend("lux");
}

window.addEventListener("load", init, false);