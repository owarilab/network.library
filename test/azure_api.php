<?php

// azure storage rest api test 

// sudo apt install php8.1-cli php8.1-curl php8.1-xml

$storage_account_name = '';
$storage_account_key = '';
$container_name = '';
$azore_rest_api = new AzureStorageRestAPI($storage_account_name, $storage_account_key, $container_name);
print_r($azore_rest_api->get('test.txt'));
print_r($azore_rest_api->properties('test.txt'));
print_r($azore_rest_api->put('test5.txt', 'test' . time()));
//print_r($azore_rest_api->put('users/1/test2.txt', 'test' . time()));
//print_r($azore_rest_api->put('users/1/test2.txt', 'test' . time()));
//print_r($azore_rest_api->put('users/2/test3.txt', 'test' . time()));
//print_r($azore_rest_api->put('users/2/test4.txt', 'test' . time()));
print_r($azore_rest_api->delete('test5.txt'));
print_r($azore_rest_api->copy('test6.txt','test.txt'));
//print_r($azore_rest_api->list_blobs());
print_r($azore_rest_api->put_stream('test.png','./tmp/test.png'));
$sas_url = $azore_rest_api->sas_url('test.png');
echo $sas_url . "\n";

if(false)
{
    $xml = simplexml_load_string($azore_rest_api->list_blobs('users/1')['body']);
    foreach($xml->Blobs->Blob as $blob){
        $blob_name = $blob->{'Name'};
        $content_length = $blob->Properties->{'Content-Length'};
        echo $blob_name . ' ' . $content_length . "\n";
    }
    
    $sas_url = $azore_rest_api->sas_url('test.txt');
    echo $sas_url . "\n";
}

class AzureStorageRestAPI
{
    private $api_version = '2023-11-03';
    private $storage_account_name = '';
    private $storage_account_key = '';
    private $container_name = '';

    public function __construct($storage_account_name, $storage_account_key, $container_name)
    {
        $this->storage_account_name = $storage_account_name;
        $this->storage_account_key = $storage_account_key;
        $this->container_name = $container_name;
    }

    public function sas_url($blob_name, $expiry = 3600)
    {
        $blob_url = $this->blob_url($blob_name);

        $resourceName = $this->resource_name($blob_name);

        // expire iso8601 format
        $signedExpiry = gmdate('Y-m-d\TH:i:s\Z', time() + $expiry);
        $signdPermissions = 'r'; // r = read, w = write, d = delete, l = list
        $signdResource = 'b'; // b = blob, c = container

        // optional
        $signedStart = '';
        $signedIP = '';
        $signedProtocol = 'https';
        $signedIdentifier = '';
        $signedSnapshotTime = '';
        $signedEncryptionScope = '';
        $cacheControl = '';
        $contentDisposition = '';
        $contentEncoding = '';
        $contentLanguage = '';
        $contentType = '';

        // sas token
        $sign_string = $signdPermissions . "\n" .
        $signedStart . "\n" .
        $signedExpiry . "\n" .
        $this->canonical_resource($signdResource,$resourceName) . "\n" .
        $signedIdentifier . "\n" .
        $signedIP . "\n" .
        $signedProtocol . "\n" .
        $this->api_version . "\n" .
        $signdResource . "\n" .
        $signedSnapshotTime . "\n" .
        $signedEncryptionScope . "\n" .
        $cacheControl . "\n" .
        $contentDisposition . "\n" .
        $contentEncoding . "\n" .
        $contentLanguage . "\n" .
        $contentType;

        $sig = base64_encode(hash_hmac('sha256', $sign_string, base64_decode($this->storage_account_key), true));

        $query_params = [];
        $query_params['sv'] = $this->api_version;
        $query_params['sr'] = $signdResource;
        if(!empty($cacheControl)) $query_params['rscc'] = $cacheControl;
        if(!empty($contentDisposition)) $query_params['rscd'] = $contentDisposition;
        if(!empty($contentEncoding)) $query_params['rsce'] = $contentEncoding;
        if(!empty($contentLanguage)) $query_params['rscl'] = $contentLanguage;
        if(!empty($contentType)) $query_params['rsct'] = $contentType;
        if(!empty($signedStart)) $query_params['st'] = $signedStart;
        if(!empty($signedProtocol)) $query_params['spr'] = $signedProtocol;
        if(!empty($signedIdentifier)) $query_params['si'] = $signedIdentifier;
        if(!empty($signedEncryptionScope)) $query_params['ses'] = $signedEncryptionScope;
        $query_params['se'] = $signedExpiry;
        $query_params['sp'] = $signdPermissions;
        if(!empty($signedIP)) $query_params['sip'] = $signedIP;

        $query_params['sig'] = $sig;

        $query_string = http_build_query($query_params, '', '&', PHP_QUERY_RFC3986);
        $blob_url .= ($query_string ? '?' . $query_string : '');

        return $blob_url;
    }

