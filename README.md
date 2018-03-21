# Videos
- [Dynamically loading the landscape.](https://www.youtube.com/watch?v=kNtDuyCNX14)
- [Removing blocks with the mouse](https://www.youtube.com/watch?v=jbDEUyFM6O4&feature=youtu.be)
- [Rendering shadow map and fog (still with perspective aliasing)](https://www.youtube.com/watch?v=eAai1v2k3uc&feature=youtu.be)

# Building
Since `#include <filesystem>` is not yet supported, specific functions for path manipulations are only implemented on unix systems.
They were however only tested under linux (see https://github.com/leohahn/lt/blob/master/src/lt_fs.cpp#L17).
To build de project:

```
cd vx2
mkdir build
cd build
cmake ..
make
```

The executable is then named `dr`.
