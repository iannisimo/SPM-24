#!/usr/bin/env bash

DIR="testdir"
EXE="../build/src/fastflow"

rm -rf $DIR
mkdir -p $DIR
find ../data -name "*.txt" -exec ln {} $DIR/ \;

HUGE="$DIR/huge.txt"
BIG="$DIR/big.txt"
SMALL="$DIR/pg2600.txt" 

SOME_SMALL=$(find $DIR -name "*.txt" -size -10M)
SOME_LARGE=$(find $DIR -name "*.txt" -size +10M)
#       1K   1M      100M      1G (no split)
SPLITS=(1000 1000000 100000000 1000000000)
NTHREAD=(3 5 10 20 40)


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