<?php
$files = glob('*.php');
usort($files, function($a, $b) {
    return filemtime($a) < filemtime($b);
});
foreach ($files as &$file) echo "$file<br>
";
?>