    public function list_blobs($prefix='')
    {
        $method = 'GET';

        $query_params = [];
        $query_params['restype'] = 'container';
        $query_params['comp'] = 'list';
        if($prefix!=''){
            $query_params['prefix'] = $prefix;
        }

        ksort($query_params);

        $query_string = http_build_query($query_params);

        $blob_url = "https://$this->storage_account_name.blob.core.windows.net/$this->container_name";
        $blob_url .= ($query_string ? '?' . $query_string : '');

        $canonicalized_header_array = $this->make_azure_storage_rest_api_headers();

        $canonicalized_headers = implode("\n", $canonicalized_header_array);

        $canonicalized_resource = "/$this->storage_account_name/$this->container_name";
        foreach ($query_params as $key => $value) {
            $canonicalized_resource .= "\n$key:$value";
        }

        $sign_params = $this->make_sign_params(
            [
                'verb' => $method,
                'canonicalized_headers' => $canonicalized_headers,
                'canonicalized_resource' => $canonicalized_resource
            ]
        );

        $signature = $this->sign($sign_params);

        $header_params = [];
        $header_params = array_merge($header_params, $canonicalized_header_array);
        $request_headers = $this->make_headers($header_params, $signature);

        return $this->request($method, $blob_url, $request_headers);
    }

    public function get($blob_name, $is_meta = false)
    {
        $method = 'GET';
        $blob_url = $this->blob_url($blob_name);

        $query_params = [];
        if($is_meta) {
            $method = 'HEAD';
            $query_params['comp'] = 'metadata';
        }

        $query_string = http_build_query($query_params);
        $blob_url .= ($query_string ? '?' . $query_string : '');

        $canonicalized_header_array = $this->make_azure_storage_rest_api_headers();

        $canonicalized_headers = implode("\n", $canonicalized_header_array);

        $canonicalized_resource = "/$this->storage_account_name/$this->container_name/$blob_name";
        foreach ($query_params as $key => $value) {
            $canonicalized_resource .= "\n$key:$value";
        }

        $sign_params = $this->make_sign_params(
            [
                'verb' => $method,
                'canonicalized_headers' => $canonicalized_headers,
                'canonicalized_resource' => $canonicalized_resource
            ]
        );

        $signature = $this->sign($sign_params);

        $header_params = [];
        $header_params = array_merge($header_params, $canonicalized_header_array);
        $request_headers = $this->make_headers($header_params, $signature);

        return $this->request($method, $blob_url, $request_headers);
    }

    public function properties($blob_name)
    {
        $method = 'HEAD';
        $blob_url = $this->blob_url($blob_name);

        $canonicalized_header_array = $this->make_azure_storage_rest_api_headers();

        $canonicalized_headers = implode("\n", $canonicalized_header_array);

        $canonicalized_resource = "/$this->storage_account_name/$this->container_name/$blob_name";

        $sign_params = $this->make_sign_params(
            [
                'verb' => $method,
                'canonicalized_headers' => $canonicalized_headers,
                'canonicalized_resource' => $canonicalized_resource
            ]
        );

        $signature = $this->sign($sign_params);

        $header_params = [];
        $header_params = array_merge($header_params, $canonicalized_header_array);
        $request_headers = $this->make_headers($header_params, $signature);

        return $this->request($method, $blob_url, $request_headers);
    }

