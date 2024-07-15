hash of a run

```
find ../data/ -name "*.seq" -exec md5sum {} \; | awk ' { print $1 } ' | sort | md5sum
```

exec on .txt

./src/sequential $(find ../data/ -name "*.txt") -s1000000 -S.spmzip.seq

delete zips

find ../data/ -name "*.spmzip.*" -delete