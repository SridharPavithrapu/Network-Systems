CSCI-ECEN​ ​4273-5273
Programming​ ​Assignment-2
HTTP-based web server

Submitted​ ​by:
Sridhar​ ​Pavithrapu

I​ ​have​ ​implemented​ ​HTTP web-server​ ​for​ ​transmitting​ ​information​ ​of requested web pages from a​ ​client(web-browser/telnet).

Here,​ ​I​ ​have​ ​created​ folder named​ ​‘​ ​server​​ ’.

The​ ​‘​ ​server ​’ folder  ​consists​ ​of​ ​‘​ ​Makefile​ ​’​ ​,‘​ ​server.c​ ​’​ , 'ws_conf' ​and​ 'root folder' for sending requested contents to web-browser.

Server:
In​ ​this​ ​assignment,​ ​I​ ​have​ ​written​ ​server​ ​which​ ​has​ ​below​ ​interfaces​ ​and​ ​their functionality:

send_notFoundError: This interface is used to send the HTTP error:404, i.e., file requested is not found.

send_fileInformation: This interface first checks whether the requested file from the web-browser is present in the 'root folder'. 
				  If the file is present, then sends the file information, otherwise sends the error message to web-browser.

send_badRequestError: This interface is used to send the HTTP error:400, i.e., bad request method, URL or HTTP version.

send_notImplementedError: This interface is used to send the HTTP error:501, i.e., the requested file is not implemented/present.

send_postFileInformation: This interface first checks whether the requested file from the web-browser is present in the 'root folder'.
					 If the file is present, then sends the file information along with the POST data with <pre> header, otherwise sends the error message to web-browser.

service_request: This interface receives the request from web-browser, then the request is parsed to check whether it has the valid HTTP protocol, URI and requested method.
		       If the above query satisfies, then the web-server sends the corresponding valid response, otherwise it sends error message. 
			  It also executes the POST request from the web-browser and also supports pipelining.

parse_config: This interface is used to parse the 'ws_conf' file.

signal_callback_handler: This interface is a signal callback handler, which recieves the "ctrl+c" requests and gracefully exits the web-server, by closing all required connections created.

main: This interface creates the socket for TCP connection, and creates child processes(using 'fork') for any connection from the web-browser and this child process services the requests
	 received from web-browser.


Extra-Credit:

Pipelining:

I have implemented pipelining functionality by keeping 'time-out', whose value is read from 'ws_conf' file.
Then, once the request is processed from web-browser, If the connection is 'Keep-alive', the timer will be run using 'select' interface, to check for any further requests.

If there is any request within the time-out, then the corresponsing request is processed. This process continues till,
no further requests are received within the time-out or if the connection type is other than 'Keep-alive'

POST method:

I have implemented POST method, and created a new HTML page named 'post.html' which is present in the 'root' folder.
This post.html file has a form which takes first-name and last-name as input, when submit button is pressed, it sends a post request to webserver.
Upon receiving this post request, the web-server parses the content received, and sends the response with POST data with <pre> header in the HTML page along with the post.html content.

Executing​ ​Instructions:

The​ ​'server' folder​ ​consists​ ​of​ ​makefile.​ ​The​ ​server​ ​code​ ​is
compiled​ ​by​ ​running​ ​the​ ​‘make’​ ​command.

Server ​is​ ​executed​ ​by​ ​running​ ​the​ ​command:


./server​
[./server​]

Here, the client can either be any web-browser or telnet.
If web-browser is used. Then any request to the web-server is sent by the following command:
http://127.0.0.1:8089/exam.gif
[http://ip-address:port-number/file-name​]
 

Signal handler is implemented to gracefully exit the web-server.

