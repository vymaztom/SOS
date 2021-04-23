#!/bin/bash

browser="otter-browser"
browser_zip="$browser.zip"

zip $browser_zip $browser

ldd $browser | while read line ; do
	library=`echo $line | grep -o "/.*/.*\\s"`
	zip −u $browser_zip $library
done
