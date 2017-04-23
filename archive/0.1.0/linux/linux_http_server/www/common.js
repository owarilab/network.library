var socket = null;
var status = 0;

window.onload = function()
{
	//createSocket( "localhost", 1024 );
};

function createSocket( hostName, port )
{
	if( socket == null ){
		socket = new WebSocket('ws:/'+'/' + hostName + ':'+ port +'/echo',['chat','superchat']);
		socket.onopen 		= onSockOpen;
		socket.onerror 		= onSockError;
		socket.onmessage 	= onSockMessage;
	}
};
function onSockOpen()
{
	console.log( "connect websocket now" );
	status = 1;
	sendSockMessage("TEST_MESSAGE:LOGIN!!");
};

function onSockMessage( e )
{
	console.log( "websocket recv : " + e.data );
	var recvwindow = document.getElementById('recv_msg');
	if( recvwindow != null ){
		if( recvwindow.innerHTML != "" ){
			recvwindow.innerHTML = "<br>\n" + recvwindow.innerHTML;
		}
		recvwindow.innerHTML = e.data + "\n" + recvwindow.innerHTML;
	}
};

function onSockError( error )
{
	console.log( "websocket error : " + error );
};

function sendSockMessage( message )
{
	if( socket != null && status > 0 )
	{
		socket.send( message );
	}
};
function onSendButton()
{
	var sendelm = document.getElementById('id_send');
	if( sendelm != null ){
		sendSockMessage( "TEST_MESSAGE:" + sendelm.value );
	}
};
