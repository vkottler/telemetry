
//TCP 
var net = require('net');
var HOST = '10.141.65.106';
var PORT = 5000;

 
//TCP CONNECTION SETUP
var server = net.createServer();
server.listen(PORT, HOST);
console.log('listening on port: '+ PORT)

//GET INCOMING DATA 
server.on('connection', function(sock) {
    console.log('CONNECTED to TCP');
    sock.on('data', (data) => {
	console.log(data)
    });

});

server.on('disconnect', function() {
  console.log('TCP connection closed');
});

