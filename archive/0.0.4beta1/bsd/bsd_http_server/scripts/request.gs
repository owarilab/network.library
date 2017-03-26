/*
 * simple http script
 * 
 * variables : _HEADER
 *             _GET
 *             _POST
 *             _INI
 */

// main
{
    document_root = "./www";
    file_path = document_root + _HEADER['REQUEST'];
    if( _HEADER['REQUEST'] == '/' ){
        file_path = document_root + "/index.html";
    }
    extension = file_extension( file_path );
    if( extension == 'html')
    {
        if( file_exist( file_path ) ){
            body = file_get( file_path );
            html = make_html_response( 200, body );
        }
        else{
            html = make_html_response( 404, "404 Not Found" );
        }
    }
    else if( extension == 'ico' )
    {
        html = make_html_response( 404, "404 Not Found" );
    }
    else if( extension == 'png' )
    {
        if( file_exist( file_path ) ){
            html = make_png_response( file_path );
        }
        else{
            html = make_html_response( 404, "404 Not Found" );
        }
    }
    else{
        html = make_html_response( 404, "404 Not Found" );
    }
} // end main

def make_html( html_body )
{
    body =
        "<!DOCTYPE html>\r\n" + 
        "<html lang=\"ja\">\r\n" + 
        "<head>\r\n" + 
        '<meta name="viewport" content="width=device-width, initial-scale=1.0, minimum-scale=1.0, maximum-scale=1.0, user-scalable=0">' + "\r\n" + 
        '<meta http-equiv="Content-Type" content="text/html; charset=UTF-8">' + "\r\n" + 
        '<meta http-equiv="Content-Style-Type" content="text/css">' + "\r\n" + 
        '<meta http-equiv="Content-Script-Type" content="text/javascript">' + "\r\n" + 
        "<title>gdt_server</title>\r\n" + 
        "</head>\r\n" + 
        "<body>\r\n" + 
        html_body +
        "</body>\r\n" +
        "</html>\r\n"
    ;
    html = make_html_response( 200, body );
    return html;
}

def make_file_response( file_path )
{
    body_length = file_size( file_path );
    body = "";
    if( file_exist( file_path ) ){
        body = file_get( file_path, "b" );
        http_status_code = 200;
    }
    else{
        http_status_code = 404;
    }
    date = gmtime();
    header = "";
    http_version = "HTTP/1.1 ";
    if( http_status_code == 200 ){
        header = http_version+http_status_code+" OK\r\n";
    }
    else if( http_status_code == 400 ){
        header = http_version+http_status_code+" 400 Bad Request\r\n";
    }
    else if( http_status_code == 404 ){
        header = http_version+http_status_code+" 404 Not Found\r\n";
    }
    header = header + "Server: gdt-httpd\r\n";
    header = header + "Content-Type: application/octet-stream\r\n";
    header = header + "Content-Description: File Transfer\r\n";
    header = header + "Content-Disposition: attachment; filename=\"test\"\r\n";
    header = header + "Expires: 0\r\n";
    header = header + "Cache-Control: must-revalidate\r\n";
    header = header + "Pragma: public\r\n";
    header = header + "Connection: close\r\n";
    header = header + "Date: "+date+" GMT\r\n";
    header = header + "Content-Length: "+body_length+"\r\n";
    header = header + "\r\n";
    if( _HEADER['HTTP_METHOD']=='HEAD'){
        body = "";
    }
    log = _HEADER['HTTP_METHOD'] + " " + http_status_code + " " + _HEADER['HTTP_VERSION'] + " " + date + " " + _HEADER['REQUEST'] + " " + _HEADER['CONTENT_LENGTH']+ " " + body_length + "\n";
    //file_put( "./access_log", log );
    html = header + body;
    return html;
}

def make_png_response( file_path )
{
    body_length = file_size( file_path );
    body = "";
    if( file_exist( file_path ) ){
        body = file_get( file_path, "b" );
        http_status_code = 200;
    }
    else{
        http_status_code = 404;
    }
    date = gmtime();
    header = "";
    http_version = "HTTP/1.1 ";
    if( http_status_code == 200 ){
        header = http_version+http_status_code+" OK\r\n";
    }
    else if( http_status_code == 400 ){
        header = http_version+http_status_code+" 400 Bad Request\r\n";
    }
    else if( http_status_code == 404 ){
        header = http_version+http_status_code+" 404 Not Found\r\n";
    }
    header = header + "Server: gdt-httpd\r\n";
    header = header + "Content-Type: image/png\r\n";
    header = header + "Content-Transfer-Encoding: binary\r\n";
    header = header + "Connection: close\r\n";
    header = header + "Date: "+date+"\r\n";
    header = header + "Content-Length: "+body_length+"\r\n";
    header = header + "\r\n";
    if( _HEADER['HTTP_METHOD']=='HEAD'){
        body = "";
    }
    log = _HEADER['HTTP_METHOD'] + " " + http_status_code + " " + _HEADER['HTTP_VERSION'] + " " + date + " " + _HEADER['REQUEST'] + " " + _HEADER['CONTENT_LENGTH']+ " " + body_length + "\n";
    //file_put( "./access_log", log );
    html = header + body;
    return html;
}

def make_html_response( http_status_code, body )
{
    body_length = count( body );
    date = gmtime();
    header = "";
    http_version = "HTTP/1.1 ";
    if( http_status_code == 200 ){
        header = http_version+http_status_code+" OK\r\n";
    }
    else if( http_status_code == 400 ){
        header = http_version+http_status_code+" 400 Bad Request\r\n";
    }
    else if( http_status_code == 404 ){
        header = http_version+http_status_code+" 404 Not Found\r\n";
    }
    header = header + "Server: gdt-httpd\r\n";
    header = header + "Content-Type: text/html\r\n";
    header = header + "Connection: close\r\n";
    header = header + "Date: "+date+"\r\n";
    header = header + "Content-Length: "+body_length+"\r\n";
    header = header + "\r\n";
    if( _HEADER['HTTP_METHOD']=='HEAD'){
        body = "";
    }
    log = _HEADER['HTTP_METHOD'] + " " + http_status_code + " " + _HEADER['HTTP_VERSION'] + " " + date + " " + _HEADER['REQUEST'] + " " + _HEADER['CONTENT_LENGTH']+ " " + body_length + "\n";
    //file_put( "./access_log", log );
    html = header + body;
    return html;
}
