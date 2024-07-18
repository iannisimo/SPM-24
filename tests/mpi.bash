#!/usr/bin/env bash

DIR="/tmp/s.ianniciello_testdir"
EXE="../build/src/mpi"

# Define function of 2 args N and P that returns the string  "srun -N N --ntasks-per-node P"
function srun_np {
  N=$1
  P=$2
  if [ "$(hostname)" == "spmln" ]; then
    echo "srun --mpi=pmix --exclusive -N $N --ntasks-per-node $P"
  else
    echo "mpirun -np $((N*P))"
  fi    
}

if [ "$(hostname)" == "spmln" ] || [ "$(hostname)" == "r7425renaissance" ]; then
  export LD_LIBRARY_PATH="/home/s.ianniciello/.local/opt/gcc-13.2.0/lib64/:$LD_LIBRARY_PATH"
fi

rm -rf $DIR
mkdir -p $DIR

if [ "$(hostname)" == "spmln" ]; then
  mkdir -p ../data/
  find /opt/SPMcode/testdir/files/ -name "*.txt" -exec cp {} ../data/ \;
elif [ "$(hostname)" == "r7425renaissance" ]; then
  mkdir -p ../data/
  find /opt/SPMcode/A2/files/ -name "*.txt" -exec cp {} ../data/ \;
fi

find ../data -name "*.txt" -exec cp {} $DIR/ \;

HUGE="$DIR/huge.txt"
BIG="$DIR/big.txt"
SMALL="$DIR/pg2600.txt" 

SOME_SMALL=$(find $DIR -name "*.txt" -size -10M)
SOME_LARGE=$(find $DIR -name "*.txt" -size +10M)
#       1K   1M      100M      1G (no split)
SPLITS=(1000 1000000 100000000 1000000000)

if [ "$(hostname)" == "spmln" ]; then
  Np=("1:2" "2:1" "1:8" "8:1" "1:32" "8:4" "8:32")
elif [ $(hostname) == "simonemsi" ]; then
  Np=("1:2" "1:4" "1:6" "1:12")
else
  Np=("1:2")
  echo "Machine not known, using 2 processes"
fi


declare -A TESTS
TESTS["some_small"]="$SOME_SMALL"
TESTS["some_large"]="$SOME_LARGE"
TESTS["huge"]="$HUGE"
TESTS["big"]="$BIG"
TESTS["small"]="$SMALL"

echo "group,split_size,N,ntasks-per-node,compression_time,decompression_time,diff"

for SPLIT in "${SPLITS[@]}"; do
  for np in "${Np[@]}"; do
    N=$(echo $np | cut -d':' -f1)
    P=$(echo $np | cut -d':' -f2)
    PREFIX=$(srun_np $N $P)
    for NAME in "${!TESTS[@]}"; do
      FILES="${TESTS[$NAME]}"
      echo -n "$NAME,$SPLIT,$N,$P,"
      CTIME=$($PREFIX $EXE $FILES -s $SPLIT -qqqq 2>/dev/null)
      echo -n "$CTIME,"
      COMPRESSED_FILES=$(find $DIR -name "*.spmzip")
      DTIME=$($PREFIX $EXE $COMPRESSED_FILES -d -S.orig -qqqq 2>/dev/null)
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