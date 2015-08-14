<?hh
namespace WebUtil;
<<__NativeData("WebUtil\\R3")>>
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
    <<__Native>>public function compile():bool;
    <<__Native>>public function match(string $uri, int $method):array;
}
