<?php
require __DIR__ . '/../test-tools.php';
$r = new \WebUtil\R3();
$r->addRoute(
    '/test-{id:\\d+}.html', 
    $r::METHOD_GET|$r::METHOD_POST,
    'my-test-data'
);
$r->compile();
$match = $r->match('/test-1234.html', $r::METHOD_GET);
echo "{$match[0]} {$match[1]['id']}\n";
$match = $r->match('/test-1234.html', $r::METHOD_POST);
echo "{$match[0]} {$match[1]['id']}\n";
