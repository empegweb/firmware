#!/bin/sh
echo "<HTML>"
echo "<TITLE>File listing</TITLE>"
echo "<BODY>"
echo "<UL>"
for i in *; do
  if [ "$i" != "makelist.sh" ]; then
	echo "<LI><A HREF=\"$i\">$i</A></LI>"
  fi
done
echo "</UL>"
echo "</BODY>"
echo "</HTML>"
