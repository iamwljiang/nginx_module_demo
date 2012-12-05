#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

//user include headers
#include <ctype.h>
//user config struct 
typedef struct {
	ngx_str_t		str_demo_arg1;
	ngx_int_t		int_demo_arg2;
} ngx_http_demo_main_conf_t;

//user global variable
static ngx_int_t DELAY_TIME = 50;

/*************our nginx's module function declare************/
//how to init when work process created
static ngx_int_t init_http_demo_process(ngx_cycle_t *cycle);
//what we need todo clear when work process before exiting
static void		 exit_http_demo_process(ngx_cycle_t *cycle);

//how to create user config struct
static void *	 ngx_http_demo_create_main_conf(ngx_conf_t * cf);
//how to init config(for postconfiguration)
static ngx_int_t ngx_http_demo_init(ngx_conf_t *cf);

//our nginx module kernel,we care this!
static ngx_int_t ngx_http_demo_handler(ngx_http_request_t *r);
//if one times can't finish process,we need set delay function that nginx will delay 'some time' check process is finished.
static void      ngx_http_demo_delay(ngx_http_request_t *r);

//user util function declare,maybe you don't need it
static ngx_int_t ngx_http_demo_delay_output(ngx_http_request_t * r, char *data_buffer,int len,ngx_buf_t **b,void *args);
static ngx_int_t ngx_http_demo_handler_output(ngx_http_request_t *r,char *data_buffer,int len,void *args);

//define our module command,it tell nginx which argument we will need and how to parse
static ngx_command_t ngx_http_demo_commands [] = { 
	{
		ngx_string("demo_str"),
		NGX_HTTP_MAIN_CONF|NGX_CONF_TAKE1,
		ngx_conf_set_str_slot,
		NGX_HTTP_MAIN_CONF_OFFSET,
		offsetof(ngx_http_demo_main_conf_t, str_demo_arg1),
		NULL 
	},
	{
		ngx_string("demo_int"),
		NGX_HTTP_MAIN_CONF|NGX_CONF_TAKE1,
		ngx_conf_set_num_slot,
		NGX_HTTP_MAIN_CONF_OFFSET,
		offsetof(ngx_http_demo_main_conf_t, int_demo_arg2),
		NULL 
	},
	ngx_null_command
};

//define our nginx module context,it will be used in module define
static ngx_http_module_t ngx_http_demo_modulte_ctx = {
    NULL,                                 		/* preconfiguration */
    ngx_http_demo_init,          				/* postconfiguration */

    ngx_http_demo_create_main_conf,				/* create main configuration */
    NULL,									    /* init main configuration */

    NULL,                                  		/* create server configuration */
    NULL,                                  		/* merge server configuration */

    NULL,    									/* create location configuration */
    NULL     									/* merge location configuration */
};

//define our nginx module
ngx_module_t ngx_http_demo_module = {
    NGX_MODULE_V1,
    &ngx_http_demo_modulte_ctx,  /* module context */
    ngx_http_demo_commands,      /* module directives */
    NGX_HTTP_MODULE,             /* module type */
    NULL,                        /* init master */
    NULL,						 /* init module */
    init_http_demo_process,      /* init process*/
    NULL,                        /* init thread */
    NULL,                        /* exit thread */
    exit_http_demo_process,      /* exit process*/
    NULL,                        /* exit master */
    NGX_MODULE_V1_PADDING
};


static ngx_int_t init_http_demo_process(ngx_cycle_t *cycle)
{
	ngx_http_demo_main_conf_t *sscf;
    sscf = ngx_http_cycle_get_module_main_conf(cycle, ngx_http_demo_module);
    if( sscf == NULL ) return NGX_ERROR;
	//if argument not set return error
	if(sscf->str_demo_arg1.len == 0) return NGX_ERROR;
	if(sscf->int_demo_arg2 <= 0) return NGX_ERROR;

	//TODO:in here you can do init things after work process created 
	return NGX_OK;
}

static void exit_http_demo_process(ngx_cycle_t *cycle)
{
	ngx_log_error(NGX_LOG_ALERT, cycle->log, ngx_errno,
		"demo moudle catch exit process called");
}

static void * ngx_http_demo_create_main_conf(ngx_conf_t * cf)
{
	ngx_http_demo_main_conf_t *conf;
    conf = ngx_pnalloc(cf->pool, sizeof(ngx_http_demo_main_conf_t));
    if(!conf) return NULL;

    conf->int_demo_arg2    	= NGX_CONF_UNSET;
    return conf;
}

