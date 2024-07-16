#!/usr/bin/env bash

DIR="testdir"
EXE="../build/src/fastflow"

if [ "$(hostname)" == "spmln" ]; then
  export LD_LIBRARY_PATH="/home/s.ianniciello/.local/opt/gcc-13.2.0/lib64/:$LD_LIBRARY_PATH"
fi

rm -rf $DIR
mkdir -p $DIR

if [ "$(hostname)" == "spmln" ]; then
  find /opt/SPMcode/testdir/files/ -name "*.txt" -exec ln -s {} $DIR/ \;
else
  find ../data -name "*.txt" -exec ln {} $DIR/ \;
fi

HUGE="$DIR/huge.txt"
BIG="$DIR/big.txt"
SMALL="$DIR/pg2600.txt" 

SOME_SMALL=$(find $DIR -name "*.txt" -size -10M)
SOME_LARGE=$(find $DIR -name "*.txt" -size +10M)
#       1K   1M      100M      1G (no split)
SPLITS=(1000 1000000 100000000 1000000000)

if [ "$(hostname)" == "spmln" ]; then
  NTHREADS=(3 5 10 20 40)
elif [ $(hostname) == "simonemsi" ]; then
  NTHREADS=(3 6 12)
else
  NTHREADS=(3)
  echo "Machine not known, using 3 threads (minimum)"
fi


declare -A TESTS
TESTS["some_small"]="$SOME_SMALL"
TESTS["some_large"]="$SOME_LARGE"
TESTS["huge"]="$HUGE"
TESTS["big"]="$BIG"
TESTS["small"]="$SMALL"

for SPLIT in "${SPLITS[@]}"; do
  for NTHREAD in "${NTHREADS[@]}"; do
    for NAME in "${!TESTS[@]}"; do
      FILES="${TESTS[$NAME]}"
      echo -n "$NAME,$SPLIT,$NTHREAD,"
      CTIME=$($EXE $FILES -j $NTHREAD -s $SPLIT )
      echo -n "$CTIME,"
      COMPRESSED_FILES=$(find $DIR -name "*.spmzip")
      DTIME=$($EXE $COMPRESSED_FILES -d -j $NTHREAD -S.orig)
      echo -n "$DTIME,"
      DECOMPRESSED_FILES=$(find $DIR -name "*.spmzip.orig")
      ORIG=$(md5sum $FILES | awk '{print $1}' | sort -h) 
      DECO=$(md5sum $DECOMPRESSED_FILES | awk '{print $1}' | sort -h) 
      if [ "$ORIG" == "$DECO" ]; then
        STATUS="OK"
      else
        STATUS="FAIL"
      fi
      echo "$STATUS"
      
      rm $COMPRESSED_FILES
      rm $DECOMPRESSED_FILES
    done
  done
done

rm -rf $DIR