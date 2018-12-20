libuv_1.20.3版本修改类容：

1.增加函数：int uv_tcp_connected_open(uv_tcp_t* handle, uv_os_sock_t sock)
	win/tcp.c:1493
	unix/tcp.c:275
	uv.h:534

2.增加函数：void uv_setfd(uv_pipe_t* handle, int fd)
	uv.h:711
	unix/pipe.c:32

3.增加函数：
int uv_acceptfd(uv_stream_t* server)
	uv.h:712
	unix/pipe.c:36