[![CI](https://github.com/xmidt-org/parodus2ccsp/actions/workflows/push.yml/badge.svg)](https://github.com/xmidt-org/parodus2ccsp/actions/workflows/push.yml)
[![codecov.io](http://codecov.io/github/xmidt-org/parodus2ccsp/coverage.svg?branch=master)](http://codecov.io/github/xmidt-org/parodus2ccsp?branch=master)
[![LGTM Analysis](https://github.com/xmidt-org/parodus2ccsp/actions/workflows/codeql-analysis.yml/badge.svg)](https://github.com/xmidt-org/parodus2ccsp/actions/workflows/codeql-analysis.yml)
[![Coverity](https://img.shields.io/coverity/scan/16783.svg)](https://scan.coverity.com/projects/comcast-parodus2ccsp)
[![Apache V2 License](http://img.shields.io/badge/license-Apache%20V2-blue.svg)](https://github.com/Comcast/parodus2ccsp/blob/master/LICENSE)

# parodus2ccsp

Webpa client to communicate with parodus in RDK environment.

# Building and Testing Instructions

```
Pre-Requisite:
--------------
- cmake >= 2.8.7
- openssl >= 1.0.2i and < 1.1.0
- expat

Configuration & Build:
----------------------
mkdir build
cd build
cmake .. -D<option>
sudo make

Test:
-----
By default tests will be disabled. Enable tests by configuring BUILD_TESTING to true and re-build.

cmake .. -DBUILD_TESTING:BOOL=true
sudo make
make test
```
