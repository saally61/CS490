var http = require('http');
var url = require('url');
var request = require("request")

 http.createServer(function (req, response) {
     response.writeHead(200, {
         'Content-Type': 'text/html'
     });
     response.write("Updated dashboard refresh to continue updating");
     //request data from thinkspeak
     request({
         url: 'https://api.thingspeak.com/channels/579448/feeds.json?results=2',
         json: true
     }, function (error, response, body) {

         if (!error && response.statusCode === 200) {
             console.log(body.feeds[0]) // Print the json response
             var item = body.feeds[0];
             send_data(item);
         }
     })

     response.end();

}).listen(80);


function send_data(item) {
    var key = "28f661bd8c1b493ca5a76ca410b53367";
    if(item == "none"){
        return;
    }
    var values = "&temp1=" + item.field1+
        "&pres=" + item.field2 +
        "&alt=" + item.field3 +
        "&light=" + item.field4 +
        "&hum=" + item.field5 +
        "&uv=" + item.field6 +
        "&dust=" + item.field7+
        "&hr="+ item.field8;
    console.log(values);
    var url = "http://io.adafruit.com/api/groups/Lab1/send.json?x-aio-key=" + key + values;
    http.get(url, function (res) {
        console.log(res.statusCode);
        console.log(res.headers);
        res.setEncoding('utf8');
        res.on("data", function (chunk) {
            //display data
            console.log(chunk);
        });
    }).on('error', function (e) {
        console.log("Got error: " + e.message);
    });
}