    public function put($blob_name, $blob_content)
    {
        $method = 'PUT';
        $blob_url = $this->blob_url($blob_name);

        $blob_type = 'BlockBlob';
        $blob_content_type = 'text/plain';
        $blob_content_length = strlen($blob_content);

        $canonicalized_header_array = $this->make_azure_storage_rest_api_headers(
            [
                "x-ms-blob-type:$blob_type",
            ]
        );

        // make string
        $canonicalized_headers = implode("\n", $canonicalized_header_array);

        $canonicalized_resource = "/$this->storage_account_name/$this->container_name/$blob_name";

        $sign_params = $this->make_sign_params(
            [
                'verb' => $method,
                'content_length' => $blob_content_length,
                'content_type' => $blob_content_type,
                'canonicalized_headers' => $canonicalized_headers,
                'canonicalized_resource' => $canonicalized_resource
            ]
        );

        $signature = $this->sign($sign_params);

        $header_params = [
            "Content-Length:$blob_content_length",
            "Content-Type:$blob_content_type",
        ];
        $header_params = array_merge($header_params, $canonicalized_header_array);
        $request_headers = $this->make_headers($header_params, $signature);

        return $this->request($method, $blob_url, $request_headers, $blob_content);
    }

    public function put_stream($blob_name,$file_pash)
    {
        $method = 'PUT';
        $blob_url = $this->blob_url($blob_name);

        $blob_type = 'BlockBlob';
        $blob_content_type = 'text/plain';
        $blob_content_length = filesize($file_pash);

        $canonicalized_header_array = $this->make_azure_storage_rest_api_headers(
            [
                "x-ms-blob-type:$blob_type",
            ]
        );

        // make string
        $canonicalized_headers = implode("\n", $canonicalized_header_array);

        $canonicalized_resource = "/$this->storage_account_name/$this->container_name/$blob_name";

        $sign_params = $this->make_sign_params(
            [
                'verb' => $method,
                'content_length' => $blob_content_length,
                'content_type' => $blob_content_type,
                'canonicalized_headers' => $canonicalized_headers,
                'canonicalized_resource' => $canonicalized_resource
            ]
        );

        $signature = $this->sign($sign_params);

        $header_params = [
            "Content-Length:$blob_content_length",
            "Content-Type:$blob_content_type",
        ];
        $header_params = array_merge($header_params, $canonicalized_header_array);
        $request_headers = $this->make_headers($header_params, $signature);

        $blob_content = fopen($file_pash, 'r');

        return $this->request($method, $blob_url, $request_headers, $blob_content, filesize($file_pash));
    }

    public function delete($blob_name)
    {
        $method = 'DELETE';
        $blob_url = $this->blob_url($blob_name);

        $canonicalized_header_array = $this->make_azure_storage_rest_api_headers();

        $canonicalized_headers = implode("\n", $canonicalized_header_array);

        $canonicalized_resource = "/$this->storage_account_name/$this->container_name/$blob_name";

        $sign_params = $this->make_sign_params(
            [
                'verb' => $method,
                'canonicalized_headers' => $canonicalized_headers,
                'canonicalized_resource' => $canonicalized_resource
            ]
        );

        $signature = $this->sign($sign_params);

        $header_params = [];
        $header_params = array_merge($header_params, $canonicalized_header_array);
        $request_headers = $this->make_headers($header_params, $signature);

        return $this->request($method, $blob_url, $request_headers);
    }

