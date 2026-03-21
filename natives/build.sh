# Build AnderBridge.so
JAVA_HOME=${JAVA_HOME:-/usr/lib/jvm/default}
gcc -shared -fPIC -O2 \
    -I"$JAVA_HOME/include" \
    -I"$JAVA_HOME/include/linux" \
    -I"./include" \
    -o ./libAnderBridge.so \
    ./AnderBridge.c
echo "Built libAnderBridge.so"