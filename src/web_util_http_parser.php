<?hh
namespace WebUtil\Parser;
<<__NativeData("WebUtil\\Parser\\HttpParser")>>
class HttpParser
{
    protected array $parsedData = [];    
    <<__Native>>public function __construct():void;
    <<__Native>>public function feed(string $data):void;
    <<__Native>>public function reset():void;
    <<__Native>>public function setOnParsedCallback(mixed $cb):mixed;
}