    public function copy($blob_name, $source_blob_name)
    {
        $src_blob = $this->properties($source_blob_name);
        if($src_blob['status_code'] != 200){
            throw new Exception("source blob not found");
        }
        $content_length = $src_blob['headers']['Content-Length'] ?? 0;

        if($content_length == 0){
            throw new Exception("source blob content length is 0");
        }

        $method = 'PUT';
        $blob_url = $this->blob_url($blob_name);

        $src_url = $this->blob_url($source_blob_name);

        $canonicalized_header_array = $this->make_azure_storage_rest_api_headers(
            [
                "x-ms-copy-source:$src_url",
            ]
        );

        // make string
        $canonicalized_headers = implode("\n", $canonicalized_header_array);

        $canonicalized_resource = "/$this->storage_account_name/$this->container_name/$blob_name";

        $sign_params = $this->make_sign_params(
            [
                'verb' => $method,
                'content_length' => $content_length,
                'canonicalized_headers' => $canonicalized_headers,
                'canonicalized_resource' => $canonicalized_resource
            ]
        );

        $signature = $this->sign($sign_params);

        $header_params = [
            "Content-Length:$content_length",
        ];
        $header_params = array_merge($header_params, $canonicalized_header_array);
        $request_headers = $this->make_headers($header_params, $signature);

        return $this->request($method, $blob_url, $request_headers);
    }

    private function blob_url($blob_name)
    {
        $blob_url = "https://$this->storage_account_name.blob.core.windows.net/$this->container_name/$blob_name";
        return $blob_url;
    }

    private function resource_name($blob_name)
    {
        $name = "$this->container_name/$blob_name";
        return $name;
    }

    private function canonical_resource($signd_resource,$resource_name)
    {
        $serviceMap = [
            'b' => 'blob',
            'c' => 'container'
        ];
        $service = $serviceMap[$signd_resource];
        $resource = "/$service/$this->storage_account_name/$resource_name";
        return $resource;
    }

    private function make_azure_storage_rest_api_headers($header_params=[])
    {
        $date = gmdate('D, d M Y H:i:s \G\M\T');
        $headers = array_merge(
            [
                "x-ms-date:{$date}",
                "x-ms-version:$this->api_version"
            ],
            $header_params
        );
        sort($headers);
        return $headers;
    }

    private function make_headers($header_params, $signature)
    {
        ksort($header_params, SORT_STRING);
        $headers = [];
        foreach ($header_params as $key => $value) {
            $headers[] = "$value";
        }
        $headers[] = "Authorization:SharedKey $this->storage_account_name:$signature";
        return $headers;
    }

    private function make_sign_params($params)
    {
        $sign_params = [
            'verb' => '',
            'content_encoding' => '',
            'content_language' => '',
            'content_length' => '',
            'content_md5' => '',
            'content_type' => '',
            'date' => '',
            'if_modified_since' => '',
            'if_match' => '',
            'if_none_match' => '',
            'if_unmodified_since' => '',
            'range' => '',
            'canonicalized_headers' => '',
            'canonicalized_resource' => ''
        ];
        return array_merge($sign_params, $params);
    }

    private function sign($sign_params)
    {
        $sign_string = $sign_params['verb'] . "\n" .
        $sign_params['content_encoding'] . "\n" .
        $sign_params['content_language'] . "\n" .
        $sign_params['content_length'] . "\n" .
        $sign_params['content_md5'] . "\n" .
        $sign_params['content_type'] . "\n" .
        $sign_params['date'] . "\n" .
        $sign_params['if_modified_since'] . "\n" .
        $sign_params['if_match'] . "\n" .
        $sign_params['if_none_match'] . "\n" .
        $sign_params['if_unmodified_since'] . "\n" .
        $sign_params['range'] . "\n" .
        $sign_params['canonicalized_headers'] . "\n" .
        $sign_params['canonicalized_resource'];
                
        $signature = base64_encode(hash_hmac('sha256', $sign_string, base64_decode($this->storage_account_key), true));
        
        return $signature;
    }

