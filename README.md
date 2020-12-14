# TrackingShot
EZG Project for FH Technikum - Master Game Engineering and Simulation

camera tracking shot to visualize Catmull-spline interpolation for camera positions
as well as SQUAD (SLERP) for rotations.
implements shadows and normal mapping for bumpmaps.

## Key Controls
### 1, 2
changes between edit mode and action mode

### +, -

allows to speed up / slow down the simulation

## Edit Mode
In this mode you can see the floating camera, represented by a pink quad.

### W, A, S, D
controls to move the camera, look around with mouse

### space
With space a new waypoint with the current position and rotation is appended to the list.

### page up, page down 

increase / decrease bumpiness factor

## Anti Aliasing

### F1 - F12

Interactively set the mode for anti-aliasing
```
F1 - toggle multisampling On / Off
F2 - GLFW_DONT_CARE
F3 - 2x(2xMS)
F4 - 2x Quincunx Multisampling
F5 - FSAA disabled
F6 - 4x Bilinear Multisampling
F7 - 4x(4xMS)
F8 - 8x(4xMS, 4xCS)
F9 - 16x(4xMS, 12xCS)
F10 - 8x(4xSS, 2xMS)
F11 - 8x(8xMS)
F12 - 16x(8xMS, 8xCS)
```