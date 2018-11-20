//import channel_data from './channel_data.js';

//TCP 
var net = require('net');
var HOST = '10.141.65.106';
var MANIFEST_PORT = 5000;
var DATA_PORT = 6000; 

var ind_buff_size = 256; 
 
//MANNIFEST TCP CONNECTION SETUP
var manifest_server = net.createServer();
manifest_server.listen(MANIFEST_PORT, HOST);
console.log('listening on port: '+ MANIFEST_PORT)

//DATA server
var data_server = net.createServer();

//DATA TCP CONNECTION SETUP
data_server.listen(DATA_PORT, HOST);
console.log('listening on port: '+ DATA_PORT)

//MANIFEST TABLE 
var manifest_arr = [];

//GET INCOMING MANIFESET DATA 
manifest_server.on('connection', function(sock) {
    console.log('CONNECTED to TCP on port 5000');
    sock.on('data', (data) => {
		var byte_len = data.byteLength
		
		//indexing into buffer (vaughn will throw up, need to change these in future)
		var max_str_len = 25; 
		var manifest_byte_offset = 4;
		
		console.log("Total buffer size: " + byte_len);
		for(i = 0; i < byte_len; i += ind_buff_size)	
		{	
			var channel_index = data.readUInt32LE(i)  
			var channel_type = data.readUInt32LE(i+manifest_byte_offset)
			var channel_size = data.readUInt32LE(i+(2*manifest_byte_offset))
			var channel_string = data.toString('utf8',i,i+max_str_len);
			var split = channel_string.split(',');
			var channel_name = split[0]; 
			var channel_units = split[1];
			var channel_object = { 
				"index"  	:  channel_index, 
				"data_type"	:  channel_type, 
				"size"      :  channel_size,
				"name"      :  channel_name,
				"units"     :  channel_units
			  }

			manifest_arr.push(channel_object)
		}
		print_array(manifest_arr);
    });
	sock.on('close', (data) => {
		console.log('TCP connection closed with Manifest Server');	
	});
});


//GET INCOMING STREAM OF DATA 
data_server.on('connection', function(sock) {
    console.log('CONNECTED to TCP via port 6000');
    sock.on('data', (data) => {
		console.log('data incoming: '+ data.toString());
	});
});


function print_array(arr)
{
	for(i = 0; i<arr.length;i++)
	{
		console.log("-------------------------------------------")
		console.log("Channel Index: " + arr[i].index)
		console.log("Channel Type: " + arr[i].data_type)
		console.log("Channel Size: " + arr[i].size)
		console.log("Channel Name: " + arr[i].name)
		console.log("Channel Units: " + arr[i].units)
	}

}



