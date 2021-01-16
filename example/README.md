./json2txt -J i3-msg.json | sed -nf script.sed > move.src  
. move.src  
rm move.src
