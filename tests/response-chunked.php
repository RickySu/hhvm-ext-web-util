<?php
use WebUtil\Parser\HttpParser;

$f = fopen(__DIR__.'/fixture/response-chunked.txt', 'r');
$parser = new HttpParser(HttpParser::TYPE_RESPONSE);
$parser->setOnHeaderParsedCallback(function($parsedData){
    print_r($parsedData);
    return true;
});
$parser->setOnBodyParsedCallback(function($content){
    echo $content; 
});
while(!feof($f)){
    $data = fread($f, rand(10, 20));
    $parser->feed($data);
}
fclose($f);
