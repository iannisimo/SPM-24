#!/usr/bin/env bash

#SBATCH --job-name=mpi
#SBATCH --output=out
#SBATCH --err=err
#SBATCH -N8
#SBATCH --ntasks-per-node=32

DIR="/tmp/s.ianniciello_testdir"
EXE="../build/src/mpi"


if [ "$(hostname)" == "spmln" ] || [ "$(hostname)" == "r7425renaissance" ] || [[ $(hostname) =~ "node0" ]]; then
  export LD_LIBRARY_PATH="/home/s.ianniciello/.local/opt/gcc-13.2.0/lib64/:$LD_LIBRARY_PATH"
fi

rm -rf $DIR
mkdir -p $DIR

if [ "$(hostname)" == "spmln" ]; then
  find /opt/SPMcode/testdir/files/ -name "*.txt" -exec cp {} $DIR \;
elif [ "$(hostname)" == "r7425renaissance" ]; then
  find /opt/SPMcode/A2/files/ -name "*.txt" -exec cp {} $DIR \;
elif [[ $(hostname) =~ "node0" ]]; then
  find /opt/ohpc/pub/SPMcode/A2/files/ -name "*.txt" -exec cp {} $DIR \;
elif [ $(hostname) == "simonemsi" ]; then
  find ../data/ -name "*.txt" -exec cp {} $DIR \;
fi

HUGE="$DIR/huge.txt"
BIG="$DIR/big.txt"
SMALL="$DIR/pg2600.txt" 

SOME_SMALL=$(find $DIR -name "*.txt" -size -10M)
SOME_LARGE=$(find $DIR -name "*.txt" -size +10M)
#       1K   1M      100M      1G (no split)
SPLITS=(1000 1000000 100000000 1000000000)

if [[ $(hostname) =~ "node0" ]]; then
  Np=("2:" "2:--bynode" "4:" "4:--bynode" "8:" "8:--bynode" "16:" "16:--bynode" "32:" "32:--bynode" "64:" "64:--bynode" "128:" "128:--bynode" "256:")
elif [ $(hostname) == "simonemsi" ]; then
  Np=("2:" "4:" "6:" "12:")
else
  Np=("2:")
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
    PREFIX="mpirun --report-bindings -np $N $P"
    for NAME in "${!TESTS[@]}"; do
      echo "$NAME,$SPLIT,$N,$P" 1>&2
      FILES="${TESTS[$NAME]}"
      echo -n "$NAME,$SPLIT,$N,$P,"
      CTIME=$($PREFIX $EXE $FILES -s $SPLIT -qqqq)
      echo -n "$CTIME,"

      COMPRESSED_FILES=$(find $DIR -name "*.spmzip")
      DTIME=$($PREFIX $EXE $COMPRESSED_FILES -d -S.orig -qqqq)
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
