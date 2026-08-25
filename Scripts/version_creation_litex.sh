#/bin/bash

# To be execute inside the Litex folder

FILE=litex_version_`date +%Y%m%d`.txt

rm tmp.txt $FILE

find -type d -name ".git" > tmp.txt

LINES=$(cat tmp.txt)

for i in $LINES; 
do 
    echo ${i/.git/}
    echo -n ${i/.git/} " : " >> $FILE 
    git -C $i log | head -n 1 | sed 's/commit //' >> $FILE
done

rm tmp.txt

