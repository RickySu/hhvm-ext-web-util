<?hh
namespace WebUtil\Parser;
<<__NativeData("WebUtil\\Parser\\HttpParser")>>
class HttpParser
{
    protected array $parsedData = [];    
    <<__Native>>public function __construct(int $type = HttpParser::TYPE_REQUEST):void;
    <<__Native>>public function feed(string $data):void;
    <<__Native>>public function reset():void;
    <<__Native>>public function setOnHeaderParsedCallback(mixed $cb):mixed;
    <<__Native>>public function setOnBodyParsedCallback(mixed $cb):mixed;
    <<__Native>>public function setOnContentPieceCallback(mixed $cb):mixed;
    <<__Native>>public function setOnMultipartCallback(mixed $cb):mixed;
}
