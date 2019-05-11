//NOTE: MUST INSTALL EventSource
//in Terminal enter the following: npm install eventsource

// required when running on node.js
var mqtt = require('mqtt');

var wuhoLat = 34.101369;
var wuhoLong = -118.331762;
var anitpodLat = -34.101364;
var antipodLong = 61.668213;
var NorthLatitude = 90;
var SouthLatitude = -90;
var WestLongitude = -180;
var EastLongitude = 180;

var lastUser = "";

var counter = 0;
var j = 0;

var client = mqtt.connect('mqtt://288b70b1:12b466c6da113704@broker.shiftr.io', {
  clientId: 'javascript'
});

client.on('connect', function(){
    console.log('client has connected!');

    client.subscribe('/WEST');
    client.subscribe('/EAST');    
    
    var EventSource = require('eventsource');
    var url = 'https://stream.wikimedia.org/v2/stream/recentchange';

    console.log(`Connecting to EventStreams at ${url}`);
    var eventSource = new EventSource(url);

    eventSource.onopen = function(event) {
        console.log('--- Opened connection.');
    };

    eventSource.onerror = function(event) {
        console.error('--- Encountered error', event);
    };

    eventSource.onmessage = function(event) {
            // event.data will be a JSON string containing the message event.
            var JSONparsed = JSON.parse(event.data);

            var http = require('http');

            //bot:true/false,
            var bot = JSONparsed.bot; 
            //"length":{"new":2763,"old":2707}, //count including white space
            //"revision":{"new":83793403,"old":82916165} //edit IDs
            //"type":"edit", categorize, log
            var user = JSONparsed.user;
            var type = JSONparsed.type;
            //var title = JSONparsed.title;
            var type = JSONparsed.type;
            var usersplit = user.split(/[\s:.]+/);
            var words = JSONparsed.length;
            var title = JSONparsed.title;
            var server = JSONparsed.server_name;
            var serversplit = server.split(".");;
            var language = serversplit[0];
		
            if (!isNaN(usersplit[0]) && lastUser != user)
            {        	
                counter += 1;
                //console.log("COUNTER: " + j + ", " + counter + ", " + user);
                lastUser = user;
                http.get('http://freegeoip.net/json/' + user, function (res) {
                    var body = '';

                    res.on('data', function (data) {
                        body += data;
                    });

                    res.on('end', function () {

                        try { 

                            var direction = 0;
                            var parsed = JSON.parse(body);
                            var parsedCountry = parsed.country_code;
                            var parsedLat = parsed.latitude;                       
                            var parsedLong = parsed.longitude;
                            var parsedCity = parsed.city;
                            var newWords = words.new;
                            var oldWords = words.old;
                            var difWords = newWords - oldWords;

                            //LA
                            if (parsedCity == "Los Angeles" || parsedCity == "Anaheim" || parsedCity == "Corona" || parsedCity == "Rosemead") {
                            	parsedCity = 'Los Angeles';
                                var outputLA = 'CITY: ' + parsedCity + ', TYPE: ' + type + ', CHANGE: ' + difWords + ', COMPASS: ' + direction + ', LANGUAGE: ' + language + ', END:';
                                //console.log('');
                                //console.log('new message: /DATA WALTZ >>> TITLE: ' + title);
                                client.publish('/WEST', outputLA);
                                client.publish('/EAST', outputLA);
                            }
                                //NOT LA
                            else
                            {                                                
                                //CALC DIRECTION
                                //http://stackoverflow.com/questions/3932502/calculate-angle-between-two-latitude-longitude-points
                                //θ = atan2(sin(Δlong)*cos(lat2), cos(lat1)*sin(lat2) − sin(lat1)*cos(lat2)*cos(Δlong))
                                //Note that the angle(θ) should be converted to radians before using this formula and Δlong = long2 - long1.
                                //convert each decimal latitude and longitude into radians by multiplying each one by PI/180
                                //http://www.geomidpoint.com/example.html
                        	
                                var lat1 = wuhoLat * Math.PI/180.0;
                                var long1 = wuhoLong * Math.PI/180.0;
                                var lat2 = parsedLat * Math.PI/180.0;
                                var long2 = parsedLong * Math.PI/180.0;

                                var dLon = (long2 - long1);

                                var y = Math.sin(dLon) * Math.cos(lat2);
                                var x = Math.cos(lat1) * Math.sin(lat2) - Math.sin(lat1) * Math.cos(lat2) * Math.cos(dLon);

                                var brng = Math.atan2(y, x);
                                direction = brng * (180 / Math.PI);
                                //direction = 180 - direction; //REVERSE DIRECTION SO THAT 0 IS SOUTH                                
							
                                                        
						
                                //CLIENT SIGNAL
                                //WEST
                                if (direction <= 0) {
                                    var directionW = direction * -1;
                                    directionW = 180 - directionW; //REVERSE DIRECTION SO THAT 0 IS SOUTH
                                    var number = Math.min(Math.max(parseInt(directionW), 0), 180); //FIX
                                    //console.log('new message: /DATA WALTZ >>> TITLE: ' + title);
                                    var output = 'CITY: ' + parsedCity + ', TYPE: ' + type + ', CHANGE: ' + difWords + ', COMPASS: ' + number + ', LANGUAGE: ' + language + ', END:';                        	 
                                    //console.log('');
                                    client.publish('/WEST', output);
                                }
                                //EAST
                                else if (direction > 0) {
                                	//console.log('');
                                	//console.log('new message: /DATA WALTZ >>> TITLE: ' + '' + title);
                                	direction = 180 - direction; //REVERSE DIRECTION SO THAT 0 IS SOUTH
                                	var number = Math.min(Math.max(parseInt(direction), 0), 180); //FIX
                                	var output = 'CITY: ' + parsedCity + ', TYPE: ' + type + ', CHANGE: ' + difWords + ', COMPASS: ' + number + ', LANGUAGE: ' + language + ', END:';
                                    client.publish('/EAST', output);
                                    
                                }

                            }
                                           
                        }
                        catch (err) {
                        }

                    });
                });
            }
        
        };

});

client.on('message', function (topic, message)
{
  console.log('new message:', topic, message.toString());
});