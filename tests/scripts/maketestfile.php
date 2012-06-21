#!/usr/bin/php
<?php

// Borrowed from http://fi.php.net/xml
function xml2array ($xml)
{
	$xmlary = array ();

	if ((strlen ($xml) < 256) && is_file ($xml))
		$xml = file_get_contents ($xml);
 
	$ReElements = '/<(\w+)\s*([^\/>]*)\s*(?:\/>|>(.*)<\/\s*\\1\s*>)/s';
	$ReAttributes = '/(\w+)=(?:"|\')([^"\']*)(:?"|\')/';
 
	preg_match_all ($ReElements, $xml, $elements);
	foreach ($elements[1] as $ie => $xx) {
		$xmlary[$ie]["name"] = $elements[1][$ie];
		if ( $attributes = trim($elements[2][$ie])) {
			preg_match_all ($ReAttributes, $attributes, $att);
			foreach ($att[1] as $ia => $xx)
				// all the attributes for current element are added here
				$xmlary[$ie]["attributes"][$att[1][$ia]] = $att[2][$ia];
		} // if $attributes
    
		// get text if it's combined with sub elements
		$cdend = strpos($elements[3][$ie],"<");
		if ($cdend > 0) {
			$xmlary[$ie]["text"] = substr($elements[3][$ie],0,$cdend -1);
		} // if cdend
      
		if (preg_match ($ReElements, $elements[3][$ie]))       
			$xmlary[$ie]["elements"] = xml2array ($elements[3][$ie]);
		else if ($elements[3][$ie]){
			$xmlary[$ie]["text"] = $elements[3][$ie];
		}
	}
	return $xmlary;
}

function print_audio_attributes($array, $path, $pref, $suff) {
	$printit = false;
	$count = 0;
	foreach($array as $name => $value) {
		if($value == "audio") {
			//print "Got audio node again\n";
			$printem = true;
		}

		//print $count++.".".$name."2=".$value."\n";

		if($name == "attributes" && $printem == true) {
			foreach($value as $tag => $val) {
				switch($tag) {
				case 'src':
					print $pref."\"".$path."/".$val."\", ";
					break;
				case 'begin':
					print "\"".$val."\", ";
					break;
				case 'end':
					print "\"".$val."\"".$suff;
					break;

				}
			}
			print "\n";
				
		}
	}	


}

function print_audio_nodes($array, $path, $pref, $suff) {
	$printem = false;
	foreach($array as $name => $value) {
		if($name == "audio") {
			$printem = true;
			//print "Got audio node\n";
		}
			
		if($name == "attributes" && $printem) {
			print_audio_attributes($value, $path, $pref, $suff);
			$printem = false;
		}

		if(is_array($value))
			print_audio_nodes($value, $path, $pref, $suff);
	}
}

if($argv[1] == "") {
	printf("Extract mp3 filename and audio start-stop times from smil files\n");
	printf("   Usage: ".$argv[0]." file.smil [prefix] [suffix]\n");
	exit();
}

if (file_exists($argv[1])) {
	$data = file_get_contents($argv[1]);
	
	$xml = xml2array($data);
	
	print_audio_nodes($xml, dirname($argv[1]), $argv[2], $argv[3]);

} else {
   exit("Failed to open $argv[1].");
}

?>
