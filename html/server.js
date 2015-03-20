var fs = require('fs');
var http = require('http');
var url = require('url');
var ROOT_DIR = "html/";

http.createServer(function(req, res) {
  	var urlObj = url.parse(req.url, true, false);
	if (urlObj.pathname.indexOf("getcity") != -1) {
		fs.readFile("cities.dat.txt", function(err,data) {
			if (err) {
				res.writeHead(404);
				res.end(JSON.stringify(err));
				return;
			}
			var cities = data.toString().split("\n");
			var regEx = new RegExp("^"+urlObj.query["q"]);
			var jsonresult = [];
			for (var i = 0; i < cities.length; i++) {
				var result = cities[i].search(regEx);
				if (result != -1) {
					jsonresult.push({city:cities[i]});
				}
			}
			res.writeHead(200);
			res.end(JSON.stringify(jsonresult));
		});
	}	
	else if (urlObj.pathname == "/comment")
	{
		if (req.method == "POST")
		{
			console.log("POST comment route");
			var jsonData = "";
			req.on('data', function(chunk) {
				jsonData += chunk;
			});
			req.on('end', function() {
				var reqObj = JSON.parse(jsonData);
				var MongoClient = require('mongodb').MongoClient;
				MongoClient.connect("mongodb://localhost/comments_db", function(err, db) {
					if (err) throw err;
					db.collection('comments').insert(reqObj, function(err, records) {
						console.log("Record added as " + records[0]._id);
					});
				});

				console.log(reqObj);
				console.log("Name:" + reqObj.Name);
				console.log("Comment: " + reqObj.Comment);
			});
			res.writeHead(200);
			res.end("");
		}
		else if (req.method == "GET")
		{
			console.log("GET comment route");
			var MongoClient = require('mongodb').MongoClient;
			MongoClient.connect("mongodb://localhost/comments_db", function(err, db) {
				if (err) throw err;
				db.collection("comments", function(err, comments) {
					if (err) throw err;
					comments.find(function(err, items) {
						items.toArray(function(err, itemArr) {
							console.log("Document Array: ");
							console.log(itemArr);
							res.writeHead(200);
							res.end(JSON.stringify(itemArr));
						});
					});
				});
			});
		}
	}
	else 
	{
		fs.readFile(ROOT_DIR + urlObj.pathname, function (err,data) {
		if (err) {
			res.writeHead(404);
			res.end(JSON.stringify(err));
			return;
		}
		res.writeHead(200);
		res.end(data);
		});
	}
}).listen(80);
