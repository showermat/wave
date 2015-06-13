# wave: Wave Propagation Simulation

This is a project that I originally wrote in high school for a computer graphics class, using OpenGL to simulate the propagation of waves through a medium.  It's nothing special, but it looks pretty and makes a good reference for the basics of OpenGL.  It consists of two parts: wave, for waves on a surface, and its little brother wave3d, for waves in 3-dimensional space.  wave3d has a lot of potential, but I haven't had the time to carry a lot of its functionality (like custom node selection) over from wave.

## Usage

This program obviously requires OpenGL, and also needs FreeGLUT installed.

Build and run:

    ./build.sh && ./wave

Keybindings:

  - `w`/`a`/`s`/`d`:  Zoom and rotate the model

  - `p`/`l`/`;`/`'`:  Rotate the model along two axes

  - space:  Start or pause the simulation

  - `.`:  Step the simulation forward

  - `r`:  Reset the current preset model

  - `f`:  Reset everything

  - `i`:  Load preset model (1 through 9, or 0 for flat)

  - `x`:  Toggle wireframe/solid

  - `c`:  Switch node coloring mode

  - `v`:  Toggle surface normals

  - `b`:  Toggle height offset of nodes

  - `m`/`n`:  Increase/decrease attenuation of waves

  - `+`/`-`:  Increase/decrease simulation speed

  - `]`/`[`:  Increase/decrease height exaggeration

  - enter:  Select nodes.  Enter two numbers separated by a comma and press enter again to select the node at that coordinate.  Add a hyphen and another two comma-separated numbers to select a rectangular area of nodes.  Press escape at any time to cancel.

  - `u`:  Display selected nodes

  - `o`:  Toggle removal of the selected nodes from the mesh

  - `h`:  Set the absolute height of the selected nodes

  - `j`:  Adjust the height of the selected nodes by a constant offset

  - `q`:  Exit the program

wave3d has a subset of these keybindings.  In particular, node selection is not supported.

## Issues

  - The display of node connections is not quite right, so in a few corner cases you'll see waves propagate between nodes that appear to be disconnected.

  - Speeding up the simulation too much (holding down the `+` key) causes some things to go to infinity in a very amusing way.

