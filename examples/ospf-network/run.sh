#!/usr/bin/env bash

# The directory of the results repository
RESULTS_BASE_DIR="/home/noel/Development/results"

# The simulation name
SIMULATION_NAME="ospf-network"

# If set to "true" enable animation
ENABLE_ANIM="true"

# If set to "true" enable eps file output
ENABLE_EPS="true"

# If set to "true" enable all the logging components in DEBUG only
VERBOSE="true"

LGF_FILE="$RESULTS_BASE_DIR/graphs/$SIMULATION_NAME.lgf"
RESULTS_DIR="$RESULTS_BASE_DIR/results/"
RESULTS_FILE_NAME="$SIMULATION_NAME.xml"
LOG_DIR="$RESULTS_BASE_DIR/logs/"
LOG_FILE_NAME="$SIMULATION_NAME.xml"
EPS_FILE="$RESULTS_BASE_DIR/figures/$SIMULATION_NAME.eps"
ANIM_FILE="$RESULTS_BASE_DIR/animations/$SIMULATION_NAME.xml"

OUTPUT_COMMAND="--verbose=$VERBOSE --lgfFile=$LGF_FILE --resultsDir=$RESULTS_DIR --resultsFileName=$RESULTS_FILE_NAME --logDir=$LOG_DIR --logFileName=$LOG_FILE_NAME"

if [ "$ENABLE_ANIM" == "true" ]
then
    OUTPUT_COMMAND="$OUTPUT_COMMAND --animFile=$ANIM_FILE"
fi # Enable animation

if [ "$ENABLE_EPS" == "true" ]
then
  OUTPUT_COMMAND="$OUTPUT_COMMAND --epsFile=$EPS_FILE"
fi # Enable EPS output

echo $OUTPUT_COMMAND

cd ~/Development/ns-3.26 && ./waf --run "$SIMULATION_NAME $OUTPUT_COMMAND"