    private function request($method, $blob_url, $request_headers, $blob_content = null, $blob_content_size = 0)
    {
        //echo $method . "\n";
        //echo $blob_url . "\n";
        //echo implode("\n", $request_headers) . "\n";
        //echo $blob_content . "\n";

        $is_file_get_contents = false;

        if ($is_file_get_contents) {
            $context = stream_context_create([
                'http' => [
                    'method' => $method,
                    'header' => implode("\r\n", $request_headers),
                    'content' => $blob_content,
                    'timeout' => 5,
                ],
            ]);

            $body = '';
            $handle = @fopen($blob_url, 'r', false, $context);

            if ($handle === false) {
                $error = error_get_last();
                throw new Exception("fopen error: " . $error['message']);
            }
            
            while (($char = fgetc($handle)) !== false) {
                $body .= $char;
            }
            
            fclose($handle);

            // http version , status code , status messageを取得
            $http_version = substr($http_response_header[0], 0, strpos($http_response_header[0], ' '));
            $status_code = substr($http_response_header[0], strpos($http_response_header[0], ' ') + 1, 3);
            $status_message = substr($http_response_header[0], strpos($http_response_header[0], ' ') + 4);

            // headerをキーと値に分割
            $response_headers = [];
            foreach ($http_response_header as $header) {
                $header = explode(':', $header, 2);
                if (count($header) < 2) {
                    continue;
                }
                $response_headers[$header[0]] = trim($header[1]);
            }
    
            $header = implode("\r\n", $http_response_header);
    
            return [
                'http_version' => $http_version,
                'status_code' => $status_code,
                'status_message' => $status_message,
                'header' => $header,
                'headers' => $response_headers,
                'body' => $body
            ];
        }else{
            $ch = curl_init();
            curl_setopt($ch, CURLOPT_URL, $blob_url);
            curl_setopt($ch, CURLOPT_CUSTOMREQUEST, $method);
            curl_setopt($ch, CURLOPT_HTTPHEADER, $request_headers);
            curl_setopt($ch, CURLOPT_RETURNTRANSFER, true);
            curl_setopt($ch, CURLOPT_HEADER, true);
            curl_setopt($ch, CURLOPT_CONNECTTIMEOUT, 1);
            curl_setopt($ch, CURLOPT_TIMEOUT, 5);

            if($blob_content){
                if(is_resource($blob_content) && get_resource_type($blob_content) == 'stream'){
                    curl_setopt($ch, CURLOPT_PUT, true);
                    curl_setopt($ch, CURLOPT_INFILE, $blob_content); // ファイルポインタ
                    curl_setopt($ch, CURLOPT_INFILESIZE, $blob_content_size);
                    // タイムアウトを長めに設定
                    curl_setopt($ch, CURLOPT_TIMEOUT, 60 * 10);
                }else if ($blob_content) {
                    curl_setopt($ch, CURLOPT_POSTFIELDS, $blob_content);
                }
            }
            if($method == 'HEAD'){
                curl_setopt($ch, CURLOPT_NOBODY, true);
            }
            $response = curl_exec($ch);
            
            // error
            if ($response === false) {
                $error_code = curl_errno($ch);
                $error_message = curl_error($ch);
                curl_close($ch);
                throw new Exception("Curl error: code $error_code , msg: $error_message");
            }
    
            $response_header = substr($response, 0, strpos($response, "\r\n\r\n"));
            $response_body = substr($response, strpos($response, "\r\n\r\n") + 4);

            // http version , status code , status message
            $http_version = '';
            $status_code = '';
            $status_message = '';

            // headerをキーと値に分割
            $response_headers = [];
            $count = 0;
            foreach (explode("\r\n", $response_header) as $header) {
                if($count == 0){
                    $http_version = substr($header, 0, strpos($header, ' '));
                    $status_code = substr($header, strpos($header, ' ') + 1, 3);
                    $status_message = substr($header, strpos($header, ' ') + 4);
                    $count++;
                    continue;
                }
                $header = explode(':', $header, 2);
                if (count($header) < 2) {
                    continue;
                }
                $response_headers[$header[0]] = trim($header[1]);
            }
    
            curl_close($ch);
    
            return [
                'http_version' => $http_version,
                'status_code' => $status_code,
                'status_message' => $status_message,
                'header' => $response_header,
                'headers' => $response_headers,
                'body' => $response_body
            ];
        }
    }
}
