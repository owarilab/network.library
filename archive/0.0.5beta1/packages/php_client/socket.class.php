<?php

$msg = socketModule::send( 'localhost', 1024, "send test" );
echo "recv : ${msg}\n";

class socketModule
{
	protected static $myinstance = null;
	protected $servaddr	= '';
	protected $servport	= 0;
	protected $sockid	= -1;
	
	public function __construct( $saddr, $sport )
	{
		$this->servaddr	= $saddr;
		$this->servport	= $sport;
	}

	public static function send( $saddr, $sport, $sendmsg )
	{
		$ret = true;
		self::$myinstance = new socketModule( $saddr, $sport );
		self::$myinstance->s_connect();
		self::$myinstance->s_send( $sendmsg );
		$ret = self::$myinstance->s_read();
		self::$myinstance->s_close();
		return $ret;
	}

	public function s_connect()
	{
		$ret = false;
		do{
			if( ( $this->sockid = socket_create( AF_INET, SOCK_STREAM, SOL_TCP ) ) === false )
			{
				echo "socket_create() error:". socket_strerror( socket_last_error() );
				$this->s_close();
				break;
			}
			if( ( socket_connect( $this->sockid, $this->servaddr, $this->servport ) ) === false )
			{
				echo "socket_connect() error:".socket_strerror( socket_last_error() );
				$this->s_close();
				break;
			}
			$ret = true;
		}while( false );
		return $ret;
	}
	
	public function s_send( $sendmsg )
	{
		if( $this->sockid >= 0 )
		{
			$sendmsg = $sendmsg;
			if( socket_write( $this->sockid, $sendmsg, strlen( $sendmsg ) ) === false )
			{
				echo "socket_write()  error:".socket_strerror( socket_last_error() );
				$this->s_close();
			}
		}
	}
	
	public function s_read()
	{
		$rmsg = false;
		if( $this->sockid >= 0 )
		{
			if( ( $readbuf = socket_read( $this->sockid, 2048, PHP_BINARY_READ ) ) === false )
			{
				echo "socket_read() error:". socket_strerror( socket_last_error() );
				$this->s_close();
			}
			else{
				$rmsg = $readbuf;
			}
		}
		return $rmsg;
	}
	
	public function s_close()
	{
		if( $this->sockid >= 0 )
		{
			socket_close( $this->sockid );
			$this->sockid = -1;
		}
	}
}