static ngx_int_t ngx_http_demo_init(ngx_conf_t *cf)
{
	ngx_http_handler_pt        *h;
    ngx_http_core_main_conf_t  *cmcf;

    cmcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_core_module);
	//NOTE:content phase if handler not return NGX_DECLINED,it will call ngx_http_finalize_request
    h = ngx_array_push(&cmcf->phases[NGX_HTTP_PREACCESS_PHASE/*NGX_HTTP_CONTENT_PHASE*/].handlers);
    if (h == NULL) {
        return NGX_ERROR;
    }

    *h = ngx_http_demo_handler;
    return NGX_OK;
}

//util functions
static ngx_int_t ngx_http_demo_handler_output(ngx_http_request_t *r,char *data_buffer,int len,void *args)
{
	//prepare header
	/*
	ngx_table_elt_t *h = r->headers_out.content_length;
	if (h == NULL)
	{	
		h = ngx_list_push(&r->headers_out.headers);
		if(h == NULL){
			r->keepalive = 0;
			ngx_log_error(NGX_LOG_ALERT,r->connection->log,0,"proc error line:%d",__LINE__);
			return NGX_HTTP_INTERNAL_SERVER_ERROR;
		}
	}
	h->key.len = sizeof("Content-Length")-1;
	h->key.data = (u_char*)("Content-Length");
	
	char len_buf[20] = "";
	sprintf(len_buf,"%d",len);
	h->value.len = strlen(len_buf);
	h->value.data = ngx_pnalloc(r->pool,h->value.len + 1);
	if(h->value.data == NULL){
		ngx_log_error(NGX_LOG_ALERT,r->connection->log,0,"handle output alloc memory error");
		return NGX_HTTP_INTERNAL_SERVER_ERROR;
	}
	ngx_memcpy((char*)h->value.data,len_buf,h->value.len);
	h->hash = 1;
	r->headers_out.content_length = h;
	*/
	r->headers_out.status = NGX_HTTP_OK;
	r->headers_out.content_length_n = len;
	r->headers_out.content_type.len = sizeof("text/html") - 1;
	r->headers_out.content_type.data = (u_char*)("text/html"); 

	//prepare output chain 
	ngx_buf_t   *out_buf;
	ngx_chain_t out;
	out_buf = ngx_pnalloc(r->pool,sizeof(ngx_buf_t));
	if(out_buf == NULL){
		ngx_log_error(NGX_LOG_ALERT,r->connection->log,0,"handle output alloc memory error");
		return NGX_HTTP_INTERNAL_SERVER_ERROR;
	}
	
	out_buf->pos = ngx_pnalloc(r->pool,len);
	if(out_buf->pos == NULL){
			ngx_log_error(NGX_LOG_ALERT,r->connection->log,0,"handle output alloc memory error");
			return NGX_HTTP_INTERNAL_SERVER_ERROR;
	}
	ngx_memcpy(out_buf->pos,data_buffer,len);

	out_buf->memory = 1;
	out_buf->last   = out_buf->pos + len;
	out_buf->last_buf= 1;
	out_buf->in_file = 0;
	out.buf  = out_buf;
	out.next = NULL;

	ngx_int_t rc = ngx_http_send_header(r);
	if (rc == NGX_ERROR || rc > NGX_OK || r->header_only) {
		ngx_log_error(NGX_LOG_ALERT,r->connection->log,0,"demo output send header error");
		return rc;
	}

	ngx_http_output_filter(r, &out);
	return NGX_HTTP_OK;
}

static ngx_int_t ngx_http_demo_delay_output(ngx_http_request_t * r, char *data_buffer,int len,ngx_buf_t **b,void *args)
{
	//just output data to ngx_buf_t 
	*b = (ngx_buf_t*)ngx_pnalloc(r->pool,sizeof(ngx_buf_t));
	if(*b == NULL){
		ngx_log_error(NGX_LOG_ALERT,r->connection->log,ngx_errno,"delay output alloc memory error");
		ngx_http_finalize_request(r,NGX_HTTP_INTERNAL_SERVER_ERROR);
		return -1;
	}

	(*b)->pos = ngx_pnalloc(r->pool,len);
	if((*b)->pos == NULL){
		ngx_log_error(NGX_LOG_ALERT,r->connection->log,ngx_errno,"delay output alloc memory error");
		ngx_http_finalize_request(r,NGX_HTTP_INTERNAL_SERVER_ERROR);
		return -2;	
	}
	ngx_memcpy((*b)->pos,data_buffer,len);		
	
	(*b)->last_buf = 1;
	(*b)->memory   = 1;
	(*b)->in_file  = 0;
	(*b)->last   = (*b)->pos + len;
	
	return len;
}


