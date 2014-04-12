/*
__________________________________________

 			 PLANTS & MACHINES
            early alpha v-0.0.1

              Martin Breuer
              * 
                  GPLv2

__________________________________________

/*


// Includes 

var mongoose = require('mongoose');
mongoose.connect('mongodb://localhost/test');

var SerialPort  = require('serialport2').SerialPort;
var portName = '/dev/ttyACM0';
var http = require('http');
var fs = require('fs');
var path = require('path');
var faye = require('faye');


var Schema = mongoose.Schema
  , ObjectId = Schema.ObjectId;

var ArduinoDataSchema = new Schema({
    msg    : ObjectId
  , temp   : Number
  , humid  : Number
  , water  : Boolean   
  , distance : Number
  , lastFlood : Number
  //, time : { type : Date, default : Date.now} 
});
mongoose.model('ArduinoData', ArduinoDataSchema )

var ArduinoDataModel = undefined;
var db = mongoose.connection;
db.on('error', console.error.bind(console, 'connection error:'));
db.once('open', function callback () {
  console.log("hail all database conection")

  ArduinoDataModel = mongoose.model('ArduinoData');
});


// Creating web server serving http and lib files

var httpserver = http.createServer(function (request, response) {
     
    var filePath = '.' + request.url;
    if (filePath == './')
        filePath = './index.html';
         
    var extname = path.extname(filePath);
    var contentType = 'text/html';
    switch (extname) {
        case '.js':
            contentType = 'text/javascript';
            break;
        case '.css':
            contentType = 'text/css';
            break;
    }
     
    path.exists(filePath, function(exists) {
     
        if (exists) {
            fs.readFile(filePath, function(error, content) {
                if (error) {
                    response.writeHead(500);
                    response.end();
                }
                else {
                    response.writeHead(200, { 'Content-Type': contentType });
                    response.end(content, 'utf-8');
                }
            });
        }
        else {
            response.writeHead(404);
            response.end();
        }
    });
     
})

httpserver.listen(80);
 
var bayeux = new faye.NodeAdapter({mount: '/faye', timeout: 45});

bayeux.attach(httpserver);


// Setting up Serial Port

var sp = new SerialPort(); // instantiate the serial port.
sp.open(portName, { // portName is instatiated to be COM3, replace as necessary
   baudRate: 9600, // this is synced to what was set for the Arduino Code
   dataBits: 8, // this is the default for Arduino serial communication
   parity: 'none', // this is the default for Arduino serial communication
   stopBits: 1, // this is the default for Arduino serial communication
   flowControl: false // this is the default for Arduino serial communication
});


// Sending sensor feed to a socket

var tempData = ''; // this stores the temperature data
var humiData = ''; // stores air humidity data
var lastFlood = ''; // stores the time in minutes since last flood
var distData = ''; // this stores the distance data

// var qualData = ''; // air quality data 
// var watrData = ''; // water detection data
// var lighData = ''; // light detection data

var readData = ''; // this stores the buffer


sp.on('data', function (data) { // call back when data is received
    readData += data.toString();
    json = readData.match(/{.+}/);
    if ( json === null || ArduinoDataModel === undefined ){
      return;
    };

    json = JSON.parse(json[0]);
    console.log(json);

    readData = '';
    var ArduinoData = new ArduinoDataModel(json);
    
    bayeux.getClient().publish('/data/live',  json  );

    ArduinoData.save(function(err, data){
      if (err) // handle the error
       console.log("no save bo more!!!");
    });

    // var instance = ArduinoDataModel.find({water : 1 }, function(err, doc) { console.log(doc); });
   
    readData = '';
        // for debug
        // console.log(tempData);
        // console.log(humiData);
    });

