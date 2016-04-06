##CodeTransformer
### About
This is a proof-of-concept implementation of a source-to-source code transformer using the ROSE compiler framework https://github.com/rose-compiler/rose. There are two parts of the transformation:
1. Strings that are used in functions are assigned to constant character pointers in the global scope. Additional unnecessary pointers are gotten rid of.
2. Strings which can be safely shifted to PROGMEM are shifted to PROGMEM

### Dependencies
1. Rose Compiler 0.9.6a
2. GCC 4.8
3. Boost 1.53

### Setup
Due to the specific version of GCC required to build ROSE (GCC-4.2.4 to 4.8.4), this project was worked on in a Docker container. The docker container is set up courtesy of https://github.com/AlexMarginean/dockerizedROSE.
However, Docker is not a prerequisite to running the code. It can be run on any computer setup with ROSE, however due to a minor bug in ROSE which affects the function return value propagation for dataflow analysis when analysisDebugLevel is set to 0, the docker image at https://hub.docker.com/r/naomilwx/dockerrosebuild/ should be used

The image docker image should be run on a computer with at least 50GB of free disk space.
### Running the tool
To run the transformer, run
`./run.sh <file-to-transform>`
