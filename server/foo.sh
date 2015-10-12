#! /bin/bash
~/httperf/bin/httperf --server=ebola.cs.byu.edu --port=4085 --uri=/CHR_Un/musca_domestica_10mb.txt --rate=0 --num-conns=500 --num-call=10 --timeout=0 > httperf_data/apache/mouse10mb500.txt
