##CodeTransformer

### Dependencies
1. Rose Compiler 0.9.6a
2. GCC 4.8
3. Boost 1.53

### Setup
Due to the specific version of GCC required to build ROSE (GCC-4.2.4 to 4.8.4), this project was worked on in a Docker container. The docker container is set up courtesy of https://github.com/AlexMarginean/dockerizedROSE.
However, Docker is not a prerequisite to running the code. It can be run on any computer setup with ROSE

### Running the tool
To run the transformer, run
`./run.sh <file-to-transform>`
