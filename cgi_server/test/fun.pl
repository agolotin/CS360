<!DOCTYPE html
	PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN"
	 "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html xmlns="http://www.w3.org/1999/xhtml" lang="en-US" xml:lang="en-US">
<head>
<title>Simple Script</title>
<meta http-equiv="Content-Type" content="text/html; charset=iso-8859-1" />
</head>
<body>
<h1>Simple Script</h1><form method="post" action="/~agolotin/CS360/cgi_server/test/fun.cgi" enctype="multipart/form-data">
What's your name? <input type="text" name="name"  /><p />What's the combination?<label><input type="checkbox" name="words" value="eenie" checked="checked" />eenie</label><label><input type="checkbox" name="words" value="meenie" />meenie</label><label><input type="checkbox" name="words" value="minie" />minie</label><label><input type="checkbox" name="words" value="moe" checked="checked" />moe</label><p />What's your favorite color?<select name="color" >
<option value="red">red</option>
<option value="green">green</option>
<option value="blue">blue</option>
<option value="chartreuse">chartreuse</option>
</select><p /><input type="submit" name=".submit" /><div><input type="hidden" name=".cgifields" value="words"  /></div></form><hr />

</body>
</html>
