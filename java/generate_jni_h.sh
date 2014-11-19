#!/bin/bash
SRC=target/classes
TGT=../src/xem-jni/include

for cl in $(find $SRC/org -name "*.class" | cut -d '/' -f 3- | cut -d . -f 1 | sed 's/\//\./g') ; do 
    echo Generate JNI header : $cl
    javah -classpath $SRC -jni -d $TGT $cl
done
