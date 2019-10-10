<?php

$id=intval($_REQUEST["id"]);

$key=$_REQUEST["key"];
$flag=$_REQUEST["flag"];

if (!preg_match("/^[a-zA-Z0-9_=]+$/", $flag)) die("bad flag format");
if (!preg_match("/^[a-zA-Z0-9]+$/", $key)) die("bad key format");

$data='<?php if($_GET["key"]!="'.$key.'") die("bad key"); echo "'.$flag.'"; ?>';

file_put_contents("AnCY_$id.php", $data);

echo "OK";
?>