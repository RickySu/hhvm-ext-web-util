<?php
use WebUtil\Parser\HttpParser;

$f = fopen(__DIR__.'/fixture/request-json.txt', 'r');
$parser = new HttpParser();
$parser->setOnHeaderParsedCallback(function($parsedData){
    print_r($parsedData);
    return true;
});
$parser->setOnBodyParsedCallback(function($content){
    print_r($content); 
});
while(!feof($f)){
    $data = fread($f, rand(10, 20));
    $parser->feed($data);
}
fclose($f);
