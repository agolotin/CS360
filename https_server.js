var fs = require('fs');
var http = require('http');
var https = require('https');
var express = require('express');
var bodyParser = require('body-parser');
var url = require('url');
var app = express();
app.use('/', express.static('./html', { maxAge: 60*60*1000})); //gets any static files
app.use(bodyParser());
var basicAuth = require('basic-auth-connect');
var auth = basicAuth(function(user, pass) {
	return ((user === 'cs360') && (pass === 'test'));
});
var options = {
	host: '127.0.0.1',
	key: fs.readFileSync('keys/server.key'),
	cert: fs.readFileSync('keys/server.crt')
};

http.createServer(app).listen(80);
https.createServer(options, app).listen(443);
app.get('/', function(req, res) {
	res.send("There's nothing here to show, my apologies");
});

app.get('/getcity', function(req, res) {
	console.log("In GET getcity route");
	fs.readFile("cities.dat.txt", function(err,data) {
		if (err) {
			res.writeHead(404);
			res.end(JSON.stringify(err));
			return;
		}
		var cities = data.toString().split("\n");
  		var urlObj = url.parse(req.url, true, false);
		var regEx = new RegExp("^"+urlObj.query["q"]);
		var jsonresult = [];
		for (var i = 0; i < cities.length; i++) {
			var result = cities[i].search(regEx);
			if (result != -1) {
				jsonresult.push({city:cities[i]});
			}
		}
		res.status(200);
		res.end(JSON.stringify(jsonresult));
	});
});

app.get('/comment', function(req, res) {
	console.log("In GET comment route");
	var MongoClient = require('mongodb').MongoClient;
	MongoClient.connect("mongodb://localhost/comments_db", function(err, db) {
		if (err) throw err;
		db.collection("comments", function(err, comments) {
			if (err) throw err;
			comments.find(function(err, items) {
				items.toArray(function(err, itemArr) {
					res.json(itemArr);
					res.status(200);
					res.end();
				});
			});
		});
	});
});

app.post('/comment', auth, function(req, res) {
	console.log("In POST comment route");
	console.log(req.body);
	var reqObj = req.body;
	var MongoClient = require('mongodb').MongoClient;
	MongoClient.connect("mongodb://localhost/comments_db", function(err, db) {
		if (err) throw err;
		db.collection("comments").insert(reqObj, function(err, records) {
			console.log("Record successfully added as " + records[0]._id);
		});
		res.status(200);
		res.end();
	});
});
