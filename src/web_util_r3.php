<?hh
namespace WebUtil;
<<__NativeData("R3")>>
class R3
{
    private array $routes = [];
    public function addRoute(string $pattern, int $method, mixed $data):int {
        $this->routes[] = array(
            $pattern,
            $method,
            $data,
        );
        return count($this->routes) - 1;
    }
    public function compile():mixed{
        return $this->_compile($this->routes);
    }
    public function match(string $uri, int $method):?array{
        if(!($result = $this->_match($uri, $method))){
            return null;
        }
        list($idx, $param) = $result;
        return [
            $this->routes[$idx][2],
            $param,
        ];
    }
    <<__Native>>private function _compile(array $routes):mixed;
    <<__Native>>private function _match(string $uri, int $method):array;
}