//NOTE:important process of below function
static ngx_int_t ngx_http_demo_handler(ngx_http_request_t *r)
{
	
	if(ngx_strncmp(r->uri.data,"/demo",r->uri.len) != 0){
		return NGX_DECLINED;	
	}

	if (!(r->method & (NGX_HTTP_GET|NGX_HTTP_HEAD))) {
        return NGX_HTTP_NOT_ALLOWED;
    }

	ngx_http_demo_main_conf_t *sscf = NULL;
	sscf = ngx_http_get_module_main_conf(r, ngx_http_demo_module);
	if( sscf == NULL ) {
		ngx_log_error(NGX_LOG_ALERT,r->connection->log,0,"handler func get demo module config error");
    	return NGX_HTTP_SERVICE_UNAVAILABLE;
    }
	
	/*
	example of demo module will allow process this uri:
	/demo 
	/demo?delay=1
	*/

	char result[1024] = "";
	sprintf(result,"This is nginx demo module\ndemo str:%s,demo int:%d\n",sscf->str_demo_arg1.data,(int)sscf->int_demo_arg2);

	if(r->args.len == 0){
		return ngx_http_demo_handler_output(r,result,ngx_strlen(result),NULL);
	}
	
	char *delay_ptr = ngx_strstr(r->args.data,"delay=");
	if(delay_ptr != (char*)r->args.data){
		return NGX_HTTP_BAD_REQUEST;
	}

	int arg_value_len = r->args.len - strlen("delay=");	
	char arg_buf [10] = "";
	strncpy(arg_buf,delay_ptr+strlen("delay="),arg_value_len);
	
	int delay_flag = atoi(delay_ptr+ngx_strlen("delay="));
	if(delay_flag == 1)
	{
		//set timeout callback function
		r->write_event_handler = ngx_http_demo_delay;
		ngx_add_timer(r->connection->write, (ngx_msec_t)DELAY_TIME);
		return NGX_AGAIN;
	}else if(delay_flag != 0 || arg_value_len != 1){
		sprintf(result+ngx_strlen(result),"Unknown delay value:%s\n",arg_buf);
	}else if(isdigit(arg_buf[0]) == 0){
		sprintf(result+ngx_strlen(result),"Unknown delay value:%s\n",arg_buf);
	}else{
		sprintf(result+ngx_strlen(result),"ignore delay,value:%d\n",delay_flag);
	}

	return ngx_http_demo_handler_output(r,result,ngx_strlen(result),NULL);
}

static void ngx_http_demo_delay(ngx_http_request_t *r)
{	
	ngx_http_demo_main_conf_t *sscf = NULL;
	sscf = ngx_http_get_module_main_conf(r, ngx_http_demo_module);
	if( sscf == NULL ) {
		ngx_log_error(NGX_LOG_ALERT,r->connection->log,0,"dlay func get demo config error");
    	ngx_http_finalize_request(r,NGX_HTTP_INTERNAL_SERVER_ERROR);
    	return ;
    }
	
	char result[1024] = "";
	sprintf(result,"This is nginx demo module\nResponse from delay,demo str:%s,demo int:%d\n",sscf->str_demo_arg1.data,(int)sscf->int_demo_arg2);
	sprintf(result+ngx_strlen(result),"Client ip:");
	ngx_memcpy(result+ngx_strlen(result),r->connection->addr_text.data,r->connection->addr_text.len);
	strcat(result,"\n");
	
	ngx_chain_t out;
	ngx_buf_t   *out_buf;
	
	int ret_len = ngx_http_demo_delay_output(r,result,ngx_strlen(result),&out_buf,NULL);
	if(ret_len < 0) return;
	out.buf = out_buf;
	out.next= NULL;

	r->headers_out.content_length_n = ret_len;
	r->headers_out.status = NGX_HTTP_OK;
	r->headers_out.content_type.len  = sizeof("text/html") -1;
	r->headers_out.content_type.data = (u_char*)("text/html");
	
	ngx_int_t rc = ngx_http_send_header(r);
	if (rc == NGX_ERROR || rc > NGX_OK || r->header_only) {
		r->keepalive = 0;
		ngx_log_error(NGX_LOG_ALERT,r->connection->log,0,"ngx_http_send_header error");
		ngx_http_finalize_request(r, rc);
		return;
	}

	rc = ngx_http_output_filter(r, &out);
	ngx_http_finalize_request(r, rc);
	return;
}


