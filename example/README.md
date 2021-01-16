i3-msg -t get_tree > i3-msg.json  
./json2txt -J i3-msg.json | sed -nf script.sed > move.src  
. move.src  
rm move.src
