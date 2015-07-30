<?php
require __DIR__ . '/../test-tools.php';
$r = new \WebUtil\R3();
$r->addRoute(
    '/test-{id:\\d+}.html', 
    $r::METHOD_GET,
    'my-test-data'
);
$r->compile();
$match = $r->match('/test-1234.html', $r::METHOD_POST);
var_dump($match);