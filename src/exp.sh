#!/bin/bash

# For PSC:

# module load openmpi
# module avail openmpi

# mpic++ -Wall -Wextra -O3 -std=c++2a -fopenmp -o red-black-parallel red-black-lock-free-test.cpp red-black-lock-free.h red-black-lock-free.cpp

# for TEST_CASE in basic bulk
# do
#     for OP in delete insert
#     do
#         for NUMTHREADS in 1 2 4 8
#         do
#             for BATCHSIZE in 8
#             do 
#                 echo "\n"
#                 echo "Running with threads = $NUMTHREADS, batch size = $BATCHSIZE, ${TEST_CASE}_$OP\n"
#                 mpirun -n $NUMTHREADS ./red-black-parallel -f ~/parallel-red-black-trees/src/inputs/${TEST_CASE}_$OP.txt -b $BATCHSIZE -n $NUMTHREADS
#             done
#         done 
#     done
# done

# For 72-core Xeon Phi:
# TODO

# For GHC:
mpic++ -Wall -Wextra -O3 -std=c++2a -fopenmp -o red-black-parallel red-black-lock-free-test.cpp red-black-lock-free.h red-black-lock-free.cpp

for TEST_CASE in basic bulk
do
    for OP in delete insert
    do
        for NUMTHREADS in 1 2 4 8
        do
            for BATCHSIZE in 8
            do 
                echo "\n"
                echo "Running with threads = $NUMTHREADS, batch size = $BATCHSIZE, ${TEST_CASE}_$OP\n"
                mpirun -n $NUMTHREADS ./red-black-parallel -f ~/private/15418/parallel-red-black-trees/src/inputs/${TEST_CASE}_$OP.txt -b $BATCHSIZE -n $NUMTHREADS
            done
        done 
    done
done