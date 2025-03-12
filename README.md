See example output in `example.out`

To build use:
```
/usr/local/bin/cmake -DCMAKE_BUILD_TYPE=Debug -G "Unix Makefiles" -S [work dir] -B [build dir]
```

To run:
```
./hft BTC-USD 10000 &> example.out
```
