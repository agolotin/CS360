#!/usr/bin/perl
#
# CGI script to print out the relevant environment variables.
# It's just one big print statement, but note the use of the
# associative %ENV array to access the environment variables.
#

print "Content-type: text/html

<head>
	<title> CGI Tutorial: Environment variables script</title>
</head>
<body>


<hr>
<h1> Environment variables script</h1>
<hr>

<p>

Here are the environment variables that this CGI script has been
called with.

<p>
<hr>

<pre>
SERVER_SOFTWARE = $ENV{'SERVER_SOFTWARE'}
SERVER_NAME = $ENV{'SERVER_NAME'}
GATEWAY_INTERFACE = $ENV{'GATEWAY_INTERFACE'}
SERVER_PROTOCOL = $ENV{'SERVER_PROTOCOL'}
SERVER_PORT = $ENV{'SERVER_PORT'}
REQUEST_METHOD = $ENV{'REQUEST_METHOD'}
HTTP_ACCEPT = '$ENV{'HTTP_ACCEPT'}'
PATH_INFO = $ENV{'PATH_INFO'}
PATH_TRANSLATED = $ENV{'PATH_TRANSLATED'}
SCRIPT_NAME = $ENV{'SCRIPT_NAME'}
QUERY_STRING = $ENV{'QUERY_STRING'}
REMOTE_HOST = $ENV{'REMOTE_HOST'}
REMOTE_ADDR = $ENV{'REMOTE_ADDR'}
REMOTE_USER = $ENV{'REMOTE_USER'}
CONTENT_TYPE = $ENV{'CONTENT_TYPE'}
CONTENT_LENGTH = $ENV{'CONTENT_LENGTH'}
</pre>

<hr>
</body>";			# Print statement (and program) ends here
