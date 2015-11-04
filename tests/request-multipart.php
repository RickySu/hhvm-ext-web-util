<?php
use WebUtil\Parser\HttpParser;
function extractHeaders($header)
{
    $headers = array();
    foreach(explode("\r\n", $header) as $raw){
        list($name, $value) = explode(':', $raw);
        $headers[$name] = trim($value);
    }
    foreach(explode(';', $headers['Content-Disposition']) as $rawCol){
        $cols = explode('=', $rawCol);
        if(count($cols) == 1){
            continue;
        }
        $headers['__extra'][trim($cols[0])] = str_replace('"', '', $cols[1]); 
    }
    
    return $headers;
}

$f = fopen(__DIR__.'/fixture/request-multipart.txt', 'r');
$parser = new HttpParser();
$parser->setOnHeaderParsedCallback(function($parsedData){
    print_r($parsedData);
    return true;
});
$headers = array();
$content = array(
    'f1' => '',
    'f2' => '',
    'f3' => '',
);
$parser->setOnMultipartCallback(function($piece, $type) use(&$headers, &$content){
    if($type == 0){
        $headers = extractHeaders($piece);
        $md5 = null;
        return true;
    }
    $content[$headers['__extra']['name']].=$piece;
    return true;
});
while(!feof($f)){
    $data = fread($f, rand(10, 20));
    $parser->feed($data);
}
fclose($f);
echo 'f1:', (md5_file(__DIR__.'/fixture/250x100.png') === md5($content['f1']))?"ok":"fail", "\n";
echo 'f2:', (md5_file(__DIR__.'/fixture/640x480.png') === md5($content['f2']))?"ok":"fail", "\n";
echo 'f3:', $content['f3'], "\n";